#include <TheEngine/Graphics/OpenGL/OpenGLContext.h>
#include <TheEngine/Graphics/GraphicsLogUtils.h>
#include <TheEngine/Graphics/OpenGL/OpenGLPipelineState.h>
#include <TheEngine/Graphics/OpenGL/OpenGLVertexBuffer.h>
#include <TheEngine/Graphics/OpenGL/OpenGLIndexBuffer.h>
#if defined(THEENGINE_ENABLE_OPENGL)
#include <glad/glad.h>
#endif

using namespace TheEngine;

OpenGLContext::OpenGLContext(Logger& logger)
	: m_logger(logger)
{
}

void OpenGLContext::clearAndSetBackBuffer(const IRenderSwapChain& swapChain, const Vec4& color)
{
	(void)swapChain;
#if defined(THEENGINE_ENABLE_OPENGL)
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
	(void)color;
#endif
}

void OpenGLContext::setGraphicsPipelineState(const IRenderPipelineState& pipeline)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	const auto* glPipeline = dynamic_cast<const OpenGLPipelineState*>(&pipeline);
	if (!glPipeline)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
			"Invalid pipeline state type for OpenGL context.");
	}
	m_program = glPipeline->getProgram();
	glUseProgram(m_program);

	// Bind default texture unit for "tex"
	const GLint texLoc = glGetUniformLocation(m_program, "tex");
	if (texLoc >= 0)
	{
		glUniform1i(texLoc, 0);
	}
	// Push current uniforms
	setWorldMatrix(m_worldMatrix);
	setViewMatrix(m_viewMatrix);
	setProjectionMatrix(m_projectionMatrix);
	setTint(m_tint);
#else
	(void)pipeline;
#endif
}

void OpenGLContext::setVertexBuffer(const IRenderVertexBuffer& buffer)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	const auto* glBuffer = dynamic_cast<const OpenGLVertexBuffer*>(&buffer);
	if (!glBuffer)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
			"Invalid vertex buffer type for OpenGL context.");
	}
	m_boundVao = glBuffer->getVao();
	glBindVertexArray(m_boundVao);
#else
	(void)buffer;
#endif
}

void OpenGLContext::setViewportSize(const Rect& size)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	glViewport(0, 0, size.width, size.height);
#else
	(void)size;
#endif
}

void OpenGLContext::drawTriangleList(ui32 vertexCount, ui32 startVertexLocation)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	glDrawArrays(GL_TRIANGLES, static_cast<GLint>(startVertexLocation), static_cast<GLsizei>(vertexCount));
#else
	(void)vertexCount;
	(void)startVertexLocation;
#endif
}

void OpenGLContext::setIndexBuffer(IRenderIndexBuffer& ib, IndexFormat fmt, ui32 offset)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	(void)fmt;
	const auto* glBuffer = dynamic_cast<const OpenGLIndexBuffer*>(&ib);
	if (!glBuffer)
	{
		THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
			"Invalid index buffer type for OpenGL context.");
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

void OpenGLContext::drawIndexedTriangleList(ui32 indexCount, ui32 startIndex)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	const GLenum indexType = m_indexType ? m_indexType : GL_UNSIGNED_INT;
	const ui32 indexSize = (indexType == GL_UNSIGNED_SHORT) ? sizeof(unsigned short) : sizeof(ui32);
	glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount), indexType,
		reinterpret_cast<const void*>(static_cast<uintptr_t>(startIndex * indexSize)));
#else
	(void)indexCount;
	(void)startIndex;
#endif
}

void OpenGLContext::drawIndexedLineList(ui32 indexCount, ui32 startIndex)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	const GLenum indexType = m_indexType ? m_indexType : GL_UNSIGNED_INT;
	const ui32 indexSize = (indexType == GL_UNSIGNED_SHORT) ? sizeof(unsigned short) : sizeof(ui32);
	glDrawElements(GL_LINES, static_cast<GLsizei>(indexCount), indexType,
		reinterpret_cast<const void*>(static_cast<uintptr_t>(startIndex * indexSize)));
#else
	(void)indexCount;
	(void)startIndex;
#endif
}

void OpenGLContext::setTexture(ui32 slot, NativeGraphicsHandle srv)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	const auto textureId = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(srv));
	glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, textureId);
#else
	(void)slot;
	(void)srv;
#endif
}

void OpenGLContext::setSampler(ui32 slot, NativeGraphicsHandle sampler)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	(void)slot;
	(void)sampler;
#else
	(void)slot;
	(void)sampler;
#endif
}

void OpenGLContext::setWorldMatrix(const Mat4& worldMatrix)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	m_worldMatrix = worldMatrix;
	if (m_program)
	{
		const GLint loc = glGetUniformLocation(m_program, "worldMatrix");
		if (loc >= 0)
		{
			glUniformMatrix4fv(loc, 1, GL_FALSE, m_worldMatrix.data());
		}
	}
