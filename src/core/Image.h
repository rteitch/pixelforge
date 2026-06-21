#pragma once

#include "CoreTypes.h"

#include <opencv2/core.hpp>

#include <memory>
#include <string>
#include <vector>
#include <cstdint>

namespace PixelForge {

/// Wrapper around cv::Mat providing convenient image operations.
/// Supports non-destructive editing via clone semantics and reference sharing.
class Image {
public:
    Image();
    explicit Image(const cv::Mat& mat);
    Image(int width, int height, int channels);
    Image(const Image& other);
    Image(Image&& other) noexcept;
    Image& operator=(const Image& other);
    Image& operator=(Image&& other) noexcept;
    ~Image();

    // ---- Factory ----
    static Image fromFile(const std::string& path);
    static Image fromBuffer(const uint8_t* data, size_t size);
    static Image empty(int width, int height, int channels = 3);

    // ---- Properties ----
    int width() const;
    int height() const;
    int channels() const;
    Size2i size() const;
    bool isEmpty() const;
    bool hasAlpha() const;
    size_t memoryBytes() const;

    // ---- Access ----
    cv::Mat& mat();
    const cv::Mat& mat() const;
    cv::Mat cloneMat() const;

    // ---- Color space ----
    Image toColorSpace(ColorSpace space) const;
    Image toGrayscale() const;
    Image toBGRA() const;

    // ---- Resize ----
    Image resized(int targetWidth, int targetHeight, int interpolation = cv::INTER_LANCZOS4) const;
    Image resizedToFit(int maxDim) const;  // longest side <= maxDim
    Image downsampledForPreview(int maxPixels = 2000000) const;

    // ---- Crop / Rotate ----
    Image cropped(const Rect2i& roi) const;
    Image rotated90(int times = 1) const; // times * 90 degrees clockwise
    Image rotated(float angleDegrees, Color3u8 fillColor = {0, 0, 0}) const;

    // ---- Pixel access (unsafe but fast) ----
    uint8_t* row(int y);
    const uint8_t* row(int y) const;
    Color3u8 pixelAt(int x, int y) const;
    void setPixelAt(int x, int y, Color3u8 color);

    // ---- Deep copy ----
    Image deepCopy() const;

    // ---- Save ----
    bool save(const std::string& path, int jpegQuality = 95) const;

private:
    cv::Mat mat_;
};

} // namespace PixelForge