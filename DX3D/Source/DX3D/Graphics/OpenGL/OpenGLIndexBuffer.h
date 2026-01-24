#pragma once
#include <DX3D/Core/Common.h>
#include <DX3D/Graphics/Abstraction/RenderResources.h>
#include <DX3D/Core/Logger.h>

#if defined(DX3D_ENABLE_OPENGL)
#include <glad/glad.h>
#endif

namespace dx3d
{
	class OpenGLIndexBuffer final : public IRenderIndexBuffer
	{
	public:
		OpenGLIndexBuffer(const IndexBufferDesc& desc, Logger& logger)
			: m_logger(logger), m_count(desc.count), m_stride(desc.stride)
		{
			if (!desc.data || !desc.count || !desc.stride)
			{
				DX3DLogThrow(m_logger, std::invalid_argument, Logger::LogLevel::Error,
					"Invalid index buffer description.");
			}
#if defined(DX3D_ENABLE_OPENGL)
			glGenBuffers(1, &m_ebo);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
			const GLsizeiptr bufferSize = static_cast<GLsizeiptr>(desc.count) * desc.stride;
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize, desc.data, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			m_indexType = (desc.stride == sizeof(unsigned short)) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
#else
			(void)desc;
#endif
		}

		~OpenGLIndexBuffer() override
		{
#if defined(DX3D_ENABLE_OPENGL)
			if (m_ebo)
			{
				glDeleteBuffers(1, &m_ebo);
				m_ebo = 0;
			}
#endif
		}

		ui32 getCount() const noexcept override { return m_count; }
		ui32 getStride() const noexcept override { return m_stride; }

		unsigned int getBuffer() const noexcept { return m_ebo; }
		unsigned int getIndexType() const noexcept { return m_indexType; }

	private:
		Logger& m_logger;
		ui32 m_count{};
		ui32 m_stride{};
		unsigned int m_ebo{};
		unsigned int m_indexType{};
	};
}
