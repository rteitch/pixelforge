#include <gtest/gtest.h>
#include "utils/ColorQuantization.h"
#include "core/CoreTypes.h"

using namespace PixelForge;

TEST(ColorQuantization, VibrantPaletteSize) {
    auto palette = ColorQuantization::vibrantPalette(8);
    EXPECT_EQ(palette.size(), 8u);
}

TEST(ColorQuantization, PastelPaletteSize) {
    auto palette = ColorQuantization::pastelPalette(16);
    EXPECT_EQ(palette.size(), 16u);
}

TEST(ColorQuantization, MonochromeAccentPalette) {
    auto palette = ColorQuantization::monochromeAccentPalette(12);
    EXPECT_EQ(palette.size(), 12u);
}

TEST(ColorQuantization, QuantizeSimpleImage) {
    // Create a simple 10x10 red image
    std::vector<uint8_t> pixels(10 * 10 * 3, 0);
    for (int i = 0; i < 100; ++i) {
        pixels[i * 3 + 0] = 255; // R
        pixels[i * 3 + 1] = 0;   // G
        pixels[i * 3 + 2] = 0;   // B
    }

    auto palette = ColorQuantization::quantize(pixels.data(), 10, 10, 4);
    EXPECT_EQ(palette.size(), 4u);

    // The dominant color should be close to red
    bool foundRed = false;
    for (const auto& c : palette) {
        if (c.r > 200 && c.g < 50 && c.b < 50) {
            foundRed = true;
            break;
        }
    }
    EXPECT_TRUE(foundRed);
}

TEST(ColorQuantization, AssignLabels) {
    std::vector<Color3u8> palette = {{0, 0, 0}, {255, 255, 255}};
    std::vector<uint8_t> pixels = {0, 0, 0, 255, 255, 255, 128, 128, 128};

    auto labels = ColorQuantization::assignLabels(pixels.data(), 3, 3, palette);
    EXPECT_EQ(labels.size(), 3u);
    EXPECT_EQ(labels[0], 0); // black -> black palette
    EXPECT_EQ(labels[1], 1); // white -> white palette
}