#include "ColorGradingModule.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <sstream>
#include <iomanip>

namespace PixelForge {

ColorGradingModule::ColorGradingModule() {
    loadBuiltInPresets();
}

ColorGradingModule::~ColorGradingModule() = default;

// ============================================================
// Preset Registration & Lookup
// ============================================================

void ColorGradingModule::loadBuiltInPresets() {
    auto registerPresetLut = [this](const std::string& id, const std::string& name,
                                     FilterCategory cat, const std::string& desc,
                                     LutEngine lut) {
        PresetInfo info;
        info.id = id;
        info.name = name;
        info.category = cat;
        info.description = desc;
        info.isBuiltIn = true;
        presets_[id] = info;
        luts_[id] = std::move(lut);
    };

    registerPresetLut("cinematic_teal_orange", "Teal & Orange", FilterCategory::Cinematic,
                      "Shadow teal, warm skin tones, high contrast",
                      createTealOrangeLut());
    registerPresetLut("cinematic_bleach_bypass", "Bleach Bypass", FilterCategory::Cinematic,
                      "Partial desaturation, extreme contrast",
                      createBleachBypassLut());
    registerPresetLut("cinematic_moody_blue", "Moody Blue", FilterCategory::Cinematic,
                      "Dark blue dominant, deep shadows",
                      createMoodyBlueLut());
    registerPresetLut("cinematic_film_noir", "Film Noir", FilterCategory::Cinematic,
                      "High contrast B&W, strong vignette",
                      createFilmNoirLut());
    registerPresetLut("cinematic_golden_hour", "Golden Hour", FilterCategory::Cinematic,
                      "Warm golden tones, soft highlights",
                      createGoldenHourLut());
    registerPresetLut("japan_mono_no_aware", "Mono no Aware", FilterCategory::JapanStyle,
                      "Muted pastel, low contrast, blue-green tint",
                      createMonoNoAwareLut());
    registerPresetLut("japan_wong_karwai", "Wong Kar-wai Neon", FilterCategory::JapanStyle,
                      "High saturation neon, dark shadows, grain",
                      createWongKarwaiNeonLut());
    registerPresetLut("japan_showa_retro", "Showa Retro Film", FilterCategory::JapanStyle,
                      "80s-90s Fuji emulation, warm, vignette",
                      createShowaRetroLut());
    registerPresetLut("japan_anime_flat", "Anime Flat Look", FilterCategory::JapanStyle,
                      "High contrast, vivid saturation, posterized",
                      createAnimeFlatLut());
    registerPresetLut("vintage_kodak", "Vintage Kodak", FilterCategory::VintageRetro,
                      "Warm tones, fine grain, medium contrast",
                      createVintageKodakLut());
    registerPresetLut("vintage_polaroid", "Vintage Polaroid", FilterCategory::VintageRetro,
                      "Faded colors, rounded vignette, warm whites",
                      createVintagePolaroidLut());
    registerPresetLut("mono_bw_fineart", "B&W Fine Art", FilterCategory::Monochrome,
                      "Wide tonal range, fine grain",
                      createBwFineArtLut());
    registerPresetLut("other_hdr_dramatic", "HDR Dramatic", FilterCategory::Other,
                      "Extended dynamic range, maximum detail",
                      createHdrDramaticLut());
    registerPresetLut("other_pastel_soft", "Pastel Soft", FilterCategory::Other,
                      "Low saturation, bright highlights, soft glow",
                      createPastelSoftLut());
    registerPresetLut("other_urban_gritty", "Urban Street Gritty", FilterCategory::Other,
                      "High contrast, partial desat, heavy grain",
                      createUrbanStreetGrittyLut());
}

std::vector<std::string> ColorGradingModule::availablePresets() const {
    std::vector<std::string> ids;
    ids.reserve(presets_.size());
    for (const auto& [id, _] : presets_) {
        ids.push_back(id);
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

const PresetInfo* ColorGradingModule::getPresetInfo(const std::string& id) const {
    auto it = presets_.find(id);
    return (it != presets_.end()) ? &it->second : nullptr;
}

void ColorGradingModule::registerPreset(const PresetInfo& info) {
    presets_[info.id] = info;
}

PresetInfo ColorGradingModule::saveCustomPreset(
    const std::string& name,
    const std::string& basedOnPresetId,
    const FilterParameters& params)
{
    PresetInfo info;
    info.id = "custom_" + name;
    info.name = name;
    info.category = FilterCategory::Other;
    info.description = "Custom preset based on " + basedOnPresetId;
    info.isBuiltIn = false;
    info.defaultParams = params;

    // Copy the LUT from the base preset if it exists
    auto lutIt = luts_.find(basedOnPresetId);
    if (lutIt != luts_.end()) {
        luts_[info.id] = lutIt->second;
    }

    presets_[info.id] = info;
    return info;
}

// ============================================================
// Main Apply
// ============================================================

Image ColorGradingModule::applyPreset(
    const Image& image,
    const std::string& presetId,
    const FilterParameters& params,
    ProgressCallback progress)
{
    if (image.isEmpty()) return image;

    if (progress) progress(0.1f, "Loading preset...");

    auto lutIt = luts_.find(presetId);
    if (lutIt == luts_.end()) {
        // Unknown preset, return copy
        return image.deepCopy();
    }

    float intensity = std::clamp(params.intensity, 0.0f, 100.0f) / 100.0f;

    if (progress) progress(0.3f, "Applying LUT...");
    Image result = applyPipeline(image, lutIt->second, ToneCurve(), params);

    return result;
}

Image ColorGradingModule::applyCustomLut(
    const Image& image,
    const std::string& lutFilePath,
    const FilterParameters& params)
{
    if (image.isEmpty()) return image;

    LutEngine lut;
    if (!lut.loadCubeFile(lutFilePath)) return image.deepCopy();

    return applyPipeline(image, lut, ToneCurve(), params);
}

Image ColorGradingModule::applyPipeline(
    const Image& image,
    const LutEngine& lut,
    const ToneCurve& curve,
    const FilterParameters& params)
{
    if (image.isEmpty()) return image;

    float intensity = std::clamp(params.intensity, 0.0f, 100.0f) / 100.0f;

    // Start with a copy
    Image result = image.deepCopy();

    // Step 1: Apply highlights/shadows adjustment
    if (params.highlights != 0.0f || params.shadows != 0.0f) {
        result = applyHighlightsShadows(result, params.highlights, params.shadows);
    }

    // Step 2: Apply contrast/brightness
    if (params.contrast != 0.0f || params.brightness != 0.0f) {
        result = applyContrastBrightness(result, params.contrast, params.brightness);
    }

    // Step 3: Apply 3D LUT
    if (lut.isValid()) {
        cv::Mat& mat = result.mat();
        int totalPixels = mat.rows * mat.cols;
        lut.apply(mat.data, totalPixels, intensity);
    }

    // Step 4: Apply tone curve
    {
        cv::Mat& mat = result.mat();
        int totalPixels = mat.rows * mat.cols;
        curve.apply(mat.data, totalPixels);
    }

    // Step 5: Temperature/Tint
    if (params.temperature != 0.0f || params.tint != 0.0f) {
        result = applyTemperatureTint(result, params.temperature, params.tint);
    }

    // Step 6: Saturation
    if (params.saturation != 0.0f) {
        result = applySaturation(result, params.saturation);
    }

    // Step 7: Grain
    if (params.grainAmount > 0.0f) {
        cv::Mat& mat = result.mat();
        NoiseGenerator::applyFilmGrain(mat.data, mat.cols, mat.rows,
                                        params.grainAmount, 40.0f);
    }

    // Step 8: Vignette
    if (params.vignetteStrength > 0.0f) {
        result = applyVignette(result, params.vignetteStrength);
    }

    return result;
}

// ============================================================
// Pipeline Helpers
// ============================================================

Image ColorGradingModule::applySplitToning(
    const Image& image,
    float shadowR, float shadowG, float shadowB,
    float highlightR, float highlightG, float highlightB,
    float amount)
{
    if (amount <= 0.0f) return image;

    float norm = amount / 100.0f;
    Image result = image.deepCopy();
    cv::Mat& mat = result.mat();
    int total = mat.rows * mat.cols;
    int ch = mat.channels();

    for (int i = 0; i < total; ++i) {
        uint8_t* p = mat.data + i * ch;
        float r = p[0], g = p[1], b = p[2];
        float lum = (0.299f * r + 0.587f * g + 0.114f * b) / 255.0f;

        float shadowWeight = std::pow(1.0f - lum, 2.0f) * norm;
        float highlightWeight = std::pow(lum, 2.0f) * norm;

        r += (shadowR * shadowWeight + highlightR * highlightWeight) * 40.0f;
        g += (shadowG * shadowWeight + highlightG * highlightWeight) * 40.0f;
        b += (shadowB * shadowWeight + highlightB * highlightWeight) * 40.0f;

        p[0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
        p[1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
        p[2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
    }

    return result;
}

Image ColorGradingModule::applyVignette(const Image& image, float strength) {
    if (strength <= 0.0f) return image;

    float norm = std::clamp(strength, 0.0f, 100.0f) / 100.0f;
    Image result = image.deepCopy();
    cv::Mat& mat = result.mat();
    int w = mat.cols, h = mat.rows;
    int ch = mat.channels();

    float cx = w / 2.0f, cy = h / 2.0f;
    float maxDist = std::sqrt(cx * cx + cy * cy);

    for (int y = 0; y < h; ++y) {
        uint8_t* row = mat.ptr<uint8_t>(y);
        for (int x = 0; x < w; ++x) {
            float dx = x - cx, dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy) / maxDist;

            // Smooth vignette falloff
            float vignette = 1.0f - std::pow(dist, 1.5f) * norm * 0.8f;
            vignette = std::clamp(vignette, 0.0f, 1.0f);

            row[x * ch + 0] = static_cast<uint8_t>(row[x * ch + 0] * vignette);
            row[x * ch + 1] = static_cast<uint8_t>(row[x * ch + 1] * vignette);
            row[x * ch + 2] = static_cast<uint8_t>(row[x * ch + 2] * vignette);
        }
    }

    return result;
}

Image ColorGradingModule::applyTemperatureTint(
    const Image& image, float temperature, float tint)
{
    if (temperature == 0.0f && tint == 0.0f) return image;

    float tempShift = temperature / 100.0f * 30.0f;
    float tintShift = tint / 100.0f * 20.0f;

    Image result = image.deepCopy();
    cv::Mat& mat = result.mat();
    int total = mat.rows * mat.cols;
    int ch = mat.channels();

    for (int i = 0; i < total; ++i) {
        uint8_t* p = mat.data + i * ch;
        // Temperature: warm = +R, -B; cool = -R, +B
        float r = p[0] + tempShift;
        float g = p[1];
        float b = p[2] - tempShift;
        // Tint: + = magenta (R+B), - = green (G)
        r += tintShift * 0.5f;
        g -= tintShift;
        b += tintShift * 0.5f;

        p[0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
        p[1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
        p[2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
    }

    return result;
}

Image ColorGradingModule::applySaturation(const Image& image, float amount) {
    if (amount == 0.0f) return image;

    float factor = 1.0f + amount / 100.0f;
    factor = std::clamp(factor, 0.0f, 3.0f);

    Image result = image.deepCopy();
    cv::Mat& mat = result.mat();
    int total = mat.rows * mat.cols;
    int ch = mat.channels();

    for (int i = 0; i < total; ++i) {
        uint8_t* p = mat.data + i * ch;
        float r = p[0], g = p[1], b = p[2];
        float gray = 0.299f * r + 0.587f * g + 0.114f * b;

        r = gray + (r - gray) * factor;
        g = gray + (g - gray) * factor;
        b = gray + (b - gray) * factor;

        p[0] = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
        p[1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
        p[2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
    }

    return result;
}

Image ColorGradingModule::applyContrastBrightness(
    const Image& image, float contrast, float brightness)
{
    if (contrast == 0.0f && brightness == 0.0f) return image;

    float contrastFactor = 1.0f + contrast / 100.0f;
    float brightnessShift = brightness / 100.0f * 128.0f;

    Image result = image.deepCopy();
    cv::Mat& mat = result.mat();
    int total = mat.rows * mat.cols;
    int ch = mat.channels();

    for (int i = 0; i < total; ++i) {
        uint8_t* p = mat.data + i * ch;
        for (int c = 0; c < 3; ++c) {
            float val = (p[c] - 128.0f) * contrastFactor + 128.0f + brightnessShift;
            p[c] = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
        }
    }

    return result;
}

Image ColorGradingModule::applyHighlightsShadows(
    const Image& image, float highlights, float shadows)
{
    if (highlights == 0.0f && shadows == 0.0f) return image;

    float hiNorm = highlights / 100.0f;
    float shNorm = shadows / 100.0f;

    // Build a combined LUT
    std::array<uint8_t, 256> lut;
    for (int i = 0; i < 256; ++i) {
        float t = i / 255.0f;
        float val = i;

        // Shadows: affect dark areas
        if (shNorm != 0.0f) {
            float shadowMask = 1.0f - t;
            shadowMask *= shadowMask;
            val += shNorm * 80.0f * shadowMask;
        }

        // Highlights: affect bright areas
        if (hiNorm != 0.0f) {
            float highlightMask = t;
            highlightMask *= highlightMask;
            val -= hiNorm * 60.0f * highlightMask;
        }

        lut[i] = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
    }

    Image result = image.deepCopy();
    cv::Mat& mat = result.mat();
    int total = mat.rows * mat.cols;
    int ch = mat.channels();

    for (int i = 0; i < total; ++i) {
        uint8_t* p = mat.data + i * ch;
        p[0] = lut[p[0]];
        p[1] = lut[p[1]];
        p[2] = lut[p[2]];
    }

    return result;
}

// ============================================================
// Built-in LUT Generators
// ============================================================

LutEngine ColorGradingModule::createTealOrangeLut() {
    // Teal shadows (-0.2 R, +0.1 B), warm highlights (+0.15 R, -0.1 B)
    return LutEngine::createParametric(17,
        -0.05f, 0.0f, 0.15f,   // shadows: teal
         0.20f, 0.05f, -0.10f,  // highlights: orange
         1.1f, 1.0f, 0.95f,     // gamma: slightly warm
         120.0f);                // saturation boost
}

LutEngine ColorGradingModule::createBleachBypassLut() {
    return LutEngine::createParametric(17,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.3f, 1.3f, 1.3f,      // high contrast gamma
        40.0f);                 // heavy desaturation
}

LutEngine ColorGradingModule::createMoodyBlueLut() {
    return LutEngine::createParametric(17,
        -0.10f, -0.05f, 0.25f,  // blue shadows
        -0.05f, 0.0f, 0.10f,   // slight blue highlights
        0.95f, 0.95f, 1.05f,
        85.0f);
}

LutEngine ColorGradingModule::createFilmNoirLut() {
    // High contrast, fully desaturated B&W
    return LutEngine::createParametric(17,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.5f, 1.5f, 1.5f,  // very high contrast
        0.0f);             // fully desaturated
}

LutEngine ColorGradingModule::createGoldenHourLut() {
    return LutEngine::createParametric(17,
        0.10f, 0.05f, -0.10f,  // warm shadows
        0.20f, 0.12f, -0.05f,  // golden highlights
        1.0f, 0.98f, 0.90f,
        110.0f);
}

LutEngine ColorGradingModule::createMonoNoAwareLut() {
    return LutEngine::createParametric(17,
        -0.05f, 0.0f, 0.10f,   // blue-green shadows
        0.02f, 0.05f, 0.10f,   // blue highlights
        0.9f, 0.9f, 0.9f,      // lower contrast
        60.0f);                 // desaturated
}

LutEngine ColorGradingModule::createWongKarwaiNeonLut() {
    return LutEngine::createParametric(17,
        0.15f, -0.10f, 0.05f,  // red-warm shadows
        0.05f, 0.10f, -0.05f,  // green highlights
        1.2f, 1.1f, 1.0f,      // boosted contrast
        150.0f);                // very saturated
}

LutEngine ColorGradingModule::createShowaRetroLut() {
    return LutEngine::createParametric(17,
        0.10f, 0.05f, -0.10f,  // warm shadows
        0.10f, 0.05f, -0.08f,  // warm highlights
        1.05f, 1.0f, 0.95f,
        90.0f);
}

LutEngine ColorGradingModule::createAnimeFlatLut() {
    return LutEngine::createParametric(17,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.2f, 1.2f, 1.2f,      // high contrast
        140.0f);                // high saturation
}

LutEngine ColorGradingModule::createVintageKodakLut() {
    return LutEngine::createParametric(17,
        0.10f, 0.05f, -0.05f,  // warm shadows
        0.08f, 0.05f, -0.03f,  // warm highlights
        1.05f, 1.0f, 0.95f,
        95.0f);
}

LutEngine ColorGradingModule::createVintagePolaroidLut() {
    return LutEngine::createParametric(17,
        0.08f, 0.05f, -0.03f,
        0.10f, 0.08f, 0.05f,   // creamy highlights
        0.9f, 0.9f, 0.9f,      // lower contrast, faded
        75.0f);
}

LutEngine ColorGradingModule::createBwFineArtLut() {
    return LutEngine::createParametric(17,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.1f, 1.1f, 1.1f,
        0.0f);  // fully grayscale
}

LutEngine ColorGradingModule::createHdrDramaticLut() {
    return LutEngine::createParametric(17,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        0.8f, 0.8f, 0.8f,      // expanded dynamic range (inverse gamma)
        130.0f);                // boosted saturation
}

LutEngine ColorGradingModule::createPastelSoftLut() {
    return LutEngine::createParametric(17,
        0.05f, 0.05f, 0.10f,   // slight blue lift
        0.08f, 0.08f, 0.10f,   // bright highlights
        0.85f, 0.85f, 0.85f,   // lifted blacks
        50.0f);                 // low saturation
}

LutEngine ColorGradingModule::createUrbanStreetGrittyLut() {
    return LutEngine::createParametric(17,
        0.05f, 0.0f, -0.05f,   // slightly warm shadows
        -0.05f, 0.0f, 0.05f,   // cool highlights
        1.3f, 1.3f, 1.3f,      // high contrast
        60.0f);                 // partial desaturation
}

} // namespace PixelForge