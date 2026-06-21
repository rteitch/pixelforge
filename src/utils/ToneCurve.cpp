#include "ToneCurve.h"
#include "core/Image.h"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>

namespace PixelForge {

ToneCurve::ToneCurve() {
    resetToIdentity();
}

void ToneCurve::resetToIdentity() {
    for (int ch = 0; ch < 4; ++ch) {
        points_[ch] = {{0.0f, 0.0f}, {1.0f, 1.0f}};
    }
}

void ToneCurve::setControlPoints(int channel, const std::vector<ControlPoint>& points) {
    if (channel < 0 || channel >= 4) return;
    points_[channel] = points;
    sortPoints(points_[channel]);
}

const std::vector<ToneCurve::ControlPoint>& ToneCurve::getControlPoints(int channel) const {
    static const std::vector<ControlPoint> empty;
    if (channel < 0 || channel >= 4) return empty;
    return points_[channel];
}

void ToneCurve::sortPoints(std::vector<ControlPoint>& points) const {
    std::sort(points.begin(), points.end(),
        [](const ControlPoint& a, const ControlPoint& b) {
            return a.input < b.input;
        });
}

// ============================================================
// Catmull-Rom spline interpolation
// ============================================================

float ToneCurve::splineInterpolate(
    const std::vector<ControlPoint>& points, float input) const
{
    if (points.empty()) return input;
    if (points.size() == 1) return points[0].output;
    if (input <= points.front().input) return points.front().output;
    if (input >= points.back().input) return points.back().output;

    // Find the two control points that bracket the input
    int n = static_cast<int>(points.size());
    int segIdx = 0;
    for (int i = 0; i < n - 1; ++i) {
        if (input >= points[i].input && input <= points[i + 1].input) {
            segIdx = i;
            break;
        }
    }

    // Catmull-Rom spline with surrounding points
    int i0 = std::max(0, segIdx - 1);
    int i1 = segIdx;
    int i2 = segIdx + 1;
    int i3 = std::min(n - 1, segIdx + 2);

    float denom = points[i2].input - points[i1].input;
    float t = (std::abs(denom) < 1e-10f) ? 0.0f
              : (input - points[i1].input) / denom;
    float t2 = t * t;
    float t3 = t2 * t;

    // Catmull-Rom basis functions
    float v0 = points[i0].output;
    float v1 = points[i1].output;
    float v2 = points[i2].output;
    float v3 = points[i3].output;

    float result = 0.5f * (
        (2.0f * v1) +
        (-v0 + v2) * t +
        (2.0f * v0 - 5.0f * v1 + 4.0f * v2 - v3) * t2 +
        (-v0 + 3.0f * v1 - 3.0f * v2 + v3) * t3
    );

    return std::clamp(result, 0.0f, 1.0f);
}

float ToneCurve::evaluate(int channel, float input) const {
    // Apply luma curve first
    float val = splineInterpolate(points_[3], input);

    // Then apply per-channel curve
    if (channel >= 0 && channel < 3) {
        val = splineInterpolate(points_[channel], val);
    }

    return std::clamp(val, 0.0f, 1.0f);
}

// ============================================================
// LUT generation
// ============================================================

std::array<uint8_t, 256> ToneCurve::generateLUT(int channel) const {
    std::array<uint8_t, 256> lut;
    for (int i = 0; i < 256; ++i) {
        float input = static_cast<float>(i) / 255.0f;
        float output = evaluate(channel, input);
        lut[i] = static_cast<uint8_t>(std::clamp(output * 255.0f, 0.0f, 255.0f));
    }
    return lut;
}

// ============================================================
// Apply curves
// ============================================================

void ToneCurve::apply(uint8_t* pixels, int numPixels) const {
    // Pre-build LUTs for all channels
    auto lutR = generateLUT(0);
    auto lutG = generateLUT(1);
    auto lutB = generateLUT(2);

    for (int i = 0; i < numPixels; ++i) {
        int idx = i * 3;
        pixels[idx]     = lutR[pixels[idx]];
        pixels[idx + 1] = lutG[pixels[idx + 1]];
        pixels[idx + 2] = lutB[pixels[idx + 2]];
    }
}

Image ToneCurve::applyToImage(const Image& image) const {
    if (image.isEmpty()) return image;

    Image result = image.deepCopy();
    cv::Mat& mat = result.mat();
    int totalPixels = mat.rows * mat.cols;

    apply(mat.data, totalPixels);
    return result;
}

// ============================================================
// Preset curves
// ============================================================

void ToneCurve::setContrastCurve(float amount) {
    // amount: -100 to +100, 0 = no change
    float normalized = amount / 100.0f;
    float midpoint = 0.5f;
    float shift = normalized * 0.15f;

    // S-curve: lift shadows down, push highlights up
    std::vector<ControlPoint> luma = {
        {0.0f, 0.0f},
        {0.25f, std::clamp(0.25f - shift * 0.5f, 0.0f, 1.0f)},
        {midpoint, midpoint},
        {0.75f, std::clamp(0.75f + shift * 0.5f, 0.0f, 1.0f)},
        {1.0f, 1.0f}
    };
    points_[3] = luma;
}

void ToneCurve::setBrightnessCurve(float amount) {
    // amount: -100 to +100
    float shift = amount / 100.0f * 0.3f;
    std::vector<ControlPoint> luma = {
        {0.0f, std::clamp(shift, 0.0f, 1.0f)},
        {1.0f, std::clamp(1.0f + shift, 0.0f, 1.0f)}
    };
    if (shift < 0.0f) {
        luma[0].output = 0.0f;
        luma[1].output = std::clamp(1.0f + shift, 0.0f, 1.0f);
    }
    points_[3] = luma;
}

void ToneCurve::setHighlightRolloff(float amount) {
    // Soft highlight rolloff (film response)
    float rolloff = std::clamp(amount, 0.0f, 100.0f) / 100.0f;
    float knee = 1.0f - rolloff * 0.15f;

    std::vector<ControlPoint> luma = {
        {0.0f, 0.0f},
        {knee * 0.5f, knee * 0.5f},
        {knee, knee},
        {1.0f, knee + (1.0f - knee) * 0.7f}
    };
    points_[3] = luma;
}

void ToneCurve::setShadowLift(float amount) {
    // Lift blacks
    float lift = std::clamp(amount, 0.0f, 100.0f) / 100.0f * 0.2f;

    std::vector<ControlPoint> luma = {
        {0.0f, lift},
        {0.15f, 0.15f + lift * 0.5f},
        {0.5f, 0.5f},
        {1.0f, 1.0f}
    };
    points_[3] = luma;
}

void ToneCurve::setFilmResponse(float highlightRolloff, float shadowLift, float contrast) {
    // Combine multiple curve adjustments
    resetToIdentity();

    float rolloff = std::clamp(highlightRolloff, 0.0f, 100.0f) / 100.0f;
    float lift = std::clamp(shadowLift, 0.0f, 100.0f) / 100.0f * 0.15f;
    float contrastNorm = std::clamp(contrast, -100.0f, 100.0f) / 100.0f;
    float contrastShift = contrastNorm * 0.1f;

    float knee = 1.0f - rolloff * 0.12f;

    std::vector<ControlPoint> luma = {
        {0.0f, lift},
        {0.2f, std::clamp(0.2f - contrastShift + lift * 0.3f, 0.0f, 1.0f)},
        {0.5f, std::clamp(0.5f, 0.0f, 1.0f)},
        {knee, std::clamp(knee + contrastShift, 0.0f, 1.0f)},
        {1.0f, std::clamp(knee + (1.0f - knee) * 0.75f, 0.0f, 1.0f)}
    };
    points_[3] = luma;
}

} // namespace PixelForge