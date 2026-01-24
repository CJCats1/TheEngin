#pragma once
#include <DX3D/Math/Types.h>

namespace dx3d::math::backend
{
    enum class MathBackendType
    {
        Default,
        Glm
    };

    void setBackend(MathBackendType type);
    MathBackendType getBackend();

    Mat4 identity();
    Mat4 orthographicLH(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ);
    Mat4 orthographicScreen(f32 screenWidth, f32 screenHeight, f32 nearZ, f32 farZ);
    Mat4 orthographic(f32 width, f32 height, f32 nearZ, f32 farZ);
    Mat4 orthographicPixelSpace(f32 width, f32 height, f32 nearZ, f32 farZ);
    Mat4 createScreenSpaceProjection(float screenWidth, float screenHeight);
    Mat4 translation(const Vec3& pos);
    Mat4 transposeMatrix(const Mat4& matrix);
    Mat4 scale(const Vec3& scale);
    Mat4 rotationZ(f32 angle);
    Mat4 rotationY(f32 angle);
    Mat4 rotationX(f32 angle);
    Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up);
    Mat4 perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ);
    Mat4 multiply(const Mat4& a, const Mat4& b);
}
