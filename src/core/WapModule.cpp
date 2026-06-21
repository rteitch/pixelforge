#include "WapModule.h"
#include "utils/DelaunayTriangulation.h"
#include "utils/ColorQuantization.h"

#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <random>
#include <numeric>

namespace PixelForge {

struct WapModule::Impl {
    std::vector<Triangle> triangles;
    std::vector<Rect2i> faceRegions;
};

WapModule::WapModule() : impl_(std::make_unique<Impl>()) {}
WapModule::~WapModule() = default;

// ============================================================
// Face Detection
// ============================================================

std::vector<Rect2i> WapModule::detectFaces(const Image& input) {
    std::vector<Rect2i> faces;

    cv::Mat gray;
    if (input.mat().channels() >= 3) {
        cv::cvtColor(input.mat(), gray, cv::COLOR_RGB2GRAY);
    } else {
        gray = input.mat().clone();
    }

    cv::CascadeClassifier faceCascade;
    // Try loading Haar cascade from OpenCV data directory
    std::vector<std::string> cascadePaths = {
        "haarcascade_frontalface_default.xml",
        "data/haarcascade_frontalface_default.xml",
        "/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml",
        "C:/opencv/etc/haarcascades/haarcascade_frontalface_default.xml"
    };

    bool loaded = false;
    for (const auto& path : cascadePaths) {
        if (faceCascade.load(path)) {
            loaded = true;
            break;
        }
    }

    if (!loaded) {
        return faces; // empty — will proceed without face detection
    }

    std::vector<cv::Rect> cvFaces;
    faceCascade.detectMultiScale(gray, cvFaces, 1.1, 4,
                                  cv::CASCADE_SCALE_IMAGE,
                                  cv::Size(30, 30));

    for (const auto& f : cvFaces) {
        faces.push_back({f.x, f.y, f.width, f.height});
    }

    return faces;
}

// ============================================================
// Edge Density Map
// ============================================================

std::vector<float> WapModule::computeEdgeDensity(const Image& input) {
    cv::Mat gray;
    if (input.mat().channels() >= 3) {
        cv::cvtColor(input.mat(), gray, cv::COLOR_RGB2GRAY);
    } else {
        gray = input.mat().clone();
    }

    // Canny edge detection
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150);

    // Gaussian blur the edge map for smoother density
    cv::Mat blurred;
    cv::GaussianBlur(edges, blurred, cv::Size(15, 15), 5.0);

    int w = input.width();
    int h = input.height();
    std::vector<float> density(w * h);

    for (int y = 0; y < h; ++y) {
        const uint8_t* row = blurred.ptr<uint8_t>(y);
        for (int x = 0; x < w; ++x) {
            density[y * w + x] = row[x] / 255.0f;
        }
    }

    return density;
}

// ============================================================
// Edge-Aware Point Sampling
// ============================================================

std::vector<Point2f> WapModule::samplePoints(
    const Image& input,
    const WapParameters& params,
    const std::vector<Rect2i>& faces)
{
    int w = input.width();
    int h = input.height();
    if (w == 0 || h == 0) return {};

    // Determine target point count based on detail level
    int targetPoints;
    switch (params.detailLevel) {
        case WapDetailLevel::Low:    targetPoints = 500;  break;
        case WapDetailLevel::Medium: targetPoints = 2000; break;
        case WapDetailLevel::High:   targetPoints = 4000; break;
        case WapDetailLevel::Custom: targetPoints = params.customPointCount; break;
    }
    targetPoints = std::clamp(targetPoints, 100, 8000);

    // Compute edge density
    auto density = computeEdgeDensity(input);

    // Build density map for weighted sampling
    // Add base density + edge-based density
    std::vector<float> weights(w * h);
    float maxDensity = 0.0f;

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            float baseWeight = 0.3f; // base uniform weight
            float edgeWeight = density[idx];

            // Boost density near faces
            if (params.faceDetectionEnabled) {
                for (const auto& face : faces) {
                    float cx = face.x + face.width / 2.0f;
                    float cy = face.y + face.height / 2.0f;
                    float dx = (x - cx) / (face.width * 0.8f);
                    float dy = (y - cy) / (face.height * 0.8f);
                    float dist = std::sqrt(dx * dx + dy * dy);
                    if (dist < 2.0f) {
                        float boost = (1.0f - dist / 2.0f) * (params.faceDetailBoost - 1.0f);
                        baseWeight += boost;
                    }
                }
            }

            weights[idx] = baseWeight + edgeWeight * 2.0f;
            maxDensity = std::max(maxDensity, weights[idx]);
        }
    }

    // Normalize weights
    if (maxDensity > 0.0f) {
        for (auto& w : weights) w /= maxDensity;
    }

    // Weighted random sampling
    std::mt19937 rng(42);
    std::vector<Point2f> points;

    // Always include corners and border points
    points.push_back({0.0f, 0.0f});
    points.push_back({static_cast<float>(w - 1), 0.0f});
    points.push_back({0.0f, static_cast<float>(h - 1)});
    points.push_back({static_cast<float>(w - 1), static_cast<float>(h - 1)});

    // Add edge points
    int borderStep = std::max(1, w / 20);
    for (int x = borderStep; x < w - 1; x += borderStep) {
        points.push_back({static_cast<float>(x), 0.0f});
        points.push_back({static_cast<float>(x), static_cast<float>(h - 1)});
    }
    for (int y = borderStep; y < h - 1; y += borderStep) {
        points.push_back({0.0f, static_cast<float>(y)});
        points.push_back({static_cast<float>(w - 1), static_cast<float>(y)});
    }

    // Build cumulative distribution for weighted sampling
    std::vector<double> cumWeights(w * h);
    double cumSum = 0.0;
    for (int i = 0; i < w * h; ++i) {
        cumSum += weights[i];
        cumWeights[i] = cumSum;
    }

    std::uniform_real_distribution<double> dist(0.0, cumSum);
    int remaining = targetPoints - static_cast<int>(points.size());

    for (int i = 0; i < remaining && i < targetPoints; ++i) {
        double r = dist(rng);
        auto it = std::lower_bound(cumWeights.begin(), cumWeights.end(), r);
        int idx = static_cast<int>(std::distance(cumWeights.begin(), it));
        int px = idx % w;
        int py = idx / w;
        points.push_back({static_cast<float>(px), static_cast<float>(py)});
    }

    return points;
}

