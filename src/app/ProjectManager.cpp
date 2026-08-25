#include "ProjectManager.h"
#include "core/IoModule.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <atomic>
#include <iomanip>
#include <ctime>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace PixelForge {

namespace fs = std::filesystem;

struct ProjectManager::Impl {
    Image sourceImage;
    Image currentImage;
    std::vector<AdjustmentLayer> layers;
    HistoryManager history;
    std::string sourcePath;
    std::string projectPath;
    bool unsavedChanges = false;
};

static std::atomic<uint64_t> s_layerCounter{0};

ProjectManager::ProjectManager() : impl_(std::make_unique<Impl>()) {}
ProjectManager::~ProjectManager() = default;

std::string ProjectManager::generateLayerId() {
    return "layer_" + std::to_string(s_layerCounter.fetch_add(1));
}

// ---- Image Management ----

bool ProjectManager::openImage(const std::string& filePath) {
    try {
        impl_->sourceImage = IoModule::loadImage(filePath);
        impl_->currentImage = impl_->sourceImage.deepCopy();
        impl_->sourcePath = filePath;
        impl_->projectPath.clear();
        impl_->layers.clear();
        impl_->history.clear();
        impl_->history.pushState(impl_->currentImage, "Original");
        impl_->unsavedChanges = false;
        return true;
    } catch (...) {
        return false;
    }
}

void ProjectManager::closeImage() {
    impl_->sourceImage = Image();
    impl_->currentImage = Image();
    impl_->layers.clear();
    impl_->history.clear();
    impl_->sourcePath.clear();
    impl_->projectPath.clear();
    impl_->unsavedChanges = false;
}

bool ProjectManager::hasImage() const {
    return !impl_->sourceImage.isEmpty();
}

const Image& ProjectManager::sourceImage() const {
    return impl_->sourceImage;
}

const Image& ProjectManager::currentImage() const {
    return impl_->currentImage;
}

void ProjectManager::setCurrentImage(const Image& image, const std::string& description) {
    impl_->currentImage = image.deepCopy();
    impl_->history.pushState(impl_->currentImage, description);
    impl_->unsavedChanges = true;
}

void ProjectManager::restoreCurrentImage(const Image& image) {
    impl_->currentImage = image.deepCopy();
    impl_->unsavedChanges = true;
}

// ---- Adjustment Layers ----

void ProjectManager::addLayer(const AdjustmentLayer& layer) {
    AdjustmentLayer l = layer;
    if (l.id.empty()) l.id = generateLayerId();
    impl_->layers.push_back(l);
    impl_->unsavedChanges = true;
}

void ProjectManager::removeLayer(const std::string& layerId) {
    auto& v = impl_->layers;
    v.erase(std::remove_if(v.begin(), v.end(),
        [&](const AdjustmentLayer& l) { return l.id == layerId; }), v.end());
    impl_->unsavedChanges = true;
}

void ProjectManager::toggleLayer(const std::string& layerId, bool enabled) {
    auto* l = findLayer(layerId);
    if (l) { l->enabled = enabled; impl_->unsavedChanges = true; }
}

void ProjectManager::updateLayerParams(const std::string& layerId, const FilterParameters& params) {
    auto* l = findLayer(layerId);
    if (l) { l->params = params; impl_->unsavedChanges = true; }
}

void ProjectManager::reorderLayer(const std::string& layerId, int newIndex) {
    auto& v = impl_->layers;
    for (size_t i = 0; i < v.size(); ++i) {
        if (v[i].id == layerId) {
            AdjustmentLayer temp = v[i];
            v.erase(v.begin() + i);
            int insertPos = std::clamp(newIndex, 0, static_cast<int>(v.size()));
            v.insert(v.begin() + insertPos, temp);
            impl_->unsavedChanges = true;
            return;
        }
    }
}

const std::vector<AdjustmentLayer>& ProjectManager::layers() const {
    return impl_->layers;
}

std::vector<AdjustmentLayer>& ProjectManager::layers() {
    return impl_->layers;
}

AdjustmentLayer* ProjectManager::findLayer(const std::string& layerId) {
    for (auto& l : impl_->layers) {
        if (l.id == layerId) return &l;
    }
    return nullptr;
}

void ProjectManager::clearLayers() {
    impl_->layers.clear();
    impl_->unsavedChanges = true;
}

// ---- History ----

HistoryManager& ProjectManager::history() { return impl_->history; }
const HistoryManager& ProjectManager::history() const { return impl_->history; }

// ---- Project Save/Load (.pforge JSON) ----

