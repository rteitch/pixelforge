#pragma once

#include "CoreTypes.h"
#include "Image.h"

#include <string>
#include <vector>
#include <cstdint>

namespace PixelForge {

/// Image I/O module handling import/export with format validation
/// and metadata management.
class IoModule {
public:
    IoModule() = default;
    ~IoModule() = default;

    // ---- Format Detection ----
    static ImageFormat detectFormat(const std::string& filePath);
    static ImageFormat detectFormatFromBytes(const uint8_t* header, size_t headerSize);
    static std::string formatExtension(ImageFormat format);

    // ---- Import ----
    static Image loadImage(const std::string& filePath);
    static Image loadImageFromMemory(const uint8_t* data, size_t size);

    // ---- Export ----
    struct ExportOptions {
        ImageFormat format = ImageFormat::PNG;
        int jpegQuality = 95;          // 1-100
        bool stripMetadata = false;
        int pngCompression = 6;        // 0-9
        int tiffCompression = 1;       // 0=none, 1=LZW, 5=ZIP
    };

    static bool saveImage(const std::string& filePath, const Image& image,
                           const ExportOptions& options = {});

    // ---- Validation ----
    static bool isValidImageFile(const std::string& filePath);
    static bool isSupportedFormat(const std::string& filePath);

    // ---- Metadata ----
    static Size2i getImageDimensions(const std::string& filePath);

    // ---- Supported formats list ----
    static std::vector<std::string> supportedImportExtensions();
    static std::vector<std::string> supportedExportExtensions();
    static std::string supportedImportFilter();  // For file dialog
    static std::string supportedExportFilter();  // For file dialog

    // ---- Max resolution check ----
    static bool checkResolutionLimit(int width, int height, int maxDim = 8000);
};

} // namespace PixelForge