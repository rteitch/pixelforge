#include <gtest/gtest.h>
#include "utils/LutEngine.h"

using namespace PixelForge;

TEST(LutEngine, IdentityLUT) {
    LutEngine lut;
    lut.createIdentity(17);
    EXPECT_TRUE(lut.isValid());
    EXPECT_EQ(lut.size(), 17);
    EXPECT_EQ(lut.title(), "Identity");

    // Identity LUT should return same values
    uint8_t pixels[] = {128, 64, 200};
    lut.apply(pixels, 1);
    EXPECT_NEAR(pixels[0], 128, 2);
    EXPECT_NEAR(pixels[1], 64, 2);
    EXPECT_NEAR(pixels[2], 200, 2);
}

TEST(LutEngine, ParametricLUT) {
    LutEngine lut = LutEngine::createParametric(17,
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
        100.0f); // neutral
    EXPECT_TRUE(lut.isValid());
}

TEST(LutEngine, IntensityBlending) {
    LutEngine lut = LutEngine::createParametric(17,
        0.5f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f,
        100.0f);

    uint8_t pixels[] = {128, 128, 128};
    lut.apply(pixels, 1, 0.5f); // 50% intensity
    // At 50% intensity, effect should be blended
    // Value should be between identity and full LUT
    EXPECT_GE(pixels[0], 128); // red should increase due to shadow boost
}

TEST(LutEngine, InvalidLUT) {
    LutEngine lut;
    EXPECT_FALSE(lut.isValid());
    EXPECT_EQ(lut.size(), 0);

    uint8_t pixels[] = {128, 64, 200};
    lut.apply(pixels, 1); // Should not crash
    EXPECT_EQ(pixels[0], 128); // Should remain unchanged
}