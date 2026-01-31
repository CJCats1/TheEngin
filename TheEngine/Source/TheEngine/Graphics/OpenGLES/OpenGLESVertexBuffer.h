#pragma once
#include <TheEngine/Core/Common.h>
#include <TheEngine/Graphics/Abstraction/RenderResources.h>
#include <TheEngine/Math/Geometry.h>
#include <TheEngine/Core/Logger.h>
#include <cstddef>

#if defined(THEENGINE_PLATFORM_ANDROID)
#include <GLES3/gl3.h>
#endif

namespace TheEngine
{
	class OpenGLESVertexBuffer final : public IRenderVertexBuffer
	{
	public:
		OpenGLESVertexBuffer(const VertexBufferDesc& desc, Logger& logger)
			: m_logger(logger), m_vertexSize(desc.vertexSize), m_vertexListSize(desc.vertexListSize),
			  m_isDynamic(desc.isDynamic)
		{
			if (!desc.vertexList || !desc.vertexListSize || !desc.vertexSize)
			{
				THEENGINE_LOG_THROW(m_logger, std::invalid_argument, Logger::LogLevel::Error,
					"Invalid vertex buffer description.");
			}
#if defined(THEENGINE_PLATFORM_ANDROID)
			glGenVertexArrays(1, &m_vao);
			glGenBuffers(1, &m_vbo);

			glBindVertexArray(m_vao);
			glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
			const GLsizeiptr bufferSize = static_cast<GLsizeiptr>(desc.vertexListSize) * desc.vertexSize;
			glBufferData(GL_ARRAY_BUFFER, bufferSize, desc.vertexList,
				m_isDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

			const GLsizei stride = static_cast<GLsizei>(sizeof(Vertex));
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, pos)));
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, normal)));
			glEnableVertexAttribArray(2);
			glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, uv)));
			glEnableVertexAttribArray(3);
			glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<void*>(offsetof(Vertex, color)));

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
#else
			(void)desc;
#endif
		}

		~OpenGLESVertexBuffer() override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			if (m_vbo)
			{
				glDeleteBuffers(1, &m_vbo);
				m_vbo = 0;
			}
			if (m_vao)
			{
				glDeleteVertexArrays(1, &m_vao);
				m_vao = 0;
			}
#endif
		}

		ui32 getVertexListSize() const noexcept override { return m_vertexListSize; }
		ui32 getVertexSize() const noexcept override { return m_vertexSize; }

		unsigned int getVao() const noexcept { return m_vao; }
		unsigned int getVbo() const noexcept { return m_vbo; }

	private:
		Logger& m_logger;
		ui32 m_vertexSize{};
		ui32 m_vertexListSize{};
		bool m_isDynamic{};
		unsigned int m_vao{};
		unsigned int m_vbo{};
	};
}
