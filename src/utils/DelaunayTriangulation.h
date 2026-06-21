#pragma once

#include "core/CoreTypes.h"

#include <vector>
#include <cstdint>

namespace PixelForge {

/// Lightweight Bowyer-Watson Delaunay triangulation.
/// Pure C++ implementation (no CGAL dependency).
class DelaunayTriangulation {
public:
    DelaunayTriangulation() = default;
    ~DelaunayTriangulation() = default;

    /// Triangulate a set of 2D points. Returns triangles with vertex indices.
    struct TriangleIndex {
        int v[3];
    };

    /// Perform Delaunay triangulation on the given points.
    /// @param points Input point cloud
    /// @return Vector of triangles (each referencing indices into the points array)
    static std::vector<TriangleIndex> compute(const std::vector<Point2f>& points);

    /// Convert indexed triangles to triangles with actual coordinates
    static std::vector<Triangle> resolveTriangles(
        const std::vector<Point2f>& points,
        const std::vector<TriangleIndex>& indices
    );

private:
    // Internal super-triangle helper
    struct SuperTriangle {
        Point2f v[3];
    };

    static SuperTriangle computeSuperTriangle(const std::vector<Point2f>& points);

    // Circumcircle test
    static bool inCircumcircle(const Point2f& p,
                                const Point2f& a,
                                const Point2f& b,
                                const Point2f& c);
};

} // namespace PixelForge