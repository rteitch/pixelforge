#pragma once

#include "core/CoreTypes.h"

#include <vector>
#include <array>
#include <cstdint>

namespace PixelForge {

/// Parametric tone curve with spline interpolation.
/// Supports per-channel (R, G, B, Luma) adjustments.
class ToneCurve {
public:
    ToneCurve();
    ~ToneCurve() = default;

    /// Control point: input [0,1] -> output [0,1]
    struct ControlPoint {
        float input = 0.0f;
        float output = 0.0f;
    };

    /// Set control points for a specific channel (0=R, 1=G, 2=B, 3=Luma)
    void setControlPoints(int channel, const std::vector<ControlPoint>& points);

    /// Get control points for a channel
    const std::vector<ControlPoint>& getControlPoints(int channel) const;

    /// Reset to identity (linear) curve for all channels
    void resetToIdentity();

    /// Generate lookup table for a channel (256 entries, uint8_t output)
    std::array<uint8_t, 256> generateLUT(int channel) const;

    /// Apply all curves to RGB pixel data
    /// @param pixels RGB data (modified in-place)
    /// @param numPixels Number of pixels
    void apply(uint8_t* pixels, int numPixels) const;

    /// Apply curve to image
    class Image applyToImage(const class Image& image) const;

    // ---- Common curve presets ----
    /// S-curve for contrast boost
    void setContrastCurve(float amount);

    /// Brightness shift
    void setBrightnessCurve(float amount);

    /// Highlight rolloff (film-like response)
    void setHighlightRolloff(float amount);

    /// Shadow lift
    void setShadowLift(float amount);

    /// Combined film response curve
    void setFilmResponse(float highlightRolloff, float shadowLift, float contrast);

private:
    // Channels: 0=R, 1=G, 2=B, 3=Luma
    std::vector<ControlPoint> points_[4];

    /// Catmull-Rom spline interpolation
    float splineInterpolate(const std::vector<ControlPoint>& points, float input) const;

    /// Ensure control points are sorted by input
    void sortPoints(std::vector<ControlPoint>& points) const;

    /// Build final LUT considering luma + per-channel adjustments
    float evaluate(int channel, float input) const;
};

} // namespace PixelForge