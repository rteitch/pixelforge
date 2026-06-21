#include "ColorQuantization.h"
#include "core/Image.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <random>
#include <numeric>
#include <array>
#include <limits>

namespace PixelForge {

// ============================================================
// RGB <-> LAB conversion helpers
// ============================================================

void ColorQuantization::rgbToLab(uint8_t r, uint8_t g, uint8_t b,
                                  float& L, float& A, float& B) {
    // sRGB to linear
    auto srgbToLinear = [](float c) -> float {
        c /= 255.0f;
        return (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
    };

    float lr = srgbToLinear(static_cast<float>(r));
    float lg = srgbToLinear(static_cast<float>(g));
    float lb = srgbToLinear(static_cast<float>(b));

    // Linear RGB to XYZ (D65 illuminant)
    float x = 0.4124564f * lr + 0.3575761f * lg + 0.1804375f * lb;
    float y = 0.2126729f * lr + 0.7151522f * lg + 0.0721750f * lb;
    float z = 0.0193339f * lr + 0.1191920f * lg + 0.9503041f * lb;

    // D65 white point
    const float xn = 0.95047f, yn = 1.0f, zn = 1.08883f;
    x /= xn; y /= yn; z /= zn;

    auto f = [](float t) -> float {
        const float delta = 6.0f / 29.0f;
        return (t > delta * delta * delta)
            ? std::cbrt(t)
            : (t / (3.0f * delta * delta) + 4.0f / 29.0f);
    };

    float fx = f(x), fy = f(y), fz = f(z);
    L = 116.0f * fy - 16.0f;
    A = 500.0f * (fx - fy);
    B = 200.0f * (fy - fz);
}

Color3u8 ColorQuantization::labToRgb(float L, float A, float B) {
    float fy = (L + 16.0f) / 116.0f;
    float fx = A / 500.0f + fy;
    float fz = fy - B / 200.0f;

    auto finv = [](float t) -> float {
        const float delta = 6.0f / 29.0f;
        return (t > delta)
            ? (t * t * t)
            : (3.0f * delta * delta * (t - 4.0f / 29.0f));
    };

    const float xn = 0.95047f, yn = 1.0f, zn = 1.08883f;
    float x = xn * finv(fx);
    float y = yn * finv(fy);
    float z = zn * finv(fz);

    // XYZ to linear RGB
    float lr =  3.2404542f * x - 1.5371385f * y - 0.4985314f * z;
    float lg = -0.9692660f * x + 1.8760108f * y + 0.0415560f * z;
    float lb =  0.0556434f * x - 0.2040259f * y + 1.0572252f * z;

    // Clamp and apply gamma
    auto linearToSrgb = [](float c) -> uint8_t {
        c = std::clamp(c, 0.0f, 1.0f);
        float s = (c <= 0.0031308f)
            ? (c * 12.92f)
            : (1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f);
        return static_cast<uint8_t>(std::clamp(s * 255.0f, 0.0f, 255.0f));
    };

    return {linearToSrgb(lr), linearToSrgb(lg), linearToSrgb(lb)};
}

// ============================================================
// K-Means++ initialization
// ============================================================

std::vector<std::array<float, 3>> ColorQuantization::initCentroidsKpp(
    const std::vector<std::array<float, 3>>& labPixels, int k)
{
    std::mt19937 rng(42); // deterministic seed
    std::vector<std::array<float, 3>> centroids;
    int n = static_cast<int>(labPixels.size());

    // Choose first centroid randomly
    std::uniform_int_distribution<int> dist(0, n - 1);
    centroids.push_back(labPixels[dist(rng)]);

    std::vector<double> distances(n, std::numeric_limits<double>::max());

    for (int c = 1; c < k; ++c) {
        // Update distances to nearest centroid
        const auto& last = centroids.back();
        double totalDist = 0.0;
        for (int i = 0; i < n; ++i) {
            float dx = labPixels[i][0] - last[0];
            float dy = labPixels[i][1] - last[1];
            float dz = labPixels[i][2] - last[2];
            double d = static_cast<double>(dx * dx + dy * dy + dz * dz);
            if (d < distances[i]) distances[i] = d;
            totalDist += distances[i];
        }

        // Weighted random selection
        std::uniform_real_distribution<double> rdist(0.0, totalDist);
        double r = rdist(rng);
        double cum = 0.0;
        int chosen = n - 1;
        for (int i = 0; i < n; ++i) {
            cum += distances[i];
            if (cum >= r) {
                chosen = i;
                break;
            }
        }
        centroids.push_back(labPixels[chosen]);
    }

    return centroids;
}

// ============================================================
// Main quantization
// ============================================================

std::vector<Color3u8> ColorQuantization::quantize(
    const uint8_t* pixels, int width, int height,
    int k, int maxIterations)
{
    int totalPixels = width * height;
    if (totalPixels == 0 || k <= 0) return {};

    k = std::clamp(k, 2, 32);

    // Sample pixels for speed (max 50000)
    const int maxSamples = 50000;
    int stride = std::max(1, totalPixels / maxSamples);

    std::vector<std::array<float, 3>> labPixels;
    labPixels.reserve(std::min(totalPixels, maxSamples));

    for (int i = 0; i < totalPixels; i += stride) {
        int idx = i * 3;
        float L, A, B;
        rgbToLab(pixels[idx], pixels[idx + 1], pixels[idx + 2], L, A, B);
        labPixels.push_back({L, A, B});
    }

    int n = static_cast<int>(labPixels.size());

    // Initialize centroids with k-means++
    auto centroids = initCentroidsKpp(labPixels, k);
    std::vector<int> labels(n, 0);

    // K-means iterations
    for (int iter = 0; iter < maxIterations; ++iter) {
        bool changed = false;

        // Assign each pixel to nearest centroid
        for (int i = 0; i < n; ++i) {
            float bestDist = std::numeric_limits<float>::max();
            int bestIdx = 0;
            for (int c = 0; c < k; ++c) {
                float dx = labPixels[i][0] - centroids[c][0];
                float dy = labPixels[i][1] - centroids[c][1];
                float dz = labPixels[i][2] - centroids[c][2];
                float dist = dx * dx + dy * dy + dz * dz;
                if (dist < bestDist) {
                    bestDist = dist;
                    bestIdx = c;
                }
            }
            if (labels[i] != bestIdx) {
                labels[i] = bestIdx;
                changed = true;
            }
        }

        if (!changed) break;

        // Recompute centroids
        std::vector<std::array<double, 3>> sums(k, {0.0, 0.0, 0.0});
        std::vector<int> counts(k, 0);

        for (int i = 0; i < n; ++i) {
            int c = labels[i];
            sums[c][0] += labPixels[i][0];
            sums[c][1] += labPixels[i][1];
            sums[c][2] += labPixels[i][2];
            counts[c]++;
        }

        for (int c = 0; c < k; ++c) {
            if (counts[c] > 0) {
                centroids[c][0] = static_cast<float>(sums[c][0] / counts[c]);
                centroids[c][1] = static_cast<float>(sums[c][1] / counts[c]);
                centroids[c][2] = static_cast<float>(sums[c][2] / counts[c]);
            }
        }
    }

    // Convert centroids back to RGB
    std::vector<Color3u8> palette;
    palette.reserve(k);
    for (int c = 0; c < k; ++c) {
        palette.push_back(labToRgb(centroids[c][0], centroids[c][1], centroids[c][2]));
    }

    return palette;
}

std::vector<Color3u8> ColorQuantization::quantizeImage(
    const Image& image, int k, int maxIterations)
{
    if (image.isEmpty()) return {};
    cv::Mat rgb = image.mat();
    if (rgb.channels() != 3) {
        cv::Mat temp;
        cv::cvtColor(rgb, temp, cv::COLOR_GRAY2RGB);
        rgb = temp;
    }
    return quantize(rgb.data, rgb.cols, rgb.rows, k, maxIterations);
}

Image ColorQuantization::applyPalette(
    const Image& image,
    const std::vector<Color3u8>& palette)
{
    if (image.isEmpty() || palette.empty()) return image;

    cv::Mat src = image.mat();
    cv::Mat result = src.clone();
    int totalPixels = src.rows * src.cols;
    int ch = src.channels();

    auto labels = assignLabels(src.data, totalPixels, ch, palette);

    for (int i = 0; i < totalPixels; ++i) {
        const auto& color = palette[labels[i]];
        uint8_t* p = result.data + i * ch;
        p[0] = color.r;
        p[1] = color.g;
        p[2] = color.b;
    }

    return Image(result);
}

std::vector<int> ColorQuantization::assignLabels(
    const uint8_t* pixels, int numPixels, int channels,
    const std::vector<Color3u8>& palette)
{
    std::vector<int> labels(numPixels, 0);
    if (palette.empty()) return labels;

    // Build LAB lookup for palette
    std::vector<std::array<float, 3>> paletteLab(palette.size());
    for (size_t c = 0; c < palette.size(); ++c) {
        rgbToLab(palette[c].r, palette[c].g, palette[c].b,
                 paletteLab[c][0], paletteLab[c][1], paletteLab[c][2]);
    }

    for (int i = 0; i < numPixels; ++i) {
        const uint8_t* p = pixels + i * channels;
        float L, A, B;
        rgbToLab(p[0], p[1], p[2], L, A, B);

        float bestDist = std::numeric_limits<float>::max();
        int bestIdx = 0;
        for (size_t c = 0; c < palette.size(); ++c) {
            float dx = L - paletteLab[c][0];
            float dy = A - paletteLab[c][1];
            float dz = B - paletteLab[c][2];
            float dist = dx * dx + dy * dy + dz * dz;
            if (dist < bestDist) {
                bestDist = dist;
                bestIdx = static_cast<int>(c);
            }
        }
        labels[i] = bestIdx;
    }

    return labels;
}

Color3u8 ColorQuantization::meanColor(
    const uint8_t* pixels, int channels,
    const std::vector<int>& indices)
{
    if (indices.empty()) return {0, 0, 0};

    uint64_t sumR = 0, sumG = 0, sumB = 0;
    for (int idx : indices) {
        const uint8_t* p = pixels + idx * channels;
        sumR += p[0];
        sumG += p[1];
        sumB += p[2];
    }
    int n = static_cast<int>(indices.size());
    return {
        static_cast<uint8_t>(sumR / n),
        static_cast<uint8_t>(sumG / n),
        static_cast<uint8_t>(sumB / n)
    };
}

// ============================================================
// Preset palettes
// ============================================================

std::vector<Color3u8> ColorQuantization::vibrantPalette(int k) {
    // High-saturation, high-contrast palette
    std::vector<Color3u8> base = {
        {255, 0, 0},     // Red
        {0, 180, 0},     // Green
        {0, 80, 255},    // Blue
        {255, 220, 0},   // Yellow
        {255, 120, 0},   // Orange
        {180, 0, 255},   // Purple
        {0, 220, 220},   // Cyan
        {255, 0, 180},   // Magenta
        {0, 0, 0},       // Black
        {255, 255, 255}, // White
        {120, 80, 40},   // Brown
        {0, 120, 80},    // Teal
        {255, 180, 180}, // Pink
        {80, 255, 0},    // Lime
        {0, 0, 180},     // Navy
        {255, 160, 60},  // Amber
    };
    if (k >= static_cast<int>(base.size())) return base;
    return std::vector<Color3u8>(base.begin(), base.begin() + k);
}

std::vector<Color3u8> ColorQuantization::pastelPalette(int k) {
    std::vector<Color3u8> base = {
        {255, 180, 180}, // Pastel Red
        {180, 255, 180}, // Pastel Green
        {180, 180, 255}, // Pastel Blue
        {255, 255, 180}, // Pastel Yellow
        {255, 200, 160}, // Pastel Orange
        {220, 180, 255}, // Pastel Purple
        {180, 255, 255}, // Pastel Cyan
        {255, 180, 220}, // Pastel Pink
        {240, 240, 240}, // Off-white
        {200, 200, 200}, // Light Gray
        {220, 200, 180}, // Pastel Beige
        {180, 220, 200}, // Pastel Mint
        {200, 180, 200}, // Pastel Lavender
        {255, 220, 200}, // Pastel Peach
        {200, 255, 220}, // Pastel Seafoam
        {220, 220, 255}, // Pastel Periwinkle
    };
    if (k >= static_cast<int>(base.size())) return base;
    return std::vector<Color3u8>(base.begin(), base.begin() + k);
}

std::vector<Color3u8> ColorQuantization::monochromeAccentPalette(int k) {
    // Grayscale base + 2-3 accent colors
    std::vector<Color3u8> base = {
        {0, 0, 0},
        {30, 30, 30},
        {60, 60, 60},
        {90, 90, 90},
        {120, 120, 120},
        {150, 150, 150},
        {180, 180, 180},
        {210, 210, 210},
        {240, 240, 240},
        {255, 255, 255},
        {220, 40, 40},   // Red accent
        {40, 120, 220},  // Blue accent
        {40, 200, 100},  // Green accent
        {220, 180, 40},  // Gold accent
        {160, 60, 200},  // Purple accent
        {220, 120, 60},  // Warm accent
    };
    if (k >= static_cast<int>(base.size())) return base;
    return std::vector<Color3u8>(base.begin(), base.begin() + k);
}

} // namespace PixelForge