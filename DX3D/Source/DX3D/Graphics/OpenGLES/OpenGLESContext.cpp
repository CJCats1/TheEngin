#include <DX3D/Graphics/OpenGLES/OpenGLESContext.h>
#include <DX3D/Graphics/GraphicsLogUtils.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESPipelineState.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESVertexBuffer.h>
#include <DX3D/Graphics/OpenGLES/OpenGLESIndexBuffer.h>
#include <iostream>
#if defined(DX3D_PLATFORM_ANDROID)
#include <GLES3/gl3.h>
#include <android/log.h>
#endif

using namespace dx3d;

OpenGLESContext::OpenGLESContext(Logger& logger)
	: m_logger(logger)
{
}

void OpenGLESContext::clearAndSetBackBuffer(const IRenderSwapChain& swapChain, const Vec4& color)
{
	(void)swapChain;
#if defined(DX3D_PLATFORM_ANDROID)
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
	(void)color;
#endif
}

void OpenGLESContext::setGraphicsPipelineState(const IRenderPipelineState& pipeline)
{
#if defined(DX3D_PLATFORM_ANDROID)
	const auto* glPipeline = dynamic_cast<const OpenGLESPipelineState*>(&pipeline);
	if (!glPipeline)
	{
		DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
			"Invalid pipeline state type for OpenGLES context.");
	}
	m_program = glPipeline->getProgram();
	glUseProgram(m_program);

	const GLint texLoc = glGetUniformLocation(m_program, "tex");
	if (texLoc >= 0)
	{
		glUniform1i(texLoc, 0);
	}
	setWorldMatrix(m_worldMatrix);
	setViewMatrix(m_viewMatrix);
	setProjectionMatrix(m_projectionMatrix);
	setTint(m_tint);
#else
	(void)pipeline;
#endif
}

void OpenGLESContext::setVertexBuffer(const IRenderVertexBuffer& buffer)
{
#if defined(DX3D_PLATFORM_ANDROID)
	const auto* glBuffer = dynamic_cast<const OpenGLESVertexBuffer*>(&buffer);
	if (!glBuffer)
	{
		DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
			"Invalid vertex buffer type for OpenGLES context.");
	}
	m_boundVao = glBuffer->getVao();
	glBindVertexArray(m_boundVao);
#else
	(void)buffer;
#endif
}

void OpenGLESContext::setViewportSize(const Rect& size)
{
#if defined(DX3D_PLATFORM_ANDROID)
	// Removed verbose logging to reduce logcat spam
	glViewport(0, 0, size.width, size.height);
#else
	(void)size;
#endif
}

void OpenGLESContext::drawTriangleList(ui32 vertexCount, ui32 startVertexLocation)
{
#if defined(DX3D_PLATFORM_ANDROID)
	glDrawArrays(GL_TRIANGLES, static_cast<GLint>(startVertexLocation), static_cast<GLsizei>(vertexCount));
#else
	(void)vertexCount;
	(void)startVertexLocation;
#endif
}

void OpenGLESContext::setIndexBuffer(IRenderIndexBuffer& ib, IndexFormat fmt, ui32 offset)
{
#if defined(DX3D_PLATFORM_ANDROID)
	(void)fmt;
	const auto* glBuffer = dynamic_cast<const OpenGLESIndexBuffer*>(&ib);
	if (!glBuffer)
	{
		DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
			"Invalid index buffer type for OpenGLES context.");
	}
	m_indexType = glBuffer->getIndexType();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glBuffer->getBuffer());
	(void)offset;
#else
	(void)ib;
	(void)fmt;
	(void)offset;
#endif
}

void OpenGLESContext::drawIndexedTriangleList(ui32 indexCount, ui32 startIndex)
{
#if defined(DX3D_PLATFORM_ANDROID)
	const GLenum indexType = m_indexType ? m_indexType : GL_UNSIGNED_INT;
	const ui32 indexSize = (indexType == GL_UNSIGNED_SHORT) ? sizeof(unsigned short) : sizeof(ui32);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), indexType,
		reinterpret_cast<const void*>(static_cast<uintptr_t>(startIndex * indexSize)));
#else
	(void)indexCount;
	(void)startIndex;
#endif
}

void OpenGLESContext::drawIndexedLineList(ui32 indexCount, ui32 startIndex)
{
#if defined(DX3D_PLATFORM_ANDROID)
	const GLenum indexType = m_indexType ? m_indexType : GL_UNSIGNED_INT;
	const ui32 indexSize = (indexType == GL_UNSIGNED_SHORT) ? sizeof(unsigned short) : sizeof(ui32);
	glDrawElements(GL_LINES, static_cast<GLsizei>(indexCount), indexType,
		reinterpret_cast<const void*>(static_cast<uintptr_t>(startIndex * indexSize)));
#else
	(void)indexCount;
	(void)startIndex;
#endif
}

