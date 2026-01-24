#include <DX3D/Math/Backend/MathBackend.h>

#if defined(DX3D_USE_GLM) && !__has_include(<glm/glm.hpp>)
#undef DX3D_USE_GLM
#endif

#if defined(DX3D_USE_GLM)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace dx3d::math::backend::glm_impl
{
	static glm::vec3 toGlm(const Vec3& v) { return { v.x, v.y, v.z }; }
	static glm::mat4 toGlm(const Mat4& m) {
		glm::mat4 out(1.0f);
		const f32* src = m.data();
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				out[col][row] = src[row * 4 + col];
			}
		}
		return out;
	}
	static Mat4 fromGlm(const glm::mat4& m) {
		Mat4 out;
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				out[row * 4 + col] = m[col][row];
			}
		}
		return out;
	}

	Mat4 identity() { return Mat4(); }
	Mat4 orthographicLH(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ) {
		return fromGlm(glm::orthoLH(left, right, bottom, top, nearZ, farZ));
	}
	Mat4 orthographicScreen(f32 screenWidth, f32 screenHeight, f32 nearZ, f32 farZ) {
		return fromGlm(glm::orthoLH(0.0f, screenWidth, screenHeight, 0.0f, nearZ, farZ));
	}
	Mat4 orthographic(f32 width, f32 height, f32 nearZ, f32 farZ) {
		float left = -width * 0.5f;
		float right = width * 0.5f;
		float bottom = -height * 0.5f;
		float top = height * 0.5f;
		return fromGlm(glm::orthoLH(left, right, bottom, top, nearZ, farZ));
	}
	Mat4 orthographicPixelSpace(f32 width, f32 height, f32 nearZ, f32 farZ) {
		return fromGlm(glm::orthoLH(0.0f, width, height, 0.0f, nearZ, farZ));
	}
	Mat4 createScreenSpaceProjection(float screenWidth, float screenHeight) {
		return orthographic(screenWidth, screenHeight, -100.0f, 100.0f);
	}
	Mat4 translation(const Vec3& pos) { return fromGlm(glm::translate(glm::mat4(1.0f), toGlm(pos))); }
	Mat4 transposeMatrix(const Mat4& matrix) { return fromGlm(glm::transpose(toGlm(matrix))); }
	Mat4 scale(const Vec3& scaleVec) { return fromGlm(glm::scale(glm::mat4(1.0f), toGlm(scaleVec))); }
	Mat4 rotationZ(f32 angle) { return fromGlm(glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 0, 1))); }
	Mat4 rotationY(f32 angle) { return fromGlm(glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0, 1, 0))); }
	Mat4 rotationX(f32 angle) { return fromGlm(glm::rotate(glm::mat4(1.0f), angle, glm::vec3(1, 0, 0))); }
	Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
		return fromGlm(glm::lookAtLH(toGlm(eye), toGlm(target), toGlm(up)));
	}
	Mat4 perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ) {
		return fromGlm(glm::perspectiveLH(fovY, aspect, nearZ, farZ));
	}
	Mat4 multiply(const Mat4& a, const Mat4& b) {
		return fromGlm(toGlm(a) * toGlm(b));
	}
}
#endif
