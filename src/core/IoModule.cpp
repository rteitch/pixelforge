#include "IoModule.h"

#include <opencv2/imgcodecs.hpp>

#include <fstream>
#include <algorithm>
#include <filesystem>

namespace PixelForge {

namespace fs = std::filesystem;

// ============================================================
// Format Detection
// ============================================================

ImageFormat IoModule::detectFormat(const std::string& filePath) {
    auto headerBytes = [](const std::string& path) -> std::array<uint8_t, 16> {
        std::array<uint8_t, 16> header{};
        std::ifstream f(path, std::ios::binary);
        if (f.is_open()) {
            f.read(reinterpret_cast<char*>(header.data()), header.size());
        }
        return header;
    };

    auto header = headerBytes(filePath);
    return detectFormatFromBytes(header.data(), header.size());
}

ImageFormat IoModule::detectFormatFromBytes(const uint8_t* header, size_t headerSize) {
    if (headerSize < 4) return ImageFormat::Unknown;

    // JPEG: FF D8 FF
    if (header[0] == 0xFF && header[1] == 0xD8 && header[2] == 0xFF)
        return ImageFormat::JPEG;

    // PNG: 89 50 4E 47
    if (header[0] == 0x89 && header[1] == 0x50 && header[2] == 0x4E && header[3] == 0x47)
        return ImageFormat::PNG;

    // BMP: 42 4D
    if (header[0] == 0x42 && header[1] == 0x4D)
        return ImageFormat::BMP;

    // TIFF: 49 49 (little-endian) or 4D 4D (big-endian)
    if ((header[0] == 0x49 && header[1] == 0x49) ||
        (header[0] == 0x4D && header[1] == 0x4D))
        return ImageFormat::TIFF;

    // WebP: RIFF....WEBP
    if (header[0] == 0x52 && header[1] == 0x49 && header[2] == 0x46 && header[3] == 0x46 &&
        headerSize >= 12 &&
        header[8] == 0x57 && header[9] == 0x45 && header[10] == 0x42 && header[11] == 0x50)
        return ImageFormat::WebP;

    return ImageFormat::Unknown;
}

std::string IoModule::formatExtension(ImageFormat format) {
    switch (format) {
        case ImageFormat::JPEG: return ".jpg";
        case ImageFormat::PNG:  return ".png";
        case ImageFormat::BMP:  return ".bmp";
        case ImageFormat::TIFF: return ".tiff";
        case ImageFormat::WebP: return ".webp";
        default: return "";
    }
}

// ============================================================
// Import
// ============================================================

Image IoModule::loadImage(const std::string& filePath) {
    if (!fs::exists(filePath)) {
        throw std::runtime_error("File does not exist: " + filePath);
    }

    if (!isValidImageFile(filePath)) {
        throw std::runtime_error("Invalid or unsupported image file: " + filePath);
    }

    return Image::fromFile(filePath);
}

Image IoModule::loadImageFromMemory(const uint8_t* data, size_t size) {
    return Image::fromBuffer(data, size);
}

// ============================================================
// Export
// ============================================================

bool IoModule::saveImage(const std::string& filePath, const Image& image,
                          const ExportOptions& options) {
    if (image.isEmpty()) return false;

    // Ensure parent directory exists
    fs::path p(filePath);
    if (p.has_parent_path()) {
        std::error_code ec;
        fs::create_directories(p.parent_path(), ec);
    }

    // Use Image::save which handles format detection and params
    return image.save(filePath, options.jpegQuality);
}

// ============================================================
// Validation
// ============================================================

bool IoModule::isValidImageFile(const std::string& filePath) {
    if (!fs::exists(filePath)) return false;

    // Check file size (reject empty or extremely large files)
    auto fileSize = fs::file_size(filePath);
    if (fileSize == 0 || fileSize > 500 * 1024 * 1024) return false; // max 500MB

    // Validate magic bytes
    ImageFormat fmt = detectFormat(filePath);
    return fmt != ImageFormat::Unknown;
}

bool IoModule::isSupportedFormat(const std::string& filePath) {
    ImageFormat fmt = detectFormat(filePath);
    return fmt != ImageFormat::Unknown;
}

// ============================================================
// Metadata
// ============================================================

Size2i IoModule::getImageDimensions(const std::string& filePath) {
    // Read just the header to get dimensions without loading the full image
    cv::Mat img = cv::imread(filePath, cv::IMREAD_UNCHANGED);
    if (img.empty()) return {0, 0};
    return {img.cols, img.rows};
}

// ============================================================
// Supported formats
// ============================================================

std::vector<std::string> IoModule::supportedImportExtensions() {
    return {".jpg", ".jpeg", ".png", ".bmp", ".tiff", ".tif", ".webp"};
}

std::vector<std::string> IoModule::supportedExportExtensions() {
    return {".jpg", ".jpeg", ".png", ".tiff", ".tif"};
}

std::string IoModule::supportedImportFilter() {
    return "Images (*.jpg *.jpeg *.png *.bmp *.tiff *.tif *.webp);;All Files (*)";
}

std::string IoModule::supportedExportFilter() {
    return "PNG (*.png);;JPEG (*.jpg *.jpeg);;TIFF (*.tiff *.tif);;All Files (*)";
}

// ============================================================
// Resolution check
// ============================================================

bool IoModule::checkResolutionLimit(int width, int height, int maxDim) {
    return (width <= maxDim && height <= maxDim);
}

} // namespace PixelForge