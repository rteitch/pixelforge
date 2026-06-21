#include <gtest/gtest.h>
#include "core/Image.h"

using namespace PixelForge;

TEST(Image, DefaultConstruction) {
    Image img;
    EXPECT_TRUE(img.isEmpty());
    EXPECT_EQ(img.width(), 0);
    EXPECT_EQ(img.height(), 0);
}

TEST(Image, CreateEmpty) {
    Image img = Image::empty(100, 80, 3);
    EXPECT_FALSE(img.isEmpty());
    EXPECT_EQ(img.width(), 100);
    EXPECT_EQ(img.height(), 80);
    EXPECT_EQ(img.channels(), 3);
}

TEST(Image, DeepCopy) {
    Image img = Image::empty(50, 50, 3);
    Image copy = img.deepCopy();
    EXPECT_EQ(copy.width(), 50);
    EXPECT_EQ(copy.height(), 50);
    // Modifying copy shouldn't affect original
    copy.setPixelAt(0, 0, {255, 0, 0});
    auto origPixel = img.pixelAt(0, 0);
    EXPECT_EQ(origPixel.r, 0);
}

TEST(Image, PixelAccess) {
    Image img = Image::empty(10, 10, 3);
    img.setPixelAt(5, 5, {100, 150, 200});
    auto pixel = img.pixelAt(5, 5);
    EXPECT_EQ(pixel.r, 100);
    EXPECT_EQ(pixel.g, 150);
    EXPECT_EQ(pixel.b, 200);
}

TEST(Image, OutOfBoundsPixelAccess) {
    Image img = Image::empty(10, 10, 3);
    auto pixel = img.pixelAt(-1, -1);
    EXPECT_EQ(pixel.r, 0);
    EXPECT_EQ(pixel.g, 0);
    EXPECT_EQ(pixel.b, 0);
}

TEST(Image, MemoryBytes) {
    Image img = Image::empty(100, 100, 3);
    EXPECT_EQ(img.memoryBytes(), 100 * 100 * 3);
}

TEST(Image, HasAlpha) {
    Image img3 = Image::empty(10, 10, 3);
    EXPECT_FALSE(img3.hasAlpha());

    Image img4 = Image::empty(10, 10, 4);
    EXPECT_TRUE(img4.hasAlpha());
}

TEST(Image, DownsampledForPreview) {
    Image img = Image::empty(4000, 3000, 3);
    Image preview = img.downsampledForPreview(2000000);
    int64_t pixels = static_cast<int64_t>(preview.width()) * preview.height();
    EXPECT_LE(pixels, 2000000);
}

TEST(Image, CopyConstructor) {
    Image img = Image::empty(50, 50, 3);
    img.setPixelAt(10, 10, {42, 42, 42});
    Image copy(img);
    EXPECT_EQ(copy.width(), 50);
    auto pixel = copy.pixelAt(10, 10);
    EXPECT_EQ(pixel.r, 42);
}