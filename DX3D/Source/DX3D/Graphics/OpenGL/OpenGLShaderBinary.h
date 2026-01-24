#pragma once
#include <DX3D/Core/Common.h>
#include <DX3D/Core/Base.h>
#include <DX3D/Graphics/Abstraction/RenderResources.h>
#include <DX3D/Core/Logger.h>
#include <string>

#if defined(DX3D_ENABLE_OPENGL)
#include <glad/glad.h>
#endif

namespace dx3d
{
	class OpenGLShaderBinary final : public IRenderShaderBinary
	{
	public:
		OpenGLShaderBinary(const ShaderCompileDesc& desc, Logger& logger)
			: m_logger(logger), m_type(desc.shaderType)
		{
			if (!desc.shaderSourceName)
			{
				DX3DLogThrow(m_logger, std::invalid_argument, Logger::LogLevel::Error,
					"No shader source name provided.");
			}
			if (!desc.shaderSourceCode)
			{
				DX3DLogThrow(m_logger, std::invalid_argument, Logger::LogLevel::Error,
					"No shader source code provided.");
			}
			if (!desc.shaderSourceCodeSize)
			{
				DX3DLogThrow(m_logger, std::invalid_argument, Logger::LogLevel::Error,
					"No shader source code size provided.");
			}

#if defined(DX3D_ENABLE_OPENGL)
			const GLenum stage = (desc.shaderType == ShaderType::VertexShader) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
			m_shader = glCreateShader(stage);
			if (!m_shader)
			{
				DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
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
				std::string errorMessage = "OpenGL shader compilation failed: ";
				errorMessage += infoLog;
				DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
					errorMessage.c_str());
			}
#else
			DX3DLogThrow(m_logger, std::runtime_error, Logger::LogLevel::Error,
				"OpenGL backend not enabled. Define DX3D_ENABLE_OPENGL.");
#endif
		}

		~OpenGLShaderBinary() override
		{
#if defined(DX3D_ENABLE_OPENGL)
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

		unsigned int getShader() const noexcept { return m_shader; }

	private:
		Logger& m_logger;
		ShaderType m_type{};
		unsigned int m_shader{};
	};
}
