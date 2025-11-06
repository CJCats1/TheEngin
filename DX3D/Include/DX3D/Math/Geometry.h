#pragma once
#include <DX3D/Core/Core.h>
#include <vector>
#include <d3d11.h>
#include <cmath>
#include <string>
#include <algorithm>

namespace dx3d {
    static float clamp(float value, float min, float max) {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
    struct MeshDimensions {
        f32 width{ 0 }, height{ 0 }, depth{ 0 }; // depth=0 for 2D quads
    };

    enum class Primitive {
        Triangles,
        Lines
    };

    class Rect
    {
    public:
        Rect() = default;
        Rect(i32 width, i32 height) : left(0), top(0), width(width), height(height) {}
        Rect(i32 left, i32 top, i32 width, i32 height) : left(left), top(top), width(width), height(height) {}
    public:
        i32 left{}, top{}, width{}, height{};
    };

    struct Vec2 {
        f32 x{}, y{};

        // Constructors
        Vec2() = default;
        Vec2(f32 x, f32 y) : x(x), y(y) {}

        // Basic operators
        Vec2 operator+(const Vec2& other) const { return Vec2(x + other.x, y + other.y); }
        Vec2 operator-(const Vec2& other) const { return Vec2(x - other.x, y - other.y); }
        Vec2 operator-() const { return Vec2(-x, -y); }  // Unary minus
        Vec2 operator*(f32 scalar) const { return Vec2(x * scalar, y * scalar); }
        Vec2 operator/(f32 scalar) const {
            if (scalar != 0.0f) return Vec2(x / scalar, y / scalar);
            return Vec2(0, 0);
        }
        Vec2& operator/=(f32 scalar) {
            if (scalar != 0.0f) { x /= scalar; y /= scalar; }
            else { x = 0; y = 0; }
            return *this;
		}
        // Assignment operators
        Vec2& operator+=(const Vec2& other) { x += other.x; y += other.y; return *this; }
        Vec2& operator-=(const Vec2& other) { x -= other.x; y -= other.y; return *this; }
        Vec2& operator-=(f32 scalar) { x -= scalar; y -= scalar; return *this; }

        Vec2& operator*=(f32 scalar) { x *= scalar; y *= scalar; return *this; }

        // Utility methods
        f32 length() const { return std::sqrt(x * x + y * y); }
        f32 lengthSquared() const { return x * x + y * y; }

        Vec2 normalized() const {
            f32 len = length();
            if (len > 0.0001f) return *this / len;
            return Vec2(0, 0);
        }

        void normalize() {
            f32 len = length();
            if (len > 0.0001f) { x /= len; y /= len; }
            else { x = 0; y = 0; }
        }

        f32 dot(const Vec2& other) const { return x * other.x + y * other.y; }
    };

    class Vec3
    {
    public:
        Vec3() = default;
        Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}

        // Basic operators
        Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
        Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
        Vec3 operator*(f32 scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
        Vec3 operator/(f32 scalar) const {
            if (scalar != 0.0f) return Vec3(x / scalar, y / scalar, z / scalar);
            return Vec3(0, 0, 0);
        }

        // Assignment operators
        Vec3& operator+=(const Vec3& other) { x += other.x; y += other.y; z += other.z; return *this; }
        Vec3& operator-=(const Vec3& other) { x -= other.x; y -= other.y; z -= other.z; return *this; }
        Vec3& operator*=(f32 scalar) { x *= scalar; y *= scalar; z *= scalar; return *this; }

        // Utility methods
        f32 length() const { return std::sqrt(x * x + y * y + z * z); }
        f32 lengthSquared() const { return x * x + y * y + z * z; }

        Vec3 normalized() const {
            f32 len = length();
            if (len > 0.0001f) return *this / len;
            return Vec3(0, 0, 1);
        }

        void normalize() {
            f32 len = length();
            if (len > 0.0001f) { x /= len; y /= len; z /= len; }
            else { x = 0; y = 0; z = 1; }
        }

        f32 dot(const Vec3& other) const { return x * other.x + y * other.y + z * other.z; }

        Vec3 cross(const Vec3& other) const {
            return Vec3(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
            );
        }
        Vec3 normalize(const Vec3& v) {
            float len = sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
            if (len > 0.0001f) {
                return Vec3(v.x / len, v.y / len, v.z / len);
            }
            return Vec3(0, 0, 1);
        }

        Vec3 cross(const Vec3& a, const Vec3& b) {
            return Vec3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        float dot(const Vec3& a, const Vec3& b) {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }
        f32 x{}, y{}, z{};
    };

    class Vec4
    {
    public:
        Vec4() = default;
        Vec4 operator-(const Vec4& other) const { return Vec4(x - other.x, y - other.y, z - other.z,w-other.w); }
        Vec4 operator+(const Vec4& other) const { return Vec4(x + other.x, y + other.y, z + other.z,w+other.z); }
        Vec4 operator*(f32 scalar) const { return Vec4(x * scalar, y * scalar, z * scalar,w*scalar); }

        Vec4(f32 x, f32 y, f32 z, f32 w) : x(x), y(y), z(z), w(w) {}
        f32 x{}, y{}, z{}, w{};
    };

    // Basic 4x4 Matrix class
    class Mat4
    {
    public:
        Mat4() {
            // Initialize as identity matrix
            for (int i = 0; i < 16; ++i) m[i] = 0.0f;
            m[0] = m[5] = m[10] = m[15] = 1.0f;
        }

        Mat4(f32 values[16]) {
            for (int i = 0; i < 16; ++i) m[i] = values[i];
        }

        // 2D array access operators (row, column)
        f32& operator()(int row, int col) { return m[row * 4 + col]; }
        const f32& operator()(int row, int col) const { return m[row * 4 + col]; }
        
        // 1D array access for backward compatibility
        f32& operator[](int index) { return m[index]; }
        const f32& operator[](int index) const { return m[index]; }

        // Static factory methods
        static Mat4 identity();
        static Mat4 orthographicLH(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ);
        static Mat4 orthographicScreen(f32 screenWidth, f32 screenHeight, f32 nearZ = 0.0f, f32 farZ = 1.0f);
        static Mat4 orthographic(f32 width, f32 height, f32 nearZ, f32 farZ);
        static Mat4 orthographicPixelSpace(f32 width, f32 height, f32 nearZ = 0.1f, f32 farZ = 100.0f);
        static Mat4 createScreenSpaceProjection(float screenWidth, float screenHeight);
        static Mat4 translation(const Vec3& pos);
        static Mat4 transposeMatrix(const Mat4& matrix);
        static Mat4 scale(const Vec3& scale);
        static Mat4 rotationZ(f32 angle);
        static Mat4 rotationY(f32 angle);
        static Mat4 rotationX(f32 angle);

        // Matrix multiplication
        Mat4 operator*(const Mat4& other) const;

        // Access raw data (for sending to GPU)
        const f32* data() const { return m; }
        f32* data() { return m; }
        static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
        static Mat4 perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ);

    private:
        f32 m[16]; // Column-major order
    };

    struct Vertex
    {
        Vec3 pos;
        Vec3 normal;
        Vec2 uv;
        Vec4 color;
    };
    
}

namespace dx3d {
    namespace geom {
        struct HalfPlane { Vec2 n; float d; };

        // 2D geometry helpers (declarations)
        float cross(const Vec2& o, const Vec2& a, const Vec2& b);

        std::vector<Vec2> clipPolygonWithHalfPlane(
            const std::vector<Vec2>& poly,
            const HalfPlane& hp
        );

        std::vector<Vec2> computeVoronoiCell(
            const Vec2& site,
            const std::vector<Vec2>& allSites,
            const Vec2& boundsCenter,
            const Vec2& boundsSize
        );

        std::vector<Vec2> computeConvexHull(const std::vector<Vec2>& points);
    }
}