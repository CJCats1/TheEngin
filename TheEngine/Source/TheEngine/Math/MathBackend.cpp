#include <TheEngine/Math/Backend/MathBackend.h>

#if defined(THEENGINE_USE_GLM) && !__has_include(<glm/glm.hpp>)
#undef THEENGINE_USE_GLM
#endif

namespace TheEngine::math::backend
{
	namespace default_impl
	{
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
#if defined(THEENGINE_USE_GLM)
	namespace glm_impl
	{
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
#endif

	namespace
	{
		MathBackendType g_backend = MathBackendType::Default;

		bool useGlmBackend()
		{
#if defined(THEENGINE_USE_GLM)
			return g_backend == MathBackendType::Glm;
#else
			return false;
#endif
		}
	}

	void setBackend(MathBackendType type)
	{
		g_backend = type;
	}

	MathBackendType getBackend()
	{
		return g_backend;
	}

	Mat4 identity() {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::identity();
		}
		#endif
		return default_impl::identity();
	}

	Mat4 orthographicLH(f32 left, f32 right, f32 bottom, f32 top, f32 nearZ, f32 farZ) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::orthographicLH(left, right, bottom, top, nearZ, farZ);
		}
		#endif
		return default_impl::orthographicLH(left, right, bottom, top, nearZ, farZ);
	}

	Mat4 orthographicScreen(f32 screenWidth, f32 screenHeight, f32 nearZ, f32 farZ) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::orthographicScreen(screenWidth, screenHeight, nearZ, farZ);
		}
		#endif
		return default_impl::orthographicScreen(screenWidth, screenHeight, nearZ, farZ);
	}

	Mat4 orthographic(f32 width, f32 height, f32 nearZ, f32 farZ) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::orthographic(width, height, nearZ, farZ);
		}
		#endif
		return default_impl::orthographic(width, height, nearZ, farZ);
	}

	Mat4 orthographicPixelSpace(f32 width, f32 height, f32 nearZ, f32 farZ) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::orthographicPixelSpace(width, height, nearZ, farZ);
		}
		#endif
		return default_impl::orthographicPixelSpace(width, height, nearZ, farZ);
	}

	Mat4 createScreenSpaceProjection(float screenWidth, float screenHeight) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::createScreenSpaceProjection(screenWidth, screenHeight);
		}
		#endif
		return default_impl::createScreenSpaceProjection(screenWidth, screenHeight);
	}

	Mat4 translation(const Vec3& pos) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::translation(pos);
		}
		#endif
		return default_impl::translation(pos);
	}

	Mat4 transposeMatrix(const Mat4& matrix) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::transposeMatrix(matrix);
		}
		#endif
		return default_impl::transposeMatrix(matrix);
	}

	Mat4 scale(const Vec3& scaleVec) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::scale(scaleVec);
		}
		#endif
		return default_impl::scale(scaleVec);
	}

	Mat4 rotationZ(f32 angle) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::rotationZ(angle);
		}
		#endif
		return default_impl::rotationZ(angle);
	}

	Mat4 rotationY(f32 angle) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::rotationY(angle);
		}
		#endif
		return default_impl::rotationY(angle);
	}

	Mat4 rotationX(f32 angle) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::rotationX(angle);
		}
		#endif
		return default_impl::rotationX(angle);
	}

	Mat4 lookAt(const Vec3& eye, const Vec3& target, const Vec3& up) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::lookAt(eye, target, up);
		}
		#endif
		return default_impl::lookAt(eye, target, up);
	}

	Mat4 perspective(f32 fovY, f32 aspect, f32 nearZ, f32 farZ) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::perspective(fovY, aspect, nearZ, farZ);
		}
		#endif
		return default_impl::perspective(fovY, aspect, nearZ, farZ);
	}

	Mat4 multiply(const Mat4& a, const Mat4& b) {
		#if defined(THEENGINE_USE_GLM)
		if (useGlmBackend())
		{
			return glm_impl::multiply(a, b);
		}
		#endif
		return default_impl::multiply(a, b);
	}
}
