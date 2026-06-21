#pragma once

#include "core/CoreTypes.h"

#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace PixelForge {

/// 3D LUT (Look-Up Table) engine for color grading.
/// Supports .cube format (Adobe/DaVinci standard).
/// Uses trilinear interpolation for smooth color mapping.
class LutEngine {
public:
    LutEngine();
    ~LutEngine() = default;

    /// Load a .cube LUT file
    bool loadCubeFile(const std::string& filePath);

    /// Create a passthrough (identity) LUT of given size
    void createIdentity(int size = 33);

    /// Apply LUT to RGB pixel data with trilinear interpolation.
    /// @param pixels RGB data (modified in-place)
    /// @param numPixels Number of pixels
    /// @param intensity Blend intensity 0.0-1.0 (1.0 = full LUT effect)
    void apply(uint8_t* pixels, int numPixels, float intensity = 1.0f) const;

    /// Apply LUT to image
    class Image applyToImage(const class Image& image, float intensity = 1.0f) const;

    /// Check if LUT is loaded and valid
    bool isValid() const;

    /// Get LUT grid size
    int size() const;

    /// Get LUT title
    const std::string& title() const;

    /// Create a LUT by blending between identity and a color transform
    static LutEngine createParametric(
        int lutSize,
        float shadowsR, float shadowsG, float shadowsB,
        float highlightsR, float highlightsG, float highlightsB,
        float gammaR, float gammaG, float gammaB,
        float saturation
    );

private:
    int size_ = 0;
    std::string title_;
    std::vector<std::array<float, 3>> data_; // R, G, B in [0,1]

    /// Trilinear interpolation lookup
    std::array<float, 3> sample(float r, float g, float b) const;

    /// Parse a single .cube line
    bool parseLine(const std::string& line, int& writeIndex);
};

} // namespace PixelForge