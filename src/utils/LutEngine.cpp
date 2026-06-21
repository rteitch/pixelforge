#include "LutEngine.h"
#include "core/Image.h"

#include <opencv2/imgproc.hpp>

#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <limits>

namespace PixelForge {

LutEngine::LutEngine() = default;

// ============================================================
// .cube file parser
// ============================================================

bool LutEngine::loadCubeFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    size_ = 0;
    title_.clear();
    data_.clear();
    int writeIndex = 0;

    std::string line;
    while (std::getline(file, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        if (line.empty() || line[0] == '#') continue;

        if (!parseLine(line, writeIndex)) {
            size_ = 0;
            data_.clear();
            return false;
        }
    }

    // Validate
    if (size_ <= 0 || static_cast<int>(data_.size()) != size_ * size_ * size_) {
        size_ = 0;
        data_.clear();
        return false;
    }

    return true;
}

bool LutEngine::parseLine(const std::string& line, int& writeIndex) {
    if (line.substr(0, 5) == "TITLE") {
        auto pos = line.find('"');
        if (pos != std::string::npos) {
            auto end = line.rfind('"');
            if (end > pos) {
                title_ = line.substr(pos + 1, end - pos - 1);
            }
        } else {
            auto sp = line.find(' ');
            if (sp != std::string::npos) {
                title_ = line.substr(sp + 1);
            }
        }
        return true;
    }

    if (line.substr(0, 9) == "LUT_3D_SIZE") {
        std::istringstream iss(line.substr(9));
        iss >> size_;
        if (size_ <= 0 || size_ > 256) return false;
        data_.resize(size_ * size_ * size_);
        return true;
    }

    if (line.substr(0, 13) == "DOMAIN_MIN" || line.substr(0, 13) == "DOMAIN_MAX") {
        // We accept but don't use domain bounds for now
        return true;
    }

    // Data line: R G B
    if (size_ > 0 && writeIndex < static_cast<int>(data_.size())) {
        std::istringstream iss(line);
        float r, g, b;
        if (iss >> r >> g >> b) {
            data_[writeIndex] = {r, g, b};
            writeIndex++;
            return true;
        }
    }

    // Unknown line — skip
    return true;
}

// ============================================================
// Identity LUT
// ============================================================

void LutEngine::createIdentity(int size) {
    if (size < 2) size = 2;
    size_ = size;
    title_ = "Identity";
    data_.resize(size * size * size);

    int idx = 0;
    for (int b = 0; b < size; ++b) {
        for (int g = 0; g < size; ++g) {
            for (int r = 0; r < size; ++r) {
                data_[idx++] = {
                    static_cast<float>(r) / (size - 1),
                    static_cast<float>(g) / (size - 1),
                    static_cast<float>(b) / (size - 1)
                };
            }
        }
    }
}

// ============================================================
// Trilinear interpolation
// ============================================================

std::array<float, 3> LutEngine::sample(float r, float g, float b) const {
    if (size_ <= 0) return {r, g, b};

    // Scale to grid coordinates
    float scale = static_cast<float>(size_ - 1);
    float ri = std::clamp(r, 0.0f, 1.0f) * scale;
    float gi = std::clamp(g, 0.0f, 1.0f) * scale;
    float bi = std::clamp(b, 0.0f, 1.0f) * scale;

    int r0 = static_cast<int>(ri), g0 = static_cast<int>(gi), b0 = static_cast<int>(bi);
    int r1 = std::min(r0 + 1, size_ - 1);
    int g1 = std::min(g0 + 1, size_ - 1);
    int b1 = std::min(b0 + 1, size_ - 1);

    float fr = ri - r0, fg = gi - g0, fb = bi - b0;

    // Helper to index into 3D LUT
    auto idx = [this](int ri, int gi, int bi) -> int {
        return bi * size_ * size_ + gi * size_ + ri;
    };

    // 8 corner samples
    const auto& c000 = data_[idx(r0, g0, b0)];
    const auto& c100 = data_[idx(r1, g0, b0)];
    const auto& c010 = data_[idx(r0, g1, b0)];
    const auto& c110 = data_[idx(r1, g1, b0)];
    const auto& c001 = data_[idx(r0, g0, b1)];
    const auto& c101 = data_[idx(r1, g0, b1)];
    const auto& c011 = data_[idx(r0, g1, b1)];
    const auto& c111 = data_[idx(r1, g1, b1)];

    // Trilinear interpolation
    std::array<float, 3> result;
    for (int c = 0; c < 3; ++c) {
        float c00 = c000[c] * (1 - fr) + c100[c] * fr;
        float c10 = c010[c] * (1 - fr) + c110[c] * fr;
        float c0  = c00 * (1 - fg) + c10 * fg;

        float c01 = c001[c] * (1 - fr) + c101[c] * fr;
        float c11 = c011[c] * (1 - fr) + c111[c] * fr;
        float c1  = c01 * (1 - fg) + c11 * fg;

        result[c] = c0 * (1 - fb) + c1 * fb;
    }

    return result;
}

// ============================================================
// Apply LUT
// ============================================================

void LutEngine::apply(uint8_t* pixels, int numPixels, float intensity) const {
    if (!isValid() || intensity <= 0.0f) return;

    intensity = std::clamp(intensity, 0.0f, 1.0f);
    bool blend = (intensity < 1.0f);

    for (int i = 0; i < numPixels; ++i) {
        int idx = i * 3;
        float r = pixels[idx] / 255.0f;
        float g = pixels[idx + 1] / 255.0f;
        float b = pixels[idx + 2] / 255.0f;

        auto result = sample(r, g, b);

        if (blend) {
            result[0] = r + (result[0] - r) * intensity;
            result[1] = g + (result[1] - g) * intensity;
            result[2] = b + (result[2] - b) * intensity;
        }

        pixels[idx]     = static_cast<uint8_t>(std::clamp(result[0] * 255.0f, 0.0f, 255.0f));
        pixels[idx + 1] = static_cast<uint8_t>(std::clamp(result[1] * 255.0f, 0.0f, 255.0f));
        pixels[idx + 2] = static_cast<uint8_t>(std::clamp(result[2] * 255.0f, 0.0f, 255.0f));
    }
}

Image LutEngine::applyToImage(const Image& image, float intensity) const {
    if (image.isEmpty() || !isValid()) return image;

    Image result = image.deepCopy();
    cv::Mat& mat = result.mat();
    int totalPixels = mat.rows * mat.cols;

    apply(mat.data, totalPixels, intensity);
    return result;
}

// ============================================================
// Validity
// ============================================================

bool LutEngine::isValid() const {
    return size_ > 0 && static_cast<int>(data_.size()) == size_ * size_ * size_;
}

int LutEngine::size() const {
    return size_;
}

const std::string& LutEngine::title() const {
    return title_;
}

// ============================================================
// Parametric LUT generation
// ============================================================

LutEngine LutEngine::createParametric(
    int lutSize,
    float shadowsR, float shadowsG, float shadowsB,
    float highlightsR, float highlightsG, float highlightsB,
    float gammaR, float gammaG, float gammaB,
    float saturation)
{
    if (lutSize < 2) lutSize = 2;
    LutEngine lut;
    lut.size_ = lutSize;
    lut.title_ = "Parametric";
    lut.data_.resize(lutSize * lutSize * lutSize);

    float invGammaR = (gammaR > 0.001f) ? (1.0f / gammaR) : 1.0f;
    float invGammaG = (gammaG > 0.001f) ? (1.0f / gammaG) : 1.0f;
    float invGammaB = (gammaB > 0.001f) ? (1.0f / gammaB) : 1.0f;

    float satFactor = saturation / 100.0f; // 0 = grayscale, 1 = original, 2 = oversaturated

    int idx = 0;
    for (int bi = 0; bi < lutSize; ++bi) {
        for (int gi = 0; gi < lutSize; ++gi) {
            for (int ri = 0; ri < lutSize; ++ri) {
                float r = static_cast<float>(ri) / (lutSize - 1);
                float g = static_cast<float>(gi) / (lutSize - 1);
                float b = static_cast<float>(bi) / (lutSize - 1);

                // Apply gamma curve
                r = std::pow(std::clamp(r, 0.0f, 1.0f), invGammaR);
                g = std::pow(std::clamp(g, 0.0f, 1.0f), invGammaG);
                b = std::pow(std::clamp(b, 0.0f, 1.0f), invGammaB);

                // Split toning: blend shadow/highlight colors
                float lum = 0.299f * r + 0.587f * g + 0.114f * b;

                // Shadow blend (stronger in darks)
                float shadowWeight = std::pow(1.0f - lum, 2.0f);
                float highlightWeight = std::pow(lum, 2.0f);

                r += shadowsR * shadowWeight * 0.3f + highlightsR * highlightWeight * 0.3f;
                g += shadowsG * shadowWeight * 0.3f + highlightsG * highlightWeight * 0.3f;
                b += shadowsB * shadowWeight * 0.3f + highlightsB * highlightWeight * 0.3f;

                // Saturation adjustment
                float gray = 0.299f * r + 0.587f * g + 0.114f * b;
                r = gray + (r - gray) * satFactor;
                g = gray + (g - gray) * satFactor;
                b = gray + (b - gray) * satFactor;

                lut.data_[idx++] = {
                    std::clamp(r, 0.0f, 1.0f),
                    std::clamp(g, 0.0f, 1.0f),
                    std::clamp(b, 0.0f, 1.0f)
                };
            }
        }
    }

    return lut;
}

} // namespace PixelForge