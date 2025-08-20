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
        static Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
            Vec3 zaxis = (target - eye).normalized();         // forward
            Vec3 xaxis = up.cross(zaxis).normalized();        // right
            Vec3 yaxis = zaxis.cross(xaxis);                  // up

            Mat4 result;
            result.m[0] = xaxis.x; result.m[1] = yaxis.x; result.m[2] = zaxis.x; result.m[3] = 0.0f;
            result.m[4] = xaxis.y; result.m[5] = yaxis.y; result.m[6] = zaxis.y; result.m[7] = 0.0f;
            result.m[8] = xaxis.z; result.m[9] = yaxis.z; result.m[10] = zaxis.z; result.m[11] = 0.0f;
            result.m[12] = -xaxis.dot(eye);
            result.m[13] = -yaxis.dot(eye);
            result.m[14] = -zaxis.dot(eye);
            result.m[15] = 1.0f;
            return result;
        }

        static Mat4 perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ) {
            f32 f = 1.0f / std::tan(fovY * 0.5f);
            Mat4 result;
            result.m[0] = f / aspect;
            result.m[1] = 0.0f;
            result.m[2] = 0.0f;
            result.m[3] = 0.0f;

            result.m[4] = 0.0f;
            result.m[5] = f;
            result.m[6] = 0.0f;
            result.m[7] = 0.0f;

            result.m[8] = 0.0f;
            result.m[9] = 0.0f;
            result.m[10] = farZ / (farZ - nearZ);
            result.m[11] = 1.0f;

            result.m[12] = 0.0f;
            result.m[13] = 0.0f;
            result.m[14] = -(farZ * nearZ) / (farZ - nearZ);
            result.m[15] = 0.0f;
            return result;
        }

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