#include "NoiseGenerator.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>

namespace PixelForge {

// ============================================================
// Construction
// ============================================================

NoiseGenerator::NoiseGenerator() {
    setSeed(12345);
}

NoiseGenerator::NoiseGenerator(uint32_t seed) {
    setSeed(seed);
}

void NoiseGenerator::setSeed(uint32_t seed) {
    seed_ = seed;
    initPermTable();
}

void NoiseGenerator::initPermTable() {
    perm_.resize(512);
    std::vector<int> p(256);
    std::iota(p.begin(), p.end(), 0);

    std::mt19937 rng(seed_);
    std::shuffle(p.begin(), p.end(), rng);

    for (int i = 0; i < 256; ++i) {
        perm_[i] = p[i];
        perm_[i + 256] = p[i];
    }
}

// ============================================================
// Noise math helpers
// ============================================================

int NoiseGenerator::fastFloor(float x) {
    int xi = static_cast<int>(x);
    return (x < xi) ? (xi - 1) : xi;
}

float NoiseGenerator::grad2D(int hash, float x, float y) {
    int h = hash & 7;      // Use 8 gradient directions
    float u = (h < 4) ? x : y;
    float v = (h < 4) ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}

float NoiseGenerator::dot2(int g, float x, float y) {
    return grad2D(g, x, y);
}

// ============================================================
// 2D Simplex-like noise
// ============================================================

float NoiseGenerator::noise2D(float x, float y) const {
    // Skewing factors for 2D
    const float F2 = 0.366025403f; // (sqrt(3)-1)/2
    const float G2 = 0.211324865f; // (3-sqrt(3))/6

    float s = (x + y) * F2;
    int i = fastFloor(x + s);
    int j = fastFloor(y + s);

    float t = (i + j) * G2;
    float x0 = x - (i - t);
    float y0 = y - (j - t);

    int i1, j1;
    if (x0 > y0) { i1 = 1; j1 = 0; }
    else         { i1 = 0; j1 = 1; }

    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    int ii = i & 255;
    int jj = j & 255;
    int gi0 = perm_[ii + perm_[jj]] % 8;
    int gi1 = perm_[ii + i1 + perm_[jj + j1]] % 8;
    int gi2 = perm_[ii + 1 + perm_[jj + 1]] % 8;

    float n0 = 0.0f, n1 = 0.0f, n2 = 0.0f;

    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 >= 0.0f) {
        t0 *= t0;
        n0 = t0 * t0 * grad2D(gi0, x0, y0);
    }

    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 >= 0.0f) {
        t1 *= t1;
        n1 = t1 * t1 * grad2D(gi1, x1, y1);
    }

    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 >= 0.0f) {
        t2 *= t2;
        n2 = t2 * t2 * grad2D(gi2, x2, y2);
    }

    // Scale to [-1, 1]
    return 70.0f * (n0 + n1 + n2);
}

// ============================================================
// Fractal Brownian motion
// ============================================================

float NoiseGenerator::fbm(float x, float y, int octaves, float lacunarity, float gain) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; ++i) {
        value += amplitude * noise2D(x * frequency, y * frequency);
        maxValue += amplitude;
        amplitude *= gain;
        frequency *= lacunarity;
    }

    return value / maxValue; // Normalize to [-1, 1]
}

// ============================================================
// Texture generation
// ============================================================

std::vector<uint8_t> NoiseGenerator::generateTexture(
    int width, int height, float scale, int octaves) const
{
    std::vector<uint8_t> texture(width * height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            float val = fbm(x * scale, y * scale, octaves);
            val = (val + 1.0f) * 0.5f; // Map [-1,1] to [0,1]
            texture[y * width + x] = static_cast<uint8_t>(std::clamp(val * 255.0f, 0.0f, 255.0f));
        }
    }
    return texture;
}

// ============================================================
// Film grain application
// ============================================================

