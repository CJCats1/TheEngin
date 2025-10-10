#include <DX3D/Math/Geometry.h>
#include <algorithm>

namespace dx3d {

Mat4 Mat4::identity() {
    return Mat4();
}

Mat4 Mat4::orthographicLH(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ) {
    Mat4 result;

    f32 width = right - left;
    f32 height = top - bottom;
    f32 depth = farZ - nearZ;

    if (width == 0.0f || height == 0.0f || depth == 0.0f) {
        return identity();
    }

    result[0] = 2.0f / width;                           // Scale X
    result[5] = 2.0f / height;                          // Scale Y  
    result[10] = 1.0f / depth;                          // Scale Z
    result[12] = -(right + left) / width;               // Translate X
    result[13] = -(top + bottom) / height;              // Translate Y
    result[14] = -nearZ / depth;                        // Translate Z
    result[15] = 1.0f;                                  // W

    return result;
}

Mat4 Mat4::orthographicScreen(f32 screenWidth, f32 screenHeight, f32 nearZ, f32 farZ) {
    Mat4 result;

    result[0] = 2.0f / screenWidth;     // Scale X
    result[12] = -1.0f;                 // Translate X

    result[5] = -2.0f / screenHeight;   // Scale Y (flip)
    result[13] = 1.0f;                  // Translate Y

    result[10] = 1.0f / (farZ - nearZ); // Scale Z
    result[14] = -nearZ / (farZ - nearZ); // Translate Z

    result[15] = 1.0f;                  // W
    return result;
}

Mat4 Mat4::orthographic(f32 width, f32 height, f32 nearZ, f32 farZ) {
    Mat4 result = {};

    f32 left = -width * 0.5f;
    f32 right = width * 0.5f;
    f32 bottom = -height * 0.5f;
    f32 top = height * 0.5f;

    result(0, 0) = 2.0f / (right - left);
    result(1, 1) = 2.0f / (top - bottom);
    result(2, 2) = 1.0f / (farZ - nearZ);
    result(3, 3) = 1.0f;

    result(3, 0) = -(right + left) / (right - left);
    result(3, 1) = -(top + bottom) / (top - bottom);
    result(3, 2) = -nearZ / (farZ - nearZ);

    return result;
}

Mat4 Mat4::orthographicPixelSpace(f32 width, f32 height, f32 nearZ, f32 farZ) {
    Mat4 result;
    result[0] = 2.0f / width;                   // X scale
    result[5] = -2.0f / height;                 // Y scale (flip)
    result[10] = -2.0f / (farZ - nearZ);        // Z scale
    result[12] = -1.0f;                         // X offset
    result[13] = 1.0f;                          // Y offset
    result[14] = -(farZ + nearZ) / (farZ - nearZ); // Z offset
    result[15] = 1.0f;
    return result;
}

Mat4 Mat4::createScreenSpaceProjection(float screenWidth, float screenHeight) {
    return Mat4::orthographic(screenWidth, screenHeight, -100.0f, 100.0f);
}

Mat4 Mat4::translation(const Vec3& pos) {
    Mat4 result;
    result[12] = pos.x;
    result[13] = pos.y;
    result[14] = pos.z;
    return result;
}

Mat4 Mat4::transposeMatrix(const Mat4& matrix) {
    Mat4 result;
    const float* src = matrix.data();
    float* dst = result.data();
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            dst[row * 4 + col] = src[col * 4 + row];
        }
    }
    return result;
}

Mat4 Mat4::scale(const Vec3& scale) {
    Mat4 result;
    result[0] = scale.x;
    result[5] = scale.y;
    result[10] = scale.z;
    return result;
}

Mat4 Mat4::rotationZ(f32 angle) {
    Mat4 result;
    f32 c = std::cos(angle);
    f32 s = std::sin(angle);
    result[0] = c;  result[1] = s;
    result[4] = -s; result[5] = c;
    return result;
}

Mat4 Mat4::rotationY(f32 angle) {
    Mat4 result;
    f32 c = std::cos(angle);
    f32 s = std::sin(angle);
    result[0] = c;   result[2] = -s;
    result[8] = s;   result[10] = c;
    return result;
}

Mat4 Mat4::rotationX(f32 angle) {
    Mat4 result;
    f32 c = std::cos(angle);
    f32 s = std::sin(angle);
    result[5] = c;   result[6] = s;
    result[9] = -s;  result[10] = c;
    return result;
}

Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
    Vec3 zaxis = (target - eye).normalized();         // forward
    Vec3 xaxis = up.cross(zaxis).normalized();        // right
    Vec3 yaxis = zaxis.cross(xaxis);                  // up

    Mat4 result;
    result[0] = xaxis.x; result[1] = yaxis.x; result[2] = zaxis.x; result[3] = 0.0f;
    result[4] = xaxis.y; result[5] = yaxis.y; result[6] = zaxis.y; result[7] = 0.0f;
    result[8] = xaxis.z; result[9] = yaxis.z; result[10] = zaxis.z; result[11] = 0.0f;
    result[12] = -xaxis.dot(eye);
    result[13] = -yaxis.dot(eye);
    result[14] = -zaxis.dot(eye);
    result[15] = 1.0f;
    return result;
}

Mat4 Mat4::perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ) {
    f32 f = 1.0f / std::tan(fovY * 0.5f);
    Mat4 result;
    result[0] = f / aspect;
    result[1] = 0.0f;
    result[2] = 0.0f;
    result[3] = 0.0f;

    result[4] = 0.0f;
    result[5] = f;
    result[6] = 0.0f;
    result[7] = 0.0f;

    result[8] = 0.0f;
    result[9] = 0.0f;
    result[10] = farZ / (farZ - nearZ);
    result[11] = 1.0f;

    result[12] = 0.0f;
    result[13] = 0.0f;
    result[14] = -(farZ * nearZ) / (farZ - nearZ);
    result[15] = 0.0f;
    return result;
}

Mat4 Mat4::operator*(const Mat4& other) const {
    Mat4 result;
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            result[row * 4 + col] = 0.0f;
            for (int k = 0; k < 4; ++k) {
                result[row * 4 + col] += (*this)[row * 4 + k] * other[k * 4 + col];
            }
        }
    }
    return result;
}

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
} // namespace dx3d


