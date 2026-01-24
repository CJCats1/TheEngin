#pragma once
#include <DX3D/Core/Logger.h>
#include <DX3D/Core/Base.h>
#if defined(_WIN32)
#include <d3d11.h>
#endif

namespace dx3d
{
	namespace GraphicsLogUtils
	{
#if defined(_WIN32)
		inline void CheckShaderCompile(Logger& logger, HRESULT hr, ID3DBlob* errorBlob)
		{
			auto errorMsg = errorBlob ? static_cast<const char*>(errorBlob->GetBufferPointer()) : nullptr;

			if (FAILED(hr))
				DX3DLogThrow(logger, std::runtime_error, Logger::LogLevel::Error, errorMsg ? errorMsg : "Shader compilation failed.");

			if (errorMsg)
				DX3DLog(logger, Logger::LogLevel::Warning, errorMsg);
		}
#endif
	}
}

#define DX3DGraphicsLogThrowOnFail(hr, message)\
	{\
	auto res = (hr);\
	if(FAILED(res))\
		DX3DLogThrowError(message);\
	}

#if defined(_WIN32)
#define DX3DGraphicsCheckShaderCompile(logger,hr,errorBlob)\
{\
auto res = (hr);\
dx3d::GraphicsLogUtils::CheckShaderCompile(logger, res,errorBlob);\
}
#else
#define DX3DGraphicsCheckShaderCompile(logger,hr,errorBlob) (void)(logger); (void)(hr); (void)(errorBlob)
#endif
#if defined(_WIN32)
#define DXCall(x) do { HRESULT _hr = (x); if (FAILED(_hr)) DX3DLogThrowError("DirectX call failed: " #x); } while(0)
#else
#define DXCall(x) do { (void)(x); } while(0)
#endif
