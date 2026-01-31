#include <TheEngine/Math/Backend/MathBackend.h>
#include <cmath>
#if defined(THEENGINE_PLATFORM_ANDROID)
#include <android/log.h>
#endif

namespace TheEngine::math::backend::default_impl
{
    Mat4 identity() {
        return Mat4();
    }

    Mat4 orthographicLH(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ) {
        Mat4 result;

        f32 width = right - left;
        f32 height = top - bottom;
        f32 depth = farZ - nearZ;

        if (width == 0.0f || height == 0.0f || depth == 0.0f) {
            return identity();
        }

        result[0] = 2.0f / width;
        result[5] = 2.0f / height;
        result[10] = 1.0f / depth;
        result[12] = -(right + left) / width;
        result[13] = -(top + bottom) / height;
        result[14] = -nearZ / depth;
        result[15] = 1.0f;

        return result;
    }

    Mat4 orthographicScreen(f32 screenWidth, f32 screenHeight, f32 nearZ, f32 farZ) {
        Mat4 result;

        result[0] = 2.0f / screenWidth;
        result[12] = -1.0f;

        result[5] = -2.0f / screenHeight;
        result[13] = 1.0f;

        result[10] = 1.0f / (farZ - nearZ);
        result[14] = -nearZ / (farZ - nearZ);

        result[15] = 1.0f;
        return result;
    }

    Mat4 orthographic(f32 width, f32 height, f32 nearZ, f32 farZ) {
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

    Mat4 orthographicPixelSpace(f32 width, f32 height, f32 nearZ, f32 farZ) {
        Mat4 result;
        result[0] = 2.0f / width;
        result[5] = -2.0f / height;
        result[10] = -2.0f / (farZ - nearZ);
        result[12] = -1.0f;
        result[13] = 1.0f;
        result[14] = -(farZ + nearZ) / (farZ - nearZ);
        result[15] = 1.0f;
        return result;
    }

    Mat4 createScreenSpaceProjection(float screenWidth, float screenHeight) {
        return orthographic(screenWidth, screenHeight, -100.0f, 100.0f);
    }

    Mat4 translation(const Vec3& pos) {
        Mat4 result;
        result[12] = pos.x;
        result[13] = pos.y;
        result[14] = pos.z;
        return result;
    }

    Mat4 transposeMatrix(const Mat4& matrix) {
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

    Mat4 scale(const Vec3& scaleVec) {
        Mat4 result;
        result[0] = scaleVec.x;
        result[5] = scaleVec.y;
        result[10] = scaleVec.z;
        return result;
    }

    Mat4 rotationZ(f32 angle) {
        Mat4 result;
        f32 c = std::cos(angle);
        f32 s = std::sin(angle);
        result[0] = c;  result[1] = s;
        result[4] = -s; result[5] = c;
        return result;
    }

    Mat4 rotationY(f32 angle) {
        Mat4 result;
        f32 c = std::cos(angle);
        f32 s = std::sin(angle);
        result[0] = c;   result[2] = -s;
        result[8] = s;   result[10] = c;
        return result;
    }

    Mat4 rotationX(f32 angle) {
        Mat4 result;
        f32 c = std::cos(angle);
        f32 s = std::sin(angle);
        result[5] = c;   result[6] = s;
        result[9] = -s;  result[10] = c;
        return result;
    }

    Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
        Vec3 zaxis = (target - eye).normalized();
        Vec3 xaxis = up.cross(zaxis).normalized();
        Vec3 yaxis = zaxis.cross(xaxis);

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

    Mat4 perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ) {
#if defined(THEENGINE_PLATFORM_ANDROID)
        __android_log_print(ANDROID_LOG_INFO, "Math", "perspective: fovY=%f, aspect=%f, near=%f, far=%f",
                           fovY, aspect, nearZ, farZ);
#endif
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
#if defined(THEENGINE_PLATFORM_ANDROID)
        __android_log_print(ANDROID_LOG_INFO, "Math", "perspective result: [0]=%f [5]=%f [10]=%f [15]=%f",
                           result[0], result[5], result[10], result[15]);
#endif
        return result;
    }

    Mat4 multiply(const Mat4& a, const Mat4& b) {
        Mat4 result;
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result[row * 4 + col] = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    result[row * 4 + col] += a[row * 4 + k] * b[k * 4 + col];
                }
            }
        }
        return result;
    }
}
