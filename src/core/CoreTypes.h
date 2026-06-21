#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <optional>
#include <functional>
#include <unordered_map>

// OpenCV forward declarations
namespace cv {
    class Mat;
}

namespace PixelForge {

// ============================================================
// Basic types
// ============================================================
struct Point2f {
    float x = 0.f, y = 0.f;
};

struct Point2i {
    int x = 0, y = 0;
};

struct Size2i {
    int width = 0, height = 0;
};

struct Rect2i {
    int x = 0, y = 0, width = 0, height = 0;
};

struct Color3f {
    float r = 0.f, g = 0.f, b = 0.f;
};

struct Color4f {
    float r = 0.f, g = 0.f, b = 0.f, a = 1.f;
};

struct Color3u8 {
    uint8_t r = 0, g = 0, b = 0;
};

// ============================================================
// Enums
// ============================================================
enum class ImageFormat {
    JPEG,
    PNG,
    BMP,
    TIFF,
    WebP,
    Unknown
};

enum class ColorSpace {
    sRGB,
    LinearRGB,
    LAB,
    HSV,
    YCrCb
};

enum class WapDetailLevel {
    Low = 0,
    Medium = 1,
    High = 2,
    Custom = 3
};

enum class WapPalettePreset {
    Vibrant = 0,
    Pastel = 1,
    MonochromeAccent = 2,
    Custom = 3
};

enum class FilterCategory {
    Cinematic,
    JapanStyle,
    VintageRetro,
    Monochrome,
    Other
};

// ============================================================
// Preset parameter ranges
// ============================================================
struct FilterParameters {
    float intensity = 100.f;       // 0-100
    float grainAmount = 0.f;       // 0-100
    float vignetteStrength = 0.f;  // 0-100
    float temperature = 0.f;       // -100 to +100
    float tint = 0.f;              // -100 to +100
    float contrast = 0.f;          // -100 to +100
    float brightness = 0.f;        // -100 to +100
    float saturation = 0.f;        // -100 to +100
    float highlights = 0.f;        // -100 to +100
    float shadows = 0.f;           // -100 to +100
};

struct WapParameters {
    int colorCount = 16;           // 6-32
    WapDetailLevel detailLevel = WapDetailLevel::Medium;
    int customPointCount = 2000;   // used when detail == Custom
    WapPalettePreset palettePreset = WapPalettePreset::Vibrant;
    std::vector<Color3u8> customPalette; // used when palette == Custom
    bool faceDetectionEnabled = true;
    float faceDetailBoost = 1.5f;  // multiplier for point density in face region
};

// ============================================================
// Preset definition
// ============================================================
struct PresetInfo {
    std::string id;
    std::string name;
    FilterCategory category;
    std::string description;
    std::string lutFilePath;       // .cube file path (empty if parametric only)
    FilterParameters defaultParams;
    bool isBuiltIn = true;
    bool isFavorite = false;
};

// ============================================================
// Triangle (used by WPAP)
// ============================================================
struct Triangle {
    Point2f vertices[3];
    Color3u8 fillColor;
};

// ============================================================
// Batch job
// ============================================================
struct BatchJobConfig {
    std::vector<std::string> inputPaths;
    std::string outputDirectory;
    std::string outputFormat = "png";   // "jpg", "png", "tiff"
    int jpegQuality = 95;
    std::string namingPattern = "{name}_{preset}"; // {name}, {preset}, {index}
    std::string presetId;
    FilterParameters filterParams;
    WapParameters wapParams;
    bool useWapMode = false;
};

enum class BatchJobStatus {
    Pending,
    Running,
    Completed,
    Failed,
    Cancelled
};

struct BatchJobResult {
    std::string inputPath;
    std::string outputPath;
    BatchJobStatus status = BatchJobStatus::Pending;
    std::string errorMessage;
    double processingTimeMs = 0.0;
};

// ============================================================
// Project file (.pforge)
// ============================================================
struct AdjustmentLayer {
    std::string id;
    std::string name;
    bool enabled = true;
    std::string presetId;
    FilterParameters params;
    WapParameters wapParams;
    bool isWapMode = false;
    float opacity = 100.f;         // 0-100
};

struct ProjectData {
    std::string version = "1.0";
    std::string sourceImagePath;
    std::vector<AdjustmentLayer> layers;
    std::string lastExportPath;
};

// ============================================================
// Callbacks
// ============================================================
using ProgressCallback = std::function<void(float progress, const std::string& message)>;
using ImageReadyCallback = std::function<void()>;

} // namespace PixelForge