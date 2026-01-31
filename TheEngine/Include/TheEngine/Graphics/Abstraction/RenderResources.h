#pragma once
#include <TheEngine/Core/Common.h>

namespace TheEngine
{
	class IRenderResource
	{
	public:
		virtual ~IRenderResource() = default;
	};

	class IRenderPipelineState : public IRenderResource
	{
	public:
		~IRenderPipelineState() override = default;
	};

	class IRenderShaderBinary : public IRenderResource
	{
	public:
		~IRenderShaderBinary() override = default;
		virtual BinaryData getData() const noexcept = 0;
		virtual ShaderType getType() const noexcept = 0;
	};

	class IRenderVertexShaderSignature : public IRenderResource
	{
	public:
		~IRenderVertexShaderSignature() override = default;
		virtual BinaryData getShaderBinaryData() const noexcept = 0;
		virtual BinaryData getInputElementsData() const noexcept = 0;
	};

	class IRenderVertexBuffer : public IRenderResource
	{
	public:
		~IRenderVertexBuffer() override = default;
		virtual ui32 getVertexListSize() const noexcept = 0;
		virtual ui32 getVertexSize() const noexcept = 0;
	};

	class IRenderIndexBuffer : public IRenderResource
	{
	public:
		~IRenderIndexBuffer() override = default;
		virtual ui32 getCount() const noexcept = 0;
		virtual ui32 getStride() const noexcept = 0;
	};
}