// ============================================================
// Color Palette
// ============================================================

std::vector<Color3u8> WapModule::computePalette(
    const Image& input,
    const WapParameters& params)
{
    // Use preset palettes or k-means quantization
    switch (params.palettePreset) {
        case WapPalettePreset::Vibrant:
            return ColorQuantization::vibrantPalette(params.colorCount);
        case WapPalettePreset::Pastel:
            return ColorQuantization::pastelPalette(params.colorCount);
        case WapPalettePreset::MonochromeAccent:
            return ColorQuantization::monochromeAccentPalette(params.colorCount);
        case WapPalettePreset::Custom:
            if (!params.customPalette.empty()) return params.customPalette;
            // Fallback to k-means
            return ColorQuantization::quantizeImage(input, params.colorCount);
        default:
            return ColorQuantization::quantizeImage(input, params.colorCount);
    }
}

// ============================================================
// Triangle Fill
// ============================================================

void WapModule::fillTriangles(
    std::vector<Triangle>& triangles,
    const Image& input,
    const std::vector<Color3u8>& palette)
{
    if (input.isEmpty() || palette.empty()) return;

    const cv::Mat& mat = input.mat();
    int w = mat.cols;
    int h = mat.rows;
    int ch = mat.channels();

    for (auto& tri : triangles) {
        // Compute bounding box of triangle
        float minX = std::min({tri.vertices[0].x, tri.vertices[1].x, tri.vertices[2].x});
        float maxX = std::max({tri.vertices[0].x, tri.vertices[1].x, tri.vertices[2].x});
        float minY = std::min({tri.vertices[0].y, tri.vertices[1].y, tri.vertices[2].y});
        float maxY = std::max({tri.vertices[0].y, tri.vertices[1].y, tri.vertices[2].y});

        int x0 = std::max(0, static_cast<int>(minX));
        int x1 = std::min(w - 1, static_cast<int>(maxX) + 1);
        int y0 = std::max(0, static_cast<int>(minY));
        int y1 = std::min(h - 1, static_cast<int>(maxY) + 1);

        // Accumulate colors from all pixels inside the triangle
        uint64_t sumR = 0, sumG = 0, sumB = 0;
        int count = 0;

        auto sign = [](float x1, float y1, float x2, float y2, float x3, float y3) -> float {
            return (x1 - x3) * (y2 - y3) - (x2 - x3) * (y1 - y3);
        };

        float ax = tri.vertices[0].x, ay = tri.vertices[0].y;
        float bx = tri.vertices[1].x, by = tri.vertices[1].y;
        float cx = tri.vertices[2].x, cy = tri.vertices[2].y;

        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                float px = x + 0.5f;
                float py = y + 0.5f;

                // Point-in-triangle test using sign method
                float d1 = sign(px, py, ax, ay, bx, by);
                float d2 = sign(px, py, bx, by, cx, cy);
                float d3 = sign(px, py, cx, cy, ax, ay);

                bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
                bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);

                if (!(hasNeg && hasPos)) {
                    const uint8_t* p = mat.ptr<uint8_t>(y) + x * ch;
                    sumR += p[0];
                    sumG += p[1];
                    sumB += p[2];
                    count++;
                }
            }
        }

        if (count > 0) {
            Color3u8 meanColor = {
                static_cast<uint8_t>(sumR / count),
                static_cast<uint8_t>(sumG / count),
                static_cast<uint8_t>(sumB / count)
            };

            // Map to nearest palette color
            float bestDist = std::numeric_limits<float>::max();
            for (const auto& palColor : palette) {
                float dr = static_cast<float>(meanColor.r) - palColor.r;
                float dg = static_cast<float>(meanColor.g) - palColor.g;
                float db = static_cast<float>(meanColor.b) - palColor.b;
                float dist = dr * dr + dg * dg + db * db;
                if (dist < bestDist) {
                    bestDist = dist;
                    tri.fillColor = palColor;
                }
            }
        } else {
            tri.fillColor = {128, 128, 128};
        }
    }
}

