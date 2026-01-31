#include <TheEngine/Math/Types.h>
#include <TheEngine/Math/Backend/MathBackend.h>

using namespace TheEngine;

Mat4 Mat4::identity() { return math::backend::identity(); }
Mat4 Mat4::orthographicLH(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ) { return math::backend::orthographicLH(left, right, bottom, top, nearZ, farZ); }
Mat4 Mat4::orthographicScreen(f32 screenWidth, f32 screenHeight, f32 nearZ, f32 farZ) { return math::backend::orthographicScreen(screenWidth, screenHeight, nearZ, farZ); }
Mat4 Mat4::orthographic(f32 width, f32 height, f32 nearZ, f32 farZ) { return math::backend::orthographic(width, height, nearZ, farZ); }
Mat4 Mat4::orthographicPixelSpace(f32 width, f32 height, f32 nearZ, f32 farZ) { return math::backend::orthographicPixelSpace(width, height, nearZ, farZ); }
Mat4 Mat4::createScreenSpaceProjection(float screenWidth, float screenHeight) { return math::backend::createScreenSpaceProjection(screenWidth, screenHeight); }
Mat4 Mat4::translation(const Vec3& pos) { return math::backend::translation(pos); }
Mat4 Mat4::transposeMatrix(const Mat4& matrix) { return math::backend::transposeMatrix(matrix); }
Mat4 Mat4::scale(const Vec3& scale) { return math::backend::scale(scale); }
Mat4 Mat4::rotationZ(f32 angle) { return math::backend::rotationZ(angle); }
Mat4 Mat4::rotationY(f32 angle) { return math::backend::rotationY(angle); }
Mat4 Mat4::rotationX(f32 angle) { return math::backend::rotationX(angle); }
Mat4 Mat4::lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) { return math::backend::lookAt(eye, target, up); }
Mat4 Mat4::perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ) { return math::backend::perspective(fovY, aspect, nearZ, farZ); }

Mat4 Mat4::operator*(const Mat4& other) const { return math::backend::multiply(*this, other); }
