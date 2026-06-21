#pragma once

#include "CoreTypes.h"
#include "Image.h"

#include <vector>
#include <memory>

namespace PixelForge {

/// WPAP (Wedha's Pop Art Portrait) generation module.
/// Pipeline: Face Detection → Edge-Aware Point Sampling → Color Quantization
///           → Delaunay Triangulation → Facet Color Fill → Optional SVG Export
class WapModule {
public:
    WapModule();
    ~WapModule();

    /// Generate WPAP art from input image
    /// @param input Source image
    /// @param params WPAP parameters
    /// @param progress Optional progress callback
    /// @return Generated WPAP raster image
    Image generate(const Image& input, const WapParameters& params,
                   ProgressCallback progress = nullptr);

    /// Get the generated triangles (for SVG export or color editing)
    const std::vector<Triangle>& triangles() const;

    /// Get the detected face regions
    const std::vector<Rect2i>& faceRegions() const;

    /// Export WPAP as SVG
    bool exportSVG(const std::string& filePath, int width, int height) const;

    /// Re-color a specific triangle
    void recolorTriangle(size_t index, Color3u8 newColor);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    /// Step 1: Detect faces using Haar Cascade or DNN
    std::vector<Rect2i> detectFaces(const Image& input);

    /// Step 2: Edge-aware point sampling
    std::vector<Point2f> samplePoints(const Image& input,
                                       const WapParameters& params,
                                       const std::vector<Rect2i>& faces);

    /// Step 3: Compute edge density map
    std::vector<float> computeEdgeDensity(const Image& input);

    /// Step 4: Color quantization to get palette
    std::vector<Color3u8> computePalette(const Image& input,
                                          const WapParameters& params);

    /// Step 5: Fill triangles with dominant colors
    void fillTriangles(std::vector<Triangle>& triangles,
                       const Image& input,
                       const std::vector<Color3u8>& palette);
};

} // namespace PixelForge