// ============================================================
// Main Generation Pipeline
// ============================================================

Image WapModule::generate(const Image& input, const WapParameters& params,
                           ProgressCallback progress) {
    if (input.isEmpty()) return Image();

    impl_->triangles.clear();
    impl_->faceRegions.clear();

    // Step 1: Face detection
    if (progress) progress(0.1f, "Detecting faces...");
    if (params.faceDetectionEnabled) {
        impl_->faceRegions = detectFaces(input);
    }

    // Step 2: Edge-aware point sampling
    if (progress) progress(0.2f, "Sampling points...");
    auto points = samplePoints(input, params, impl_->faceRegions);

    if (points.size() < 3) {
        return input.deepCopy();
    }

    // Step 3: Delaunay triangulation
    if (progress) progress(0.4f, "Triangulating...");
    auto triIndices = DelaunayTriangulation::compute(points);
    auto triangles = DelaunayTriangulation::resolveTriangles(points, triIndices);

    // Step 4: Color quantization
    if (progress) progress(0.6f, "Computing palette...");
    auto palette = computePalette(input, params);

    // Step 5: Fill triangles with palette colors
    if (progress) progress(0.8f, "Filling facets...");
    fillTriangles(triangles, input, palette);

    impl_->triangles = std::move(triangles);

    // Step 6: Render result
    if (progress) progress(0.9f, "Rendering...");
    int w = input.width();
    int h = input.height();
    Image result = Image::empty(w, h, 3);
    cv::Mat& outMat = result.mat();

    // Fill background white
    outMat.setTo(cv::Scalar(255, 255, 255));

    // Draw filled triangles
    for (const auto& tri : impl_->triangles) {
        cv::Point pts[3] = {
            {static_cast<int>(tri.vertices[0].x), static_cast<int>(tri.vertices[0].y)},
            {static_cast<int>(tri.vertices[1].x), static_cast<int>(tri.vertices[1].y)},
            {static_cast<int>(tri.vertices[2].x), static_cast<int>(tri.vertices[2].y)}
        };
        cv::fillConvexPoly(outMat, pts, 3,
                           cv::Scalar(tri.fillColor.r, tri.fillColor.g, tri.fillColor.b));
    }

    // Draw thin triangle edges for the geometric look
    for (const auto& tri : impl_->triangles) {
        cv::Point pts[3] = {
            {static_cast<int>(tri.vertices[0].x), static_cast<int>(tri.vertices[0].y)},
            {static_cast<int>(tri.vertices[1].x), static_cast<int>(tri.vertices[1].y)},
            {static_cast<int>(tri.vertices[2].x), static_cast<int>(tri.vertices[2].y)}
        };
        cv::polylines(outMat, pts, 3, true,
                      cv::Scalar(30, 30, 30), 1, cv::LINE_AA);
    }

    if (progress) progress(1.0f, "Done");
    return result;
}

// ============================================================
// Accessors
// ============================================================

const std::vector<Triangle>& WapModule::triangles() const {
    return impl_->triangles;
}

const std::vector<Rect2i>& WapModule::faceRegions() const {
    return impl_->faceRegions;
}

void WapModule::recolorTriangle(size_t index, Color3u8 newColor) {
    if (index < impl_->triangles.size()) {
        impl_->triangles[index].fillColor = newColor;
    }
}

// ============================================================
// SVG Export
// ============================================================

bool WapModule::exportSVG(const std::string& filePath, int width, int height) const {
    std::ofstream file(filePath);
    if (!file.is_open()) return false;

    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
         << "width=\"" << width << "\" height=\"" << height << "\" "
         << "viewBox=\"0 0 " << width << " " << height << "\">\n";
    file << "<rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";

    for (const auto& tri : impl_->triangles) {
        char colorHex[8];
        std::snprintf(colorHex, sizeof(colorHex), "#%02x%02x%02x",
                      tri.fillColor.r, tri.fillColor.g, tri.fillColor.b);

        file << "<polygon points=\""
             << tri.vertices[0].x << "," << tri.vertices[0].y << " "
             << tri.vertices[1].x << "," << tri.vertices[1].y << " "
             << tri.vertices[2].x << "," << tri.vertices[2].y
             << "\" fill=\"" << colorHex
             << "\" stroke=\"#1e1e1e\" stroke-width=\"0.5\"/>\n";
    }

    file << "</svg>\n";
    file.close();
    return true;
}

} // namespace PixelForge