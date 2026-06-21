#include "DelaunayTriangulation.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <unordered_set>
#include <cassert>

namespace PixelForge {

// ============================================================
// Bowyer-Watson Delaunay Triangulation
// ============================================================

std::vector<DelaunayTriangulation::TriangleIndex>
DelaunayTriangulation::compute(const std::vector<Point2f>& points) {
    if (points.size() < 3) return {};

    // 1. Create super-triangle that contains all points
    SuperTriangle super = computeSuperTriangle(points);

    // Build index arrays: real points + 3 super-triangle vertices
    int n = static_cast<int>(points.size());
    std::vector<Point2f> allPoints = points;
    allPoints.push_back(super.v[0]);
    allPoints.push_back(super.v[1]);
    allPoints.push_back(super.v[2]);
    int si0 = n, si1 = n + 1, si2 = n + 2;

    // 2. Start with super-triangle
    std::vector<TriangleIndex> triangulation;
    triangulation.push_back({si0, si1, si2});

    // 3. Insert each point one by one
    for (int i = 0; i < n; ++i) {
        const Point2f& p = allPoints[i];

        // Find all triangles whose circumcircle contains the new point
        std::vector<TriangleIndex> badTriangles;
        for (const auto& tri : triangulation) {
            const Point2f& a = allPoints[tri.v[0]];
            const Point2f& b = allPoints[tri.v[1]];
            const Point2f& c = allPoints[tri.v[2]];
            if (inCircumcircle(p, a, b, c)) {
                badTriangles.push_back(tri);
            }
        }

        // Find boundary polygon of the hole
        // An edge is on the boundary if it belongs to exactly one bad triangle
        struct Edge {
            int v0, v1;
            bool operator==(const Edge& o) const {
                return (v0 == o.v0 && v1 == o.v1) || (v0 == o.v1 && v1 == o.v0);
            }
        };

        // Count edge occurrences
        struct EdgeHash {
            size_t operator()(const Edge& e) const {
                int a = std::min(e.v0, e.v1);
                int b = std::max(e.v0, e.v1);
                return std::hash<int>()(a) ^ (std::hash<int>()(b) << 16);
            }
        };
        struct EdgeEqual {
            bool operator()(const Edge& a, const Edge& b) const {
                return a == b;
            }
        };

        std::unordered_map<Edge, int, EdgeHash, EdgeEqual> edgeCount;
        for (const auto& tri : badTriangles) {
            edgeCount[{tri.v[0], tri.v[1]}]++;
            edgeCount[{tri.v[1], tri.v[2]}]++;
            edgeCount[{tri.v[2], tri.v[0]}]++;
        }

        std::vector<Edge> boundary;
        for (const auto& [edge, count] : edgeCount) {
            if (count == 1) {
                boundary.push_back(edge);
            }
        }

        // Remove bad triangles
        std::unordered_set<int> badIndices;
        for (const auto& bad : badTriangles) {
            for (int j = 0; j < static_cast<int>(triangulation.size()); ++j) {
                const auto& t = triangulation[j];
                if (t.v[0] == bad.v[0] && t.v[1] == bad.v[1] && t.v[2] == bad.v[2]) {
                    badIndices.insert(j);
                }
            }
        }

        std::vector<TriangleIndex> remaining;
        for (int j = 0; j < static_cast<int>(triangulation.size()); ++j) {
            if (badIndices.find(j) == badIndices.end()) {
                remaining.push_back(triangulation[j]);
            }
        }
        triangulation = std::move(remaining);

        // Re-triangulate the hole with new point
        for (const auto& edge : boundary) {
            triangulation.push_back({edge.v0, edge.v1, i});
        }
    }

    // 4. Remove triangles that share vertices with the super-triangle
    std::vector<TriangleIndex> result;
    for (const auto& tri : triangulation) {
        if (tri.v[0] >= n || tri.v[1] >= n || tri.v[2] >= n) continue;
        result.push_back(tri);
    }

    return result;
}

std::vector<Triangle> DelaunayTriangulation::resolveTriangles(
    const std::vector<Point2f>& points,
    const std::vector<TriangleIndex>& indices)
{
    std::vector<Triangle> triangles;
    triangles.reserve(indices.size());

    for (const auto& idx : indices) {
        Triangle t;
        t.vertices[0] = points[idx.v[0]];
        t.vertices[1] = points[idx.v[1]];
        t.vertices[2] = points[idx.v[2]];
        t.fillColor = {0, 0, 0};
        triangles.push_back(t);
    }

    return triangles;
}

DelaunayTriangulation::SuperTriangle
DelaunayTriangulation::computeSuperTriangle(const std::vector<Point2f>& points) {
    float minX = std::numeric_limits<float>::max();
    float minY = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& p : points) {
        minX = std::min(minX, p.x);
        minY = std::min(minY, p.y);
        maxX = std::max(maxX, p.x);
        maxY = std::max(maxY, p.y);
    }

    float dx = maxX - minX;
    float dy = maxY - minY;
    float dmax = std::max(dx, dy);
    float midX = (minX + maxX) / 2.0f;
    float midY = (minY + maxY) / 2.0f;

    // Super-triangle vertices (much larger than the point set)
    SuperTriangle st;
    st.v[0] = {midX - 20.0f * dmax, midY - dmax};
    st.v[1] = {midX, midY + 20.0f * dmax};
    st.v[2] = {midX + 20.0f * dmax, midY - dmax};
    return st;
}

bool DelaunayTriangulation::inCircumcircle(
    const Point2f& p,
    const Point2f& a,
    const Point2f& b,
    const Point2f& c)
{
    // Using determinant method
    float ax_ = a.x - p.x;
    float ay_ = a.y - p.y;
    float bx_ = b.x - p.x;
    float by_ = b.y - p.y;
    float cx_ = c.x - p.x;
    float cy_ = c.y - p.y;

    float det = (ax_ * ax_ + ay_ * ay_) * (bx_ * cy_ - cx_ * by_)
              - (bx_ * bx_ + by_ * by_) * (ax_ * cy_ - cx_ * ay_)
              + (cx_ * cx_ + cy_ * cy_) * (ax_ * by_ - bx_ * ay_);

    // For counter-clockwise oriented triangle, det > 0 means inside circumcircle
    return det > 0.0f;
}

} // namespace PixelForge