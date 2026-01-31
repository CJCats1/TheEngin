#pragma once
#include <TheEngine/Core/Common.h>
#include <TheEngine/Core/Base.h>
#include <TheEngine/Graphics/Abstraction/RenderResources.h>
#include <TheEngine/Core/Logger.h>
#include <string>

#if defined(THEENGINE_PLATFORM_ANDROID)
#include <GLES3/gl3.h>
#endif

namespace TheEngine
{
	class OpenGLESPipelineState final : public IRenderPipelineState
	{
	public:
		OpenGLESPipelineState(const GraphicsPipelineStateDesc& desc, Logger& logger)
			: m_logger(logger)
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			if (desc.ps.getType() != ShaderType::PixelShader)
			{
				THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
					"OpenGLES pipeline expects a pixel shader for the fragment stage.");
			}
			const BinaryData vsData = desc.vs.getShaderBinaryData();
			const BinaryData psData = desc.ps.getData();
			if (!vsData.data || !psData.data ||
				vsData.dataSize != sizeof(unsigned int) || psData.dataSize != sizeof(unsigned int))
			{
				THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
					"OpenGLES pipeline expects OpenGLES shader binaries.");
			}
			const auto* vsHandle = static_cast<const unsigned int*>(vsData.data);
			const auto* psHandle = static_cast<const unsigned int*>(psData.data);

			m_program = glCreateProgram();
			if (!m_program)
			{
				THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
					"glCreateProgram failed.");
			}

			glAttachShader(m_program, *vsHandle);
			glAttachShader(m_program, *psHandle);
			glLinkProgram(m_program);

			GLint linked = 0;
			glGetProgramiv(m_program, GL_LINK_STATUS, &linked);
			if (!linked)
			{
				char infoLog[1024]{};
				glGetProgramInfoLog(m_program, sizeof(infoLog), nullptr, infoLog);
				std::string errorMessage = "OpenGLES program link failed: ";
				errorMessage += infoLog;
				THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
					errorMessage.c_str());
			}
#else
			(void)desc;
			THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
				"OpenGLES backend only supported on Android.");
#endif
		}

		~OpenGLESPipelineState() override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			if (m_program)
			{
				glDeleteProgram(m_program);
				m_program = 0;
			}
#endif
		}

		unsigned int getProgram() const noexcept { return m_program; }

	private:
		Logger& m_logger;
		unsigned int m_program{};
	};
}
