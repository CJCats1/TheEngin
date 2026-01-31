#pragma once
#include <TheEngine/Core/Logger.h>
#include <TheEngine/Core/Base.h>
#if defined(_WIN32)
#include <d3d11.h>
#endif

namespace TheEngine
{
	namespace GraphicsLogUtils
	{
#if defined(_WIN32)
		inline void CheckShaderCompile(Logger& logger, HRESULT hr, ID3DBlob* errorBlob)
		{
			auto errorMsg = errorBlob ? static_cast<const char*>(errorBlob->GetBufferPointer()) : nullptr;

			if (FAILED(hr))
				THEENGINE_LOG_THROW(logger, std::runtime_error, Logger::LogLevel::Error, errorMsg ? errorMsg : "Shader compilation failed.");

			if (errorMsg)
				THEENGINE_LOG(logger, Logger::LogLevel::Warning, errorMsg);
		}
#endif
	}
}

#define THEENGINE_GRAPHICS_LOG_THROW_ON_FAIL(hr, message)\
	{\
	auto res = (hr);\
	if(FAILED(res))\
		THEENGINE_LOG_THROW_ERROR(message);\
	}

#if defined(_WIN32)
#define THEENGINE_GRAPHICS_CHECK_SHADER_COMPILE(logger,hr,errorBlob)\
{\
auto res = (hr);\
TheEngine::GraphicsLogUtils::CheckShaderCompile(logger, res,errorBlob);\
}
#else
#define THEENGINE_GRAPHICS_CHECK_SHADER_COMPILE(logger,hr,errorBlob) (void)(logger); (void)(hr); (void)(errorBlob)
#endif
#if defined(_WIN32)
#define DXCall(x) do { HRESULT _hr = (x); if (FAILED(_hr)) THEENGINE_LOG_THROW_ERROR("DirectX call failed: " #x); } while(0)
#else
#define DXCall(x) do { (void)(x); } while(0)
#endif
