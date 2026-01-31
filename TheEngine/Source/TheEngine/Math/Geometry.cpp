#include <TheEngine/Math/Geometry.h>

namespace TheEngine {
namespace geom {

float cross(const Vec2& o, const Vec2& a, const Vec2& b) {
    float ax = a.x - o.x;
    float ay = a.y - o.y;
    float bx = b.x - o.x;
    float by = b.y - o.y;
    return ax * by - ay * bx;
}

std::vector<Vec2> clipPolygonWithHalfPlane(const std::vector<Vec2>& poly, const HalfPlane& hp) {
    std::vector<Vec2> out;
    if (poly.empty()) return out;
    auto inside = [&](const Vec2& p) -> bool { return hp.n.x * p.x + hp.n.y * p.y <= hp.d + 1e-4f; };
    auto intersect = [&](const Vec2& a, const Vec2& b) -> Vec2 {
        Vec2 ab = b - a;
        float denom = hp.n.x * ab.x + hp.n.y * ab.y;
        if (std::abs(denom) < 1e-6f) return a;
        float t = (hp.d - (hp.n.x * a.x + hp.n.y * a.y)) / denom;
        return a + ab * t;
    };

    for (size_t i = 0; i < poly.size(); ++i) {
        Vec2 curr = poly[i];
        Vec2 prev = poly[(i + poly.size() - 1) % poly.size()];
        bool currIn = inside(curr);
        bool prevIn = inside(prev);
        if (currIn) {
            if (!prevIn) out.push_back(intersect(prev, curr));
            out.push_back(curr);
        } else if (prevIn) {
            out.push_back(intersect(prev, curr));
        }
    }
    return out;
}

std::vector<Vec2> computeVoronoiCell(const Vec2& site, const std::vector<Vec2>& allSites, const Vec2& boundsCenter, const Vec2& boundsSize) {
    Vec2 hs = boundsSize * 0.5f;
    std::vector<Vec2> poly = {
        Vec2(boundsCenter.x - hs.x, boundsCenter.y - hs.y),
        Vec2(boundsCenter.x + hs.x, boundsCenter.y - hs.y),
        Vec2(boundsCenter.x + hs.x, boundsCenter.y + hs.y),
        Vec2(boundsCenter.x - hs.x, boundsCenter.y + hs.y)
    };

    for (const auto& other : allSites) {
        if (other.x == site.x && other.y == site.y) continue;
        Vec2 m = (site + other) * 0.5f;
        Vec2 n = other - site;
        HalfPlane hp{ n, n.x * m.x + n.y * m.y };
        poly = clipPolygonWithHalfPlane(poly, hp);
        if (poly.empty()) break;
    }
    return poly;
}

std::vector<Vec2> computeConvexHull(const std::vector<Vec2>& points) {
    if (points.size() <= 1) return points;
    std::vector<Vec2> pts = points;
    std::sort(pts.begin(), pts.end(), [](const Vec2& p1, const Vec2& p2) {
        if (p1.x == p2.x) return p1.y < p2.y;
        return p1.x < p2.x;
    });

    std::vector<Vec2> lower;
    for (const auto& p : pts) {
        while (lower.size() >= 2 && cross(lower[lower.size()-2], lower.back(), p) <= 0.0f) {
            lower.pop_back();
        }
        lower.push_back(p);
    }

    std::vector<Vec2> upper;
    for (int i = static_cast<int>(pts.size()) - 1; i >= 0; --i) {
        const auto& p = pts[i];
        while (upper.size() >= 2 && cross(upper[upper.size()-2], upper.back(), p) <= 0.0f) {
            upper.pop_back();
        }
        upper.push_back(p);
    }

    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());
    return lower;
}

} // namespace geom
} // namespace TheEngine
