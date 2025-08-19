#pragma once
#include <DX3D/Core/Core.h>
#include <vector>
#include <d3d11.h>
#include <cmath>

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

    struct Vec2 { f32 x{}, y{}; };

    class Vec3
    {
    public:
        Vec3() = default;
        Vec3(f32 x, f32 y, f32 z) : x(x), y(y), z(z) {}

        // Add some basic operators
        Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
        Vec3 operator*(f32 scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }

        f32 x{}, y{}, z{};
    };

    class Vec4
    {
    public:
        Vec4() = default;
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

        // Static factory methods
        static Mat4 identity() {
            return Mat4();
        }
        static Mat4 orthographic(f32 width, f32 height, f32 nearZ, f32 farZ) {
            Mat4 result;
            result.m[0] = 2.0f / width;                    // X scale
            result.m[5] = 2.0f / height;                   // Y scale  
            result.m[10] = -2.0f / (farZ - nearZ);         // Z scale (note the negative!)
            result.m[14] = -(farZ + nearZ) / (farZ - nearZ); // Z offset (different formula)
            result.m[15] = 1.0f;                           // W component
            return result;
        }
        static Mat4 translation(const Vec3& pos) {
            Mat4 result;
            result.m[12] = pos.x;
            result.m[13] = pos.y;
            result.m[14] = pos.z;
            return result;
        }
        static Mat4 transposeMatrix(const Mat4& matrix) {
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

        static Mat4 scale(const Vec3& scale) {
            Mat4 result;
            result.m[0] = scale.x;
            result.m[5] = scale.y;
            result.m[10] = scale.z;
            return result;
        }

        static Mat4 rotationZ(f32 angle) {
            Mat4 result;
            f32 c = std::cos(angle);
            f32 s = std::sin(angle);
            result.m[0] = c;  result.m[1] = s;
            result.m[4] = -s; result.m[5] = c;
            return result;
        }

        static Mat4 rotationY(f32 angle) {
            Mat4 result;
            f32 c = std::cos(angle);
            f32 s = std::sin(angle);
            result.m[0] = c;   result.m[2] = -s;
            result.m[8] = s;   result.m[10] = c;
            return result;
        }

        static Mat4 rotationX(f32 angle) {
            Mat4 result;
            f32 c = std::cos(angle);
            f32 s = std::sin(angle);
            result.m[5] = c;   result.m[6] = s;
            result.m[9] = -s;  result.m[10] = c;
            return result;
        }

        // Matrix multiplication
        Mat4 operator*(const Mat4& other) const {
            Mat4 result;
            for (int row = 0; row < 4; ++row) {
                for (int col = 0; col < 4; ++col) {
                    result.m[row * 4 + col] = 0;
                    for (int k = 0; k < 4; ++k) {
                        result.m[row * 4 + col] += m[row * 4 + k] * other.m[k * 4 + col];
                    }
                }
            }
            return result;
        }

        // Access raw data (for sending to GPU)
        const f32* data() const { return m; }
        f32* data() { return m; }

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