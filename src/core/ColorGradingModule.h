#pragma once

#include "CoreTypes.h"
#include "Image.h"
#include "utils/LutEngine.h"
#include "utils/ToneCurve.h"
#include "utils/NoiseGenerator.h"

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace PixelForge {

/// Cinematic & style color grading module.
/// Combines 3D LUT, tone curves, split toning, grain, vignette, and more.
class ColorGradingModule {
public:
    ColorGradingModule();
    ~ColorGradingModule();

    /// Apply a preset filter to an image
    /// @param image Source image
    /// @param presetId Preset identifier string
    /// @param params Filter parameters
    /// @param progress Optional progress callback
    /// @return Processed image
    Image applyPreset(const Image& image,
                      const std::string& presetId,
                      const FilterParameters& params,
                      ProgressCallback progress = nullptr);

    /// Apply a custom LUT file
    Image applyCustomLut(const Image& image,
                         const std::string& lutFilePath,
                         const FilterParameters& params);

    /// Apply color grading pipeline to image (advanced)
    Image applyPipeline(const Image& image,
                        const LutEngine& lut,
                        const ToneCurve& curve,
                        const FilterParameters& params);

    /// Get list of all available preset IDs
    std::vector<std::string> availablePresets() const;

    /// Get preset info by ID
    const PresetInfo* getPresetInfo(const std::string& id) const;

    /// Register a custom preset
    void registerPreset(const PresetInfo& info);

    /// Save current params as custom preset
    PresetInfo saveCustomPreset(const std::string& name,
                                 const std::string& basedOnPresetId,
                                 const FilterParameters& params);

    /// Load all built-in presets
    void loadBuiltInPresets();

private:
    std::unordered_map<std::string, PresetInfo> presets_;
    std::unordered_map<std::string, LutEngine> luts_;

    // Built-in LUT generators
    LutEngine createTealOrangeLut();
    LutEngine createBleachBypassLut();
    LutEngine createMoodyBlueLut();
    LutEngine createFilmNoirLut();
    LutEngine createGoldenHourLut();
    LutEngine createMonoNoAwareLut();
    LutEngine createWongKarwaiNeonLut();
    LutEngine createShowaRetroLut();
    LutEngine createAnimeFlatLut();
    LutEngine createVintageKodakLut();
    LutEngine createVintagePolaroidLut();
    LutEngine createBwFineArtLut();
    LutEngine createHdrDramaticLut();
    LutEngine createPastelSoftLut();
    LutEngine createUrbanStreetGrittyLut();

    // Pipeline helpers
    Image applySplitToning(const Image& image, float shadowR, float shadowG, float shadowB,
                            float highlightR, float highlightG, float highlightB,
                            float amount);

    Image applyVignette(const Image& image, float strength);

    Image applyTemperatureTint(const Image& image, float temperature, float tint);

    Image applySaturation(const Image& image, float amount);

    Image applyContrastBrightness(const Image& image, float contrast, float brightness);

    Image applyHighlightsShadows(const Image& image, float highlights, float shadows);
};

} // namespace PixelForge