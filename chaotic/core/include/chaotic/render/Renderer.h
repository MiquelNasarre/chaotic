#pragma once

#include "Image/Color.h"
#include "Math/Vectors.h"

#include <cstddef>
#include <memory>

namespace chaotic::render
{
	enum class BackendType
	{
		Unknown,
		D3D11,
		Metal,
	};

	enum class BufferUsage
	{
		Immutable,
		Dynamic,
	};

	struct BufferDesc
	{
		const void* data = nullptr;
		unsigned long long size_bytes = 0;
		unsigned stride_bytes = 0;
		BufferUsage usage = BufferUsage::Immutable;
		const char* debug_name = nullptr;
	};

	enum class ShaderLanguage
	{
		MetalShadingLanguage,
	};

	struct ShaderLibraryDesc
	{
		ShaderLanguage language = ShaderLanguage::MetalShadingLanguage;
		const char* source = nullptr;
		unsigned long long source_size = 0;
		const char* debug_name = nullptr;
	};

	enum class VertexFormat
	{
		Float,
		Float2,
		Float3,
		Float4,
		UChar4Normalized,
	};

	struct VertexAttributeDesc
	{
		unsigned location = 0;
		unsigned buffer_slot = 0;
		unsigned offset_bytes = 0;
		VertexFormat format = VertexFormat::Float3;
	};

	struct VertexLayoutDesc
	{
		const VertexAttributeDesc* attributes = nullptr;
		unsigned attribute_count = 0;
		unsigned stride_bytes = 0;
	};

	enum class PrimitiveTopology
	{
		PointList,
		LineList,
		LineStrip,
		TriangleList,
	};

	enum class BlendMode
	{
		Opaque,
		Alpha,
		Additive,
	};

	enum class DepthMode
	{
		Disabled,
		ReadWrite,
		ReadOnly,
	};

	struct PipelineDesc
	{
		const class ShaderLibrary* shader_library = nullptr;
		const char* vertex_entry = "vertex_main";
		const char* fragment_entry = "fragment_main";
		VertexLayoutDesc vertex_layout = {};
		PrimitiveTopology topology = PrimitiveTopology::TriangleList;
		BlendMode blend_mode = BlendMode::Opaque;
		DepthMode depth_mode = DepthMode::ReadWrite;
		const char* debug_name = nullptr;
	};

	struct FrameDesc
	{
		Color clear_color = Color::Black;
		bool clear_color_buffer = true;
		bool clear_depth_buffer = true;
		float clear_depth = 1.f;
	};

	class Buffer
	{
	public:
		virtual ~Buffer() = default;
		virtual BackendType backendType() const noexcept = 0;
		virtual unsigned long long sizeBytes() const noexcept = 0;
		virtual unsigned strideBytes() const noexcept = 0;
		virtual void update(const void* data, unsigned long long size_bytes, unsigned long long offset_bytes = 0) = 0;
	};

	class ShaderLibrary
	{
	public:
		virtual ~ShaderLibrary() = default;
		virtual BackendType backendType() const noexcept = 0;
	};

	class Pipeline
	{
	public:
		virtual ~Pipeline() = default;
		virtual BackendType backendType() const noexcept = 0;
		virtual PrimitiveTopology topology() const noexcept = 0;
	};

	class CommandEncoder
	{
	public:
		virtual ~CommandEncoder() = default;
		virtual BackendType backendType() const noexcept = 0;
		virtual void setPipeline(const Pipeline& pipeline) = 0;
		virtual void setVertexBuffer(const Buffer& buffer, unsigned slot = 0, unsigned long long offset_bytes = 0) = 0;
		virtual void setFragmentBuffer(const Buffer& buffer, unsigned slot = 0, unsigned long long offset_bytes = 0) = 0;
		virtual void draw(unsigned vertex_count, unsigned start_vertex = 0) = 0;
	};

	class Device
	{
	public:
		virtual ~Device() = default;
		virtual BackendType backendType() const noexcept = 0;
		virtual std::unique_ptr<Buffer> createBuffer(const BufferDesc& desc) = 0;
		virtual std::unique_ptr<ShaderLibrary> createShaderLibrary(const ShaderLibraryDesc& desc) = 0;
		virtual std::unique_ptr<Pipeline> createPipeline(const PipelineDesc& desc) = 0;
	};

	class RenderTarget
	{
	public:
		virtual ~RenderTarget() = default;
		virtual BackendType backendType() const noexcept = 0;
		virtual Device& device() = 0;
		virtual const Device& device() const = 0;
		virtual Vector2i dimensions() const = 0;
		virtual bool isOpen() const noexcept = 0;
		virtual CommandEncoder* beginFrame(const FrameDesc& desc = {}) = 0;
		virtual void endFrame() = 0;
	};
}
