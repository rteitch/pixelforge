#include <gtest/gtest.h>
#include "utils/ToneCurve.h"

using namespace PixelForge;

TEST(ToneCurve, IdentityCurve) {
    ToneCurve curve;
    // Default is identity
    auto lut = curve.generateLUT(3); // luma channel
    for (int i = 0; i < 256; ++i) {
        EXPECT_EQ(lut[i], static_cast<uint8_t>(i));
    }
}

TEST(ToneCurve, ContrastCurve) {
    ToneCurve curve;
    curve.setContrastCurve(50.0f); // positive contrast
    auto lut = curve.generateLUT(3);
    // Midtones should shift: darks darker, lights lighter
    EXPECT_LT(lut[64], 64);    // darks get darker
    EXPECT_GT(lut[192], 192);  // lights get lighter
}

TEST(ToneCurve, BrightnessCurve) {
    ToneCurve curve;
    curve.setBrightnessCurve(50.0f);
    auto lut = curve.generateLUT(3);
    // Overall brightness should increase
    EXPECT_GE(lut[128], 128);
}

TEST(ToneCurve, ShadowLift) {
    ToneCurve curve;
    curve.setShadowLift(50.0f);
    auto lut = curve.generateLUT(3);
    // Blacks should be lifted
    EXPECT_GT(lut[0], 0);
    // Highlights should remain close to original
    EXPECT_NEAR(lut[255], 255, 10);
}

TEST(ToneCurve, ResetToIdentity) {
    ToneCurve curve;
    curve.setContrastCurve(80.0f);
    curve.resetToIdentity();
    auto lut = curve.generateLUT(3);
    for (int i = 0; i < 256; ++i) {
        EXPECT_EQ(lut[i], static_cast<uint8_t>(i));
    }
}

TEST(ToneCurve, FilmResponse) {
    ToneCurve curve;
    curve.setFilmResponse(60.0f, 20.0f, 30.0f);
    auto lut = curve.generateLUT(3);
    // Should produce a valid curve (no crashes, values in range)
    for (int i = 0; i < 256; ++i) {
        EXPECT_GE(lut[i], 0);
        EXPECT_LE(lut[i], 255);
    }
}

TEST(ToneCurve, ApplyToPixels) {
    ToneCurve curve;
    curve.setContrastCurve(30.0f);
    std::vector<uint8_t> pixels = {128, 128, 128, 64, 64, 64};
    curve.apply(pixels.data(), 2);
    // Pixels should be modified
    EXPECT_NE(pixels[0], 128);
}