void NoiseGenerator::applyFilmGrain(uint8_t* pixels, int width, int height,
                                     float intensity, float grainSize) {
    if (intensity <= 0.0f) return;

    intensity = std::clamp(intensity, 0.0f, 100.0f) / 100.0f;

    // Map grainSize (0-100) to noise scale (fine to coarse)
    float scale = 0.1f + (1.0f - grainSize / 100.0f) * 0.3f;

    NoiseGenerator noise(42); // Fixed seed for deterministic grain
    int totalPixels = width * height;

    for (int i = 0; i < totalPixels; ++i) {
        int px = i % width;
        int py = i / width;
        int idx = i * 3;

        // Compute luminance (0-255)
        float lum = 0.299f * pixels[idx] + 0.587f * pixels[idx + 1] + 0.114f * pixels[idx + 2];

        // Grain is more visible in shadows (film characteristic)
        float lumWeight = 1.0f - (lum / 255.0f) * 0.5f; // 0.5 at white, 1.0 at black

        // Generate noise value
        float n = noise.fbm(px * scale, py * scale, 2);
        float grain = n * intensity * 40.0f * lumWeight; // Max ~40 intensity units

        // Apply grain to each channel
        for (int c = 0; c < 3; ++c) {
            float val = pixels[idx + c] + grain;
            pixels[idx + c] = static_cast<uint8_t>(std::clamp(val, 0.0f, 255.0f));
        }
    }
}

// ============================================================
// Halation (bloom around highlights)
// ============================================================

void NoiseGenerator::applyHalation(uint8_t* pixels, int width, int height,
                                    float strength, uint8_t threshold) {
    if (strength <= 0.0f) return;

    strength = std::clamp(strength, 0.0f, 100.0f) / 100.0f;

    // Step 1: Create highlight mask
    std::vector<float> mask(width * height, 0.0f);
    for (int i = 0; i < width * height; ++i) {
        int idx = i * 3;
        float lum = 0.299f * pixels[idx] + 0.587f * pixels[idx + 1] + 0.114f * pixels[idx + 2];
        if (lum > threshold) {
            mask[i] = (lum - threshold) / (255.0f - threshold);
        }
    }

    // Step 2: Gaussian blur the mask (simple box blur approximation)
    int blurRadius = std::max(3, std::min(width, height) / 20);
    std::vector<float> blurred = mask;

    // Horizontal pass
    for (int y = 0; y < height; ++y) {
        float sum = 0.0f;
        int count = 0;
        for (int x = -blurRadius; x <= blurRadius; ++x) {
            int sx = std::clamp(x, 0, width - 1);
            sum += mask[y * width + sx];
            count++;
        }
        for (int x = 0; x < width; ++x) {
            blurred[y * width + x] = sum / count;
            int removeX = std::clamp(x - blurRadius, 0, width - 1);
            int addX = std::clamp(x + blurRadius + 1, 0, width - 1);
            sum -= mask[y * width + removeX];
            sum += mask[y * width + addX];
        }
    }

    // Vertical pass
    std::vector<float> blurred2 = blurred;
    for (int x = 0; x < width; ++x) {
        float sum = 0.0f;
        int count = 0;
        for (int y = -blurRadius; y <= blurRadius; ++y) {
            int sy = std::clamp(y, 0, height - 1);
            sum += blurred[sy * width + x];
            count++;
        }
        for (int y = 0; y < height; ++y) {
            blurred2[y * width + x] = sum / count;
            int removeY = std::clamp(y - blurRadius, 0, height - 1);
            int addY = std::clamp(y + blurRadius + 1, 0, height - 1);
            sum -= blurred[removeY * width + x];
            sum += blurred[addY * width + x];
        }
    }

    // Step 3: Apply halation as a warm glow (slightly reddish-orange tint)
    for (int i = 0; i < width * height; ++i) {
        float halationAmount = blurred2[i] * strength * 60.0f;
        if (halationAmount < 0.5f) continue;

        int idx = i * 3;
        // Warm halation tint
        float r = pixels[idx]     + halationAmount * 1.2f;
        float g = pixels[idx + 1] + halationAmount * 0.9f;
        float b = pixels[idx + 2] + halationAmount * 0.6f;

        pixels[idx]     = static_cast<uint8_t>(std::clamp(r, 0.0f, 255.0f));
        pixels[idx + 1] = static_cast<uint8_t>(std::clamp(g, 0.0f, 255.0f));
        pixels[idx + 2] = static_cast<uint8_t>(std::clamp(b, 0.0f, 255.0f));
    }
}

} // namespace PixelForge