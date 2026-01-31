#pragma once
#include <TheEngine/Core/Common.h>
#include <TheEngine/Graphics/Abstraction/RenderResources.h>
#include <TheEngine/Core/Logger.h>

#if defined(THEENGINE_PLATFORM_ANDROID)
#include <GLES3/gl3.h>
#endif

namespace TheEngine
{
	class OpenGLESIndexBuffer final : public IRenderIndexBuffer
	{
	public:
		OpenGLESIndexBuffer(const IndexBufferDesc& desc, Logger& logger)
			: m_logger(logger), m_indexCount(desc.count), m_stride(desc.stride)
		{
			if (!desc.data || !desc.count || !desc.stride)
			{
				THEENGINE_LOG_THROW(m_logger, std::invalid_argument, Logger::LogLevel::Error,
					"Invalid index buffer description.");
			}
#if defined(THEENGINE_PLATFORM_ANDROID)
			glGenBuffers(1, &m_buffer);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buffer);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(desc.count * desc.stride),
				desc.data, GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			m_indexType = (desc.stride == sizeof(unsigned short)) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
#else
			(void)desc;
#endif
		}

		~OpenGLESIndexBuffer() override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			if (m_buffer)
			{
				glDeleteBuffers(1, &m_buffer);
				m_buffer = 0;
			}
#endif
		}

		ui32 getCount() const noexcept override { return m_indexCount; }
		ui32 getStride() const noexcept override { return m_stride; }
		unsigned int getBuffer() const noexcept { return m_buffer; }
		unsigned int getIndexType() const noexcept { return m_indexType; }

	private:
		Logger& m_logger;
		ui32 m_indexCount{};
		ui32 m_stride{};
		unsigned int m_buffer{};
		unsigned int m_indexType{};
	};
}
