#include "ProjectManager.h"
#include "core/IoModule.h"

#include <fstream>
#include <sstream>
#include <filesystem>
#include <atomic>
#include <iomanip>
#include <ctime>

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
    // Simple JSON-like format for .pforge
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"version\": \"1.0\",\n";
    file << "  \"sourceImage\": \"" << impl_->sourcePath << "\",\n";
    file << "  \"layers\": [\n";
    for (size_t i = 0; i < impl_->layers.size(); ++i) {
        const auto& l = impl_->layers[i];
        file << "    {\n";
        file << "      \"id\": \"" << l.id << "\",\n";
        file << "      \"name\": \"" << l.name << "\",\n";
        file << "      \"enabled\": " << (l.enabled ? "true" : "false") << ",\n";
        file << "      \"presetId\": \"" << l.presetId << "\",\n";
        file << "      \"isWapMode\": " << (l.isWapMode ? "true" : "false") << ",\n";
        file << "      \"opacity\": " << l.opacity << ",\n";
        file << "      \"params\": {\n";
        file << "        \"intensity\": " << l.params.intensity << ",\n";
        file << "        \"grainAmount\": " << l.params.grainAmount << ",\n";
        file << "        \"vignetteStrength\": " << l.params.vignetteStrength << ",\n";
        file << "        \"temperature\": " << l.params.temperature << ",\n";
        file << "        \"tint\": " << l.params.tint << ",\n";
        file << "        \"contrast\": " << l.params.contrast << ",\n";
        file << "        \"brightness\": " << l.params.brightness << ",\n";
        file << "        \"saturation\": " << l.params.saturation << ",\n";
        file << "        \"highlights\": " << l.params.highlights << ",\n";
        file << "        \"shadows\": " << l.params.shadows << "\n";
        file << "      }\n";
        file << "    }" << (i + 1 < impl_->layers.size() ? "," : "") << "\n";
    }
    file << "  ]\n";
    file << "}\n";

    file.close();
    impl_->projectPath = filePath;
    impl_->unsavedChanges = false;
    return true;
}

bool ProjectManager::loadProject(const std::string& filePath) {
    // For v1, we just re-open the source image
    // Full JSON parsing would use a library like nlohmann/json
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    // Minimal parsing: extract sourceImage path
    std::string line;
    std::string sourcePath;
    while (std::getline(file, line)) {
        if (line.find("\"sourceImage\"") != std::string::npos) {
            auto start = line.find('"', line.find(':') + 1);
            if (start != std::string::npos) {
                start = line.find('"', start + 1);
                auto end = line.rfind('"');
                if (start != std::string::npos && end > start) {
                    sourcePath = line.substr(start + 1, end - start - 1);
                }
            }
        }
    }
    file.close();

    if (sourcePath.empty()) return false;
    if (!openImage(sourcePath)) return false;

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