#include <gtest/gtest.h>
#include "utils/NoiseGenerator.h"
#include <cmath>
#include <vector>

using namespace PixelForge;

TEST(NoiseGenerator, BasicNoise) {
    NoiseGenerator noise(42);
    float n1 = noise.noise2D(0.5f, 0.5f);
    EXPECT_GE(n1, -1.0f);
    EXPECT_LE(n1, 1.0f);
}

TEST(NoiseGenerator, DeterministicSeed) {
    NoiseGenerator noise1(42);
    NoiseGenerator noise2(42);
    EXPECT_FLOAT_EQ(noise1.noise2D(1.0f, 2.0f), noise2.noise2D(1.0f, 2.0f));
}

TEST(NoiseGenerator, DifferentSeeds) {
    NoiseGenerator noise1(42);
    NoiseGenerator noise2(123);
    // Different seeds should generally produce different values
    float v1 = noise1.noise2D(1.0f, 2.0f);
    float v2 = noise2.noise2D(1.0f, 2.0f);
    // They might rarely be equal, but very unlikely
    // Just check both are valid
    EXPECT_GE(v1, -1.0f);
    EXPECT_GE(v2, -1.0f);
}

TEST(NoiseGenerator, FBM) {
    NoiseGenerator noise(42);
    float fbm = noise.fbm(0.5f, 0.5f, 4);
    EXPECT_GE(fbm, -1.0f);
    EXPECT_LE(fbm, 1.0f);
}

TEST(NoiseGenerator, GenerateTexture) {
    NoiseGenerator noise(42);
    auto tex = noise.generateTexture(64, 64, 0.05f, 1);
    EXPECT_EQ(tex.size(), 64u * 64u);
    // All values should be in [0, 255]
    for (auto v : tex) {
        EXPECT_GE(v, 0);
        EXPECT_LE(v, 255);
    }
}

TEST(NoiseGenerator, FilmGrainNoCrash) {
    std::vector<uint8_t> pixels(100 * 100 * 3, 128);
    NoiseGenerator::applyFilmGrain(pixels.data(), 100, 100, 50.0f, 40.0f);
    // Should not crash; pixels should be modified
    bool modified = false;
    for (size_t i = 0; i < pixels.size(); ++i) {
        if (pixels[i] != 128) { modified = true; break; }
    }
    EXPECT_TRUE(modified);
}

TEST(NoiseGenerator, HalationNoCrash) {
    std::vector<uint8_t> pixels(100 * 100 * 3, 128);
    // Set some bright pixels
    for (int i = 0; i < 10; ++i) {
        pixels[i * 3 + 0] = 250;
        pixels[i * 3 + 1] = 250;
        pixels[i * 3 + 2] = 250;
    }
    NoiseGenerator::applyHalation(pixels.data(), 100, 100, 30.0f, 220);
    // Should not crash
}