void OpenGLESContext::setTexture(ui32 slot, NativeGraphicsHandle srv)
{
#if defined(DX3D_PLATFORM_ANDROID)
	const auto textureId = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(srv));
	if (textureId == 0) {
		__android_log_print(ANDROID_LOG_ERROR, "OpenGLESContext", "setTexture: WARNING - textureId is 0 (invalid texture)!");
		return;
	}
	
	// Ensure shader program is active before binding texture
	if (m_program == 0) {
		__android_log_print(ANDROID_LOG_WARN, "OpenGLESContext", "setTexture: No shader program active!");
	}
	
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureId);
	
	// Verify the texture is actually bound
	GLint boundTexture = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture);
	if (static_cast<unsigned int>(boundTexture) != textureId) {
		__android_log_print(ANDROID_LOG_ERROR, "OpenGLESContext", "setTexture: FAILED to bind texture! Expected %u, got %d", textureId, boundTexture);
	} else {
		// Texture binding verified successfully
		// Note: Cannot query texture dimensions in OpenGL ES (GL_TEXTURE_WIDTH/HEIGHT not available)
		// Removed verbose success logging to reduce logcat spam
		
		// Verify the shader uniform is set correctly
		if (m_program != 0) {
			const GLint texLoc = glGetUniformLocation(m_program, "tex");
			if (texLoc >= 0) {
				// Ensure uniform is set to the correct texture unit
				glUniform1i(texLoc, slot);
				GLint currentTexUnit = -1;
				glGetUniformiv(m_program, texLoc, &currentTexUnit);
				static int uniformSetCount = 0;
				if (uniformSetCount < 3) {
					__android_log_print(ANDROID_LOG_INFO, "OpenGLESContext", "setTexture: Uniform 'tex' found at location %d, set to unit %u (verified: %d)", texLoc, slot, currentTexUnit);
					uniformSetCount++;
				}
				if (currentTexUnit != static_cast<GLint>(slot)) {
					__android_log_print(ANDROID_LOG_WARN, "OpenGLESContext", "setTexture: Shader uniform 'tex' expects unit %d, but texture is bound to unit %u", currentTexUnit, slot);
				}
			} else {
				static int uniformNotFoundCount = 0;
				if (uniformNotFoundCount < 3) {
					__android_log_print(ANDROID_LOG_ERROR, "OpenGLESContext", "setTexture: Shader uniform 'tex' not found! Program: %u", m_program);
					uniformNotFoundCount++;
				}
			}
		} else {
			static int noProgramCount = 0;
			if (noProgramCount < 3) {
				__android_log_print(ANDROID_LOG_WARN, "OpenGLESContext", "setTexture: No shader program active (m_program=0)!");
				noProgramCount++;
			}
		}
	}
#else
	(void)slot;
	(void)srv;
#endif
}

void OpenGLESContext::setSampler(ui32 slot, NativeGraphicsHandle sampler)
{
#if defined(DX3D_PLATFORM_ANDROID)
	(void)slot;
	(void)sampler;
#else
	(void)slot;
	(void)sampler;
#endif
}

void OpenGLESContext::setWorldMatrix(const Mat4& worldMatrix)
{
#if defined(DX3D_PLATFORM_ANDROID)
	m_worldMatrix = worldMatrix;
	if (m_program)
	{
		const GLint loc = glGetUniformLocation(m_program, "worldMatrix");
		if (loc >= 0)
		{
			// Transpose because shader uses row-vector multiplication (vec * mat)
			glUniformMatrix4fv(loc, 1, GL_TRUE, m_worldMatrix.data());
		}
	}
#else
	(void)worldMatrix;
#endif
}

void OpenGLESContext::setViewMatrix(const Mat4& viewMatrix)
{
#if defined(DX3D_PLATFORM_ANDROID)
	m_viewMatrix = viewMatrix;
	if (m_program)
	{
		const GLint loc = glGetUniformLocation(m_program, "viewMatrix");
		if (loc >= 0)
		{
			// Transpose because shader uses row-vector multiplication (vec * mat)
			glUniformMatrix4fv(loc, 1, GL_TRUE, m_viewMatrix.data());
		}
	}
#else
	(void)viewMatrix;
#endif
}