#else
	(void)worldMatrix;
#endif
}

void OpenGLContext::setViewMatrix(const Mat4& viewMatrix)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	m_viewMatrix = viewMatrix;
	if (m_program)
	{
		const GLint loc = glGetUniformLocation(m_program, "viewMatrix");
		if (loc >= 0)
		{
			glUniformMatrix4fv(loc, 1, GL_FALSE, m_viewMatrix.data());
		}
	}
#else
	(void)viewMatrix;
#endif
}

void OpenGLContext::setProjectionMatrix(const Mat4& projectionMatrix)
{
#if defined(THEENGINE_ENABLE_OPENGL)
	m_projectionMatrix = projectionMatrix;
	if (m_program)
	{
		const GLint loc = glGetUniformLocation(m_program, "projectionMatrix");
		if (loc >= 0)
		{
			glUniformMatrix4fv(loc, 1, GL_FALSE, m_projectionMatrix.data());
		}
	}
#else
	(void)projectionMatrix;
#endif
}

void OpenGLContext::updateTransformBuffer()
{
}

void OpenGLContext::enableAlphaBlending()
{
#if defined(THEENGINE_ENABLE_OPENGL)
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
#endif
}

void OpenGLContext::disableAlphaBlending()
{
#if defined(THEENGINE_ENABLE_OPENGL)
	glDisable(GL_BLEND);
#endif
}

void OpenGLContext::enableTransparentDepth()
{
#if defined(THEENGINE_ENABLE_OPENGL)
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
#endif
}

void OpenGLContext::enableDefaultDepth()
{
#if defined(THEENGINE_ENABLE_OPENGL)
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
#endif
}

void OpenGLContext::setScreenSpaceMatrices(float screenWidth, float screenHeight)
{
	(void)screenWidth;
	(void)screenHeight;
}

void OpenGLContext::restoreWorldSpaceMatrices(const Mat4& viewMatrix, const Mat4& projectionMatrix)
{
	(void)viewMatrix;
	(void)projectionMatrix;
}

void OpenGLContext::setTint(const Vec4& tint)
{
#if defined(THEENGINE_ENABLE_OPENGL)
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

void OpenGLContext::setDirectionalLight(const Vec3& direction, const Vec3& color, float intensity, float ambient)
{
	(void)direction;
	(void)color;
	(void)intensity;
	(void)ambient;
}

void OpenGLContext::setLights(const std::vector<Vec3>& dirs, const std::vector<Vec3>& colors,
	const std::vector<float>& intensities)
{
	(void)dirs;
	(void)colors;
	(void)intensities;
}

void OpenGLContext::setMaterial(const Vec3& specColor, float shininess, float ambient)
{
	(void)specColor;
	(void)shininess;
	(void)ambient;
}

void OpenGLContext::setCameraPosition(const Vec3& pos)
{
	(void)pos;
}

void OpenGLContext::setPBR(bool enabled, const Vec3& albedo, float metallic, float roughness)
{
	(void)enabled;
	(void)albedo;
	(void)metallic;
	(void)roughness;
}

void OpenGLContext::setSpotlight(bool enabled, const Vec3& position, const Vec3& direction, float range,
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

void OpenGLContext::disableDepthTest()
{
#if defined(THEENGINE_ENABLE_OPENGL)
	glDisable(GL_DEPTH_TEST);
#endif
}

void OpenGLContext::enableDepthTest()
{
#if defined(THEENGINE_ENABLE_OPENGL)
	glEnable(GL_DEPTH_TEST);
#endif
}

void OpenGLContext::setPSConstants0(const void* data, ui32 byteSize)
{
	(void)data;
	(void)byteSize;
}

void OpenGLContext::setShadowMap(NativeGraphicsHandle shadowMap, NativeGraphicsHandle shadowSampler)
{
	(void)shadowMap;
	(void)shadowSampler;
}

void OpenGLContext::setShadowMaps(NativeGraphicsHandle const* shadowMaps, ui32 count, NativeGraphicsHandle shadowSampler)
{
	(void)shadowMaps;
	(void)count;
	(void)shadowSampler;
}

void OpenGLContext::setShadowMatrix(const Mat4& lightViewProj)
{
	(void)lightViewProj;
}

void OpenGLContext::setShadowMatrices(const std::vector<Mat4>& lightViewProjMatrices)
{
	(void)lightViewProjMatrices;
}

NativeGraphicsHandle OpenGLContext::getDefaultSamplerHandle() const
{
	return nullptr;
}

NativeGraphicsHandle OpenGLContext::getNativeContextHandle() const noexcept
{
	return nullptr;
}

void OpenGLContext::clearShaderResourceBindings()
{
}