bool ProjectManager::saveProject(const std::string& filePath) {
    QJsonArray layers;
    for (const auto& layer : impl_->layers) {
        const auto& p = layer.params;
        const auto& w = layer.wapParams;
        QJsonObject params{
            {"intensity", p.intensity}, {"grainAmount", p.grainAmount},
            {"vignetteStrength", p.vignetteStrength}, {"temperature", p.temperature},
            {"tint", p.tint}, {"contrast", p.contrast}, {"brightness", p.brightness},
            {"saturation", p.saturation}, {"highlights", p.highlights},
            {"shadows", p.shadows}
        };
        QJsonObject wapParams{
            {"colorCount", w.colorCount},
            {"detailLevel", static_cast<int>(w.detailLevel)},
            {"customPointCount", w.customPointCount},
            {"palettePreset", static_cast<int>(w.palettePreset)},
            {"faceDetectionEnabled", w.faceDetectionEnabled},
            {"faceDetailBoost", w.faceDetailBoost}
        };
        QJsonArray customPalette;
        for (const auto& color : w.customPalette) {
            customPalette.append(QJsonArray{color.r, color.g, color.b});
        }
        wapParams.insert("customPalette", customPalette);
        layers.append(QJsonObject{
            {"id", QString::fromStdString(layer.id)},
            {"name", QString::fromStdString(layer.name)},
            {"enabled", layer.enabled},
            {"presetId", QString::fromStdString(layer.presetId)},
            {"isWapMode", layer.isWapMode},
            {"opacity", layer.opacity},
            {"params", params},
            {"wapParams", wapParams}
        });
    }

    QJsonObject root{
        {"version", "1.0"},
        {"sourceImage", QString::fromStdString(impl_->sourcePath)},
        {"layers", layers}
    };

    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();
    impl_->projectPath = filePath;
    impl_->unsavedChanges = false;
    return true;
}

bool ProjectManager::loadProject(const std::string& filePath) {
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly)) return false;

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) return false;

    const QJsonObject root = document.object();
    const std::string sourcePath = root.value("sourceImage").toString().toStdString();
    if (sourcePath.empty()) return false;
    if (!openImage(sourcePath)) return false;

    const QJsonArray layers = root.value("layers").toArray();
    for (const QJsonValue& value : layers) {
        const QJsonObject object = value.toObject();
        const QJsonObject params = object.value("params").toObject();
        const QJsonObject wapParams = object.value("wapParams").toObject();
        AdjustmentLayer layer;
        layer.id = object.value("id").toString().toStdString();
        layer.name = object.value("name").toString().toStdString();
        layer.enabled = object.value("enabled").toBool(true);
        layer.presetId = object.value("presetId").toString().toStdString();
        layer.isWapMode = object.value("isWapMode").toBool(false);
        layer.opacity = static_cast<float>(object.value("opacity").toDouble(100.0));
        layer.params.intensity = static_cast<float>(params.value("intensity").toDouble(100.0));
        layer.params.grainAmount = static_cast<float>(params.value("grainAmount").toDouble());
        layer.params.vignetteStrength = static_cast<float>(params.value("vignetteStrength").toDouble());
        layer.params.temperature = static_cast<float>(params.value("temperature").toDouble());
        layer.params.tint = static_cast<float>(params.value("tint").toDouble());
        layer.params.contrast = static_cast<float>(params.value("contrast").toDouble());
        layer.params.brightness = static_cast<float>(params.value("brightness").toDouble());
        layer.params.saturation = static_cast<float>(params.value("saturation").toDouble());
        layer.params.highlights = static_cast<float>(params.value("highlights").toDouble());
        layer.params.shadows = static_cast<float>(params.value("shadows").toDouble());
        layer.wapParams.colorCount = wapParams.value("colorCount").toInt(16);
        layer.wapParams.detailLevel = static_cast<WapDetailLevel>(
            wapParams.value("detailLevel").toInt(static_cast<int>(WapDetailLevel::Medium)));
        layer.wapParams.customPointCount = wapParams.value("customPointCount").toInt(2000);
        layer.wapParams.palettePreset = static_cast<WapPalettePreset>(
            wapParams.value("palettePreset").toInt(static_cast<int>(WapPalettePreset::Vibrant)));
        layer.wapParams.faceDetectionEnabled = wapParams.value("faceDetectionEnabled").toBool(true);
        layer.wapParams.faceDetailBoost = static_cast<float>(
            wapParams.value("faceDetailBoost").toDouble(1.5));
        for (const QJsonValue& colorValue : wapParams.value("customPalette").toArray()) {
            const QJsonArray color = colorValue.toArray();
            if (color.size() != 3) continue;
            layer.wapParams.customPalette.push_back({
                static_cast<uint8_t>(color.at(0).toInt()),
                static_cast<uint8_t>(color.at(1).toInt()),
                static_cast<uint8_t>(color.at(2).toInt())});
        }
        impl_->layers.push_back(std::move(layer));
    }

    impl_->projectPath = filePath;
    impl_->unsavedChanges = false;
    return true;
}

bool ProjectManager::hasUnsavedChanges() const {
    return impl_->unsavedChanges;
}

void ProjectManager::markSaved() {
    impl_->unsavedChanges = false;
}

// ---- Export ----

bool ProjectManager::exportImage(const std::string& filePath, int jpegQuality) {
    if (impl_->currentImage.isEmpty()) return false;
    return impl_->currentImage.save(filePath, jpegQuality);
}

// ---- Paths ----

const std::string& ProjectManager::sourcePath() const {
    return impl_->sourcePath;
}

const std::string& ProjectManager::projectPath() const {
    return impl_->projectPath;
}

std::string ProjectManager::suggestedExportName(const std::string& presetName) const {
    if (impl_->sourcePath.empty()) return "export.png";

    fs::path src(impl_->sourcePath);
    std::string stem = src.stem().string();
    std::string ext = ".png";

    if (presetName.empty()) {
        return stem + "_edited" + ext;
    }
    return stem + "_" + presetName + ext;
}

} // namespace PixelForge