#pragma once

#include "core/CoreTypes.h"

#include <cstdint>
#include <vector>

namespace PixelForge {

/// Procedural noise generator for film grain, texture effects.
/// Uses simplex-like noise with luminance-weighted application.
class NoiseGenerator {
public:
    NoiseGenerator();
    explicit NoiseGenerator(uint32_t seed);
    ~NoiseGenerator() = default;

    /// Set random seed for reproducible noise
    void setSeed(uint32_t seed);

    /// Generate a single noise value in [-1, 1] at (x, y)
    float noise2D(float x, float y) const;

    /// Generate fractal Brownian motion (fBm) noise
    float fbm(float x, float y, int octaves = 4, float lacunarity = 2.0f, float gain = 0.5f) const;

    /// Generate a full noise texture (normalized to [0, 255])
    /// @param width Texture width
    /// @param height Texture height
    /// @param scale Noise scale (higher = finer detail)
    /// @param octaves Number of octaves for fBm
    std::vector<uint8_t> generateTexture(int width, int height,
                                           float scale = 0.02f,
                                           int octaves = 1) const;

    /// Apply film grain to an image.
    /// Grain intensity is weighted by luminance (more visible in shadows).
    /// @param pixels RGB pixel data (modified in-place)
    /// @param width Image width
    /// @param height Image height
    /// @param intensity Grain intensity (0-100)
    /// @param grainSize Grain size/scale (0-100, maps to pixel scale)
    static void applyFilmGrain(uint8_t* pixels, int width, int height,
                                float intensity, float grainSize);

    /// Apply halation (bloom around highlights)
    /// @param pixels RGB pixel data (modified in-place)
    /// @param width Image width
    /// @param height Image height
    /// @param strength Halation strength (0-100)
    /// @param threshold Brightness threshold for halation trigger
    static void applyHalation(uint8_t* pixels, int width, int height,
                               float strength, uint8_t threshold = 220);

private:
    // Permutation table for noise
    std::vector<int> perm_;
    uint32_t seed_ = 0;

    void initPermTable();

    // Gradient vectors
    static float grad2D(int hash, float x, float y);

    // Helper for noise
    static int fastFloor(float x);
    static float dot2(int g, float x, float y);
};

} // namespace PixelForge