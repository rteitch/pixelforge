#include <gtest/gtest.h>
#include "utils/DelaunayTriangulation.h"

using namespace PixelForge;

TEST(DelaunayTriangulation, EmptyInput) {
    std::vector<Point2f> points;
    auto result = DelaunayTriangulation::compute(points);
    EXPECT_TRUE(result.empty());
}

TEST(DelaunayTriangulation, ThreePoints) {
    std::vector<Point2f> points = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}
    };
    auto result = DelaunayTriangulation::compute(points);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_EQ(result[0].v[0], 0);
    EXPECT_EQ(result[0].v[1], 1);
    EXPECT_EQ(result[0].v[2], 2);
}

TEST(DelaunayTriangulation, FourPointsSquare) {
    std::vector<Point2f> points = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
    };
    auto result = DelaunayTriangulation::compute(points);
    EXPECT_EQ(result.size(), 2u);
}

TEST(DelaunayTriangulation, ManyPoints) {
    std::vector<Point2f> points;
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            points.push_back({static_cast<float>(x), static_cast<float>(y)});
        }
    }
    auto result = DelaunayTriangulation::compute(points);
    EXPECT_GT(result.size(), 0u);

    // Euler's formula: for n points, ~2n triangles (approximately)
    EXPECT_GT(result.size(), 100u);
}

TEST(DelaunayTriangulation, ResolveTriangles) {
    std::vector<Point2f> points = {
        {0.0f, 0.0f}, {1.0f, 0.0f}, {0.5f, 1.0f}
    };
    auto indices = DelaunayTriangulation::compute(points);
    auto triangles = DelaunayTriangulation::resolveTriangles(points, indices);

    ASSERT_EQ(triangles.size(), 1u);
    EXPECT_FLOAT_EQ(triangles[0].vertices[0].x, 0.0f);
    EXPECT_FLOAT_EQ(triangles[0].vertices[1].x, 1.0f);
    EXPECT_FLOAT_EQ(triangles[0].vertices[2].x, 0.5f);
}