void OpenGLESContext::setProjectionMatrix(const Mat4& projectionMatrix)
{
#if defined(DX3D_PLATFORM_ANDROID)
	m_projectionMatrix = projectionMatrix;
	if (m_program)
	{
		const GLint loc = glGetUniformLocation(m_program, "projectionMatrix");
		if (loc >= 0)
		{
			// Transpose because shader uses row-vector multiplication (vec * mat)
			glUniformMatrix4fv(loc, 1, GL_TRUE, m_projectionMatrix.data());
		}
	}
	
	// Removed verbose projection matrix logging to reduce logcat spam
#else
	(void)projectionMatrix;
#endif
}

void OpenGLESContext::updateTransformBuffer()
{
}

void OpenGLESContext::enableAlphaBlending()
{
#if defined(DX3D_PLATFORM_ANDROID)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}

void OpenGLESContext::disableAlphaBlending()
{
#if defined(DX3D_PLATFORM_ANDROID)
	glDisable(GL_BLEND);
#endif
}

void OpenGLESContext::enableTransparentDepth()
{
#if defined(DX3D_PLATFORM_ANDROID)
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
#endif
}

void OpenGLESContext::enableDefaultDepth()
{
#if defined(DX3D_PLATFORM_ANDROID)
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
#endif
}

void OpenGLESContext::setScreenSpaceMatrices(float screenWidth, float screenHeight)
{
	(void)screenWidth;
	(void)screenHeight;
}

void OpenGLESContext::restoreWorldSpaceMatrices(const Mat4& viewMatrix, const Mat4& projectionMatrix)
{
	(void)viewMatrix;
	(void)projectionMatrix;
}

void OpenGLESContext::setTint(const Vec4& tint)
{
#if defined(DX3D_PLATFORM_ANDROID)
	m_tint = tint;
	if (m_program)
	{
		const GLint loc = glGetUniformLocation(m_program, "tintColor");
		if (loc >= 0)
		{
			glUniform4f(loc, m_tint.x, m_tint.y, m_tint.z, m_tint.w);
		}
	}
#else
	(void)tint;
#endif
}

void OpenGLESContext::setDirectionalLight(const Vec3& direction, const Vec3& color, float intensity, float ambient)
{
	(void)direction;
	(void)color;
	(void)intensity;
	(void)ambient;
}

void OpenGLESContext::setLights(const std::vector<Vec3>& dirs, const std::vector<Vec3>& colors,
	const std::vector<float>& intensities)
{
	(void)dirs;
	(void)colors;
	(void)intensities;
}

void OpenGLESContext::setMaterial(const Vec3& specColor, float shininess, float ambient)
{
	(void)specColor;
	(void)shininess;
	(void)ambient;
}

void OpenGLESContext::setCameraPosition(const Vec3& pos)
{
	(void)pos;
}

void OpenGLESContext::setPBR(bool enabled, const Vec3& albedo, float metallic, float roughness)
{
	(void)enabled;
	(void)albedo;
	(void)metallic;
	(void)roughness;
}

void OpenGLESContext::setSpotlight(bool enabled, const Vec3& position, const Vec3& direction, float range,
	float innerAngleRadians, float outerAngleRadians, const Vec3& color, float intensity)
{
	(void)enabled;
	(void)position;
	(void)direction;
	(void)range;
	(void)innerAngleRadians;
	(void)outerAngleRadians;
	(void)color;
	(void)intensity;
}

void OpenGLESContext::disableDepthTest()
{
#if defined(DX3D_PLATFORM_ANDROID)
	glDisable(GL_DEPTH_TEST);
#endif
}

void OpenGLESContext::enableDepthTest()
{
#if defined(DX3D_PLATFORM_ANDROID)
	glEnable(GL_DEPTH_TEST);
#endif
}

void OpenGLESContext::setShadowMap(NativeGraphicsHandle shadowMap, NativeGraphicsHandle shadowSampler)
{
	(void)shadowMap;
	(void)shadowSampler;
}

void OpenGLESContext::setShadowMaps(NativeGraphicsHandle const* shadowMaps, ui32 count, NativeGraphicsHandle shadowSampler)
{
	(void)shadowMaps;
	(void)count;
	(void)shadowSampler;
}

void OpenGLESContext::setShadowMatrix(const Mat4& lightViewProj)
{
	(void)lightViewProj;
}

void OpenGLESContext::setShadowMatrices(const std::vector<Mat4>& lightViewProjMatrices)
{
	(void)lightViewProjMatrices;
}

NativeGraphicsHandle OpenGLESContext::getDefaultSamplerHandle() const
{
	return nullptr;
}

NativeGraphicsHandle OpenGLESContext::getNativeContextHandle() const noexcept
{
	return nullptr;
}

void OpenGLESContext::setPSConstants0(const void* data, ui32 byteSize)
{
	(void)data;
	(void)byteSize;
}

void OpenGLESContext::clearShaderResourceBindings()
{
}
