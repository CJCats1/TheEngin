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
	class OpenGLESShaderBinary final : public IRenderShaderBinary
	{
	public:
		OpenGLESShaderBinary(const ShaderCompileDesc& desc, Logger& logger)
			: m_logger(logger), m_type(desc.shaderType)
		{
			if (!desc.shaderSourceName)
			{
				THEENGINE_LOG_THROW(m_logger, std::invalid_argument, Logger::LogLevel::Error,
					"No shader source name provided.");
			}
			if (!desc.shaderSourceCode)
			{
				THEENGINE_LOG_THROW(m_logger, std::invalid_argument, Logger::LogLevel::Error,
					"No shader source code provided.");
			}
			if (!desc.shaderSourceCodeSize)
			{
				THEENGINE_LOG_THROW(m_logger, std::invalid_argument, Logger::LogLevel::Error,
					"No shader source code size provided.");
			}

#if defined(THEENGINE_PLATFORM_ANDROID)
			const GLenum stage = (desc.shaderType == ShaderType::VertexShader) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
			m_shader = glCreateShader(stage);
			if (!m_shader)
			{
				THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
					"glCreateShader failed.");
			}

			const char* src = static_cast<const char*>(desc.shaderSourceCode);
			const GLint len = static_cast<GLint>(desc.shaderSourceCodeSize);
			glShaderSource(m_shader, 1, &src, &len);
			glCompileShader(m_shader);

			GLint success = 0;
			glGetShaderiv(m_shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				char infoLog[1024]{};
				glGetShaderInfoLog(m_shader, sizeof(infoLog), nullptr, infoLog);
				std::string errorMessage = "OpenGLES shader compilation failed: ";
				errorMessage += infoLog;
				THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
					errorMessage.c_str());
			}
#else
			THEENGINE_LOG_THROW(m_logger, std::runtime_error, Logger::LogLevel::Error,
				"OpenGLES backend only supported on Android.");
#endif
		}

		~OpenGLESShaderBinary() override
		{
#if defined(THEENGINE_PLATFORM_ANDROID)
			if (m_shader)
			{
				glDeleteShader(m_shader);
				m_shader = 0;
			}
#endif
		}

		BinaryData getData() const noexcept override
		{
			return { &m_shader, sizeof(m_shader) };
		}

		ShaderType getType() const noexcept override
		{
			return m_type;
		}

	private:
		Logger& m_logger;
		unsigned int m_shader{};
		ShaderType m_type{};
	};
}
