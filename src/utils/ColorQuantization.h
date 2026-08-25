#pragma once

#include "core/CoreTypes.h"

#include <vector>
#include <cstdint>

namespace PixelForge {

/// K-Means color quantization for WPAP and palette generation.
class ColorQuantization {
public:
    ColorQuantization() = default;
    ~ColorQuantization() = default;

    /// Quantize an image to a limited color palette using k-means in LAB space.
    /// @param pixels Input pixel data (RGB, row-major)
    /// @param width Image width
    /// @param height Image height
    /// @param k Number of colors in output palette
    /// @param maxIterations Maximum k-means iterations
    /// @return Vector of k dominant colors (RGB)
    static std::vector<Color3u8> quantize(
        const uint8_t* pixels, int width, int height,
        int k = 16, int maxIterations = 20
    );

    /// Quantize using an image directly
    static std::vector<Color3u8> quantizeImage(
        const class Image& image, int k = 16, int maxIterations = 20
    );

    /// Map each pixel to the nearest color in the palette.
    /// Returns a new image with quantized colors.
    static class Image applyPalette(
        const class Image& image,
        const std::vector<Color3u8>& palette
    );

    /// Assign each pixel an index into the palette.
    static std::vector<int> assignLabels(
        const uint8_t* pixels, int numPixels, int channels,
        const std::vector<Color3u8>& palette
    );

    /// Generate preset palettes
    static std::vector<Color3u8> vibrantPalette(int k = 16);
    static std::vector<Color3u8> pastelPalette(int k = 16);
    static std::vector<Color3u8> monochromeAccentPalette(int k = 16);
    static std::vector<Color3u8> sunsetPalette(int k = 16);
    static std::vector<Color3u8> oceanPalette(int k = 16);

    /// Compute mean color of a group of pixels
    static Color3u8 meanColor(
        const uint8_t* pixels, int channels,
        const std::vector<int>& indices
    );

private:
    /// Convert RGB to LAB (floating point)
    static void rgbToLab(uint8_t r, uint8_t g, uint8_t b,
                         float& L, float& A, float& B);

    /// Convert LAB to RGB
    static Color3u8 labToRgb(float L, float A, float B);

    /// Initialize k-means centroids using k-means++ strategy
    static std::vector<std::array<float, 3>> initCentroidsKpp(
        const std::vector<std::array<float, 3>>& labPixels, int k
    );
};

} // namespace PixelForge