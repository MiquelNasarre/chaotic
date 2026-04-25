#include "chaotic_metal.h"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>

#include <cstring>
#include <memory>
#include <utility>

@interface ChaoticMetalWindowDelegate : NSObject <NSWindowDelegate>
@property(nonatomic, assign) bool* closed;
@end

@implementation ChaoticMetalWindowDelegate

- (BOOL)windowShouldClose:(id)sender
{
	if (self.closed)
		*self.closed = true;
	return YES;
}

- (void)windowWillClose:(NSNotification*)notification
{
	if (self.closed)
		*self.closed = true;
}

@end

namespace chaotic::metal
{
	namespace
	{
		void ensureApplication()
		{
			static bool initialized = false;
			if (initialized)
				return;

			[NSApplication sharedApplication];
			[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
			[NSApp finishLaunching];

			initialized = true;
		}

		NSString* makeString(const char* text)
		{
			return [NSString stringWithUTF8String:text ? text : ""];
		}

		MTLClearColor makeClearColor(Color color)
		{
			return MTLClearColorMake(
				static_cast<double>(color.R) / 255.0,
				static_cast<double>(color.G) / 255.0,
				static_cast<double>(color.B) / 255.0,
				static_cast<double>(color.A) / 255.0
			);
		}

		MTLPrimitiveType makePrimitiveType(render::PrimitiveTopology topology)
		{
			switch (topology)
			{
			case render::PrimitiveTopology::PointList: return MTLPrimitiveTypePoint;
			case render::PrimitiveTopology::LineList: return MTLPrimitiveTypeLine;
			case render::PrimitiveTopology::LineStrip: return MTLPrimitiveTypeLineStrip;
			case render::PrimitiveTopology::TriangleList: return MTLPrimitiveTypeTriangle;
			default: return MTLPrimitiveTypeTriangle;
			}
		}

		MTLVertexFormat makeVertexFormat(render::VertexFormat format)
		{
			switch (format)
			{
			case render::VertexFormat::Float: return MTLVertexFormatFloat;
			case render::VertexFormat::Float2: return MTLVertexFormatFloat2;
			case render::VertexFormat::Float3: return MTLVertexFormatFloat3;
			case render::VertexFormat::Float4: return MTLVertexFormatFloat4;
			case render::VertexFormat::UChar4Normalized: return MTLVertexFormatUChar4Normalized;
			default: return MTLVertexFormatFloat3;
			}
		}

		void configureBlend(MTLRenderPipelineColorAttachmentDescriptor* attachment, render::BlendMode blend_mode)
		{
			attachment.blendingEnabled = NO;

			switch (blend_mode)
			{
			case render::BlendMode::Opaque:
				return;

			case render::BlendMode::Alpha:
				attachment.blendingEnabled = YES;
				attachment.rgbBlendOperation = MTLBlendOperationAdd;
				attachment.alphaBlendOperation = MTLBlendOperationAdd;
				attachment.sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
				attachment.destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
				attachment.sourceAlphaBlendFactor = MTLBlendFactorOne;
				attachment.destinationAlphaBlendFactor = MTLBlendFactorZero;
				return;

			case render::BlendMode::Additive:
				attachment.blendingEnabled = YES;
				attachment.rgbBlendOperation = MTLBlendOperationAdd;
				attachment.alphaBlendOperation = MTLBlendOperationAdd;
				attachment.sourceRGBBlendFactor = MTLBlendFactorOne;
				attachment.destinationRGBBlendFactor = MTLBlendFactorOne;
				attachment.sourceAlphaBlendFactor = MTLBlendFactorZero;
				attachment.destinationAlphaBlendFactor = MTLBlendFactorOne;
				return;
			}
		}

		void failWithNSError(NSError* error, const char* fallback)
		{
			if (error && [error localizedDescription])
				USER_ERROR([[error localizedDescription] UTF8String]);

			USER_ERROR(fallback);
		}

		const char kCurveShaderSource[] = R"msl(
#include <metal_stdlib>
using namespace metal;

struct CurvePositionVertex
{
	float4 position [[attribute(0)]];
};

struct CurveColorVertex
{
	float4 position [[attribute(0)]];
	float4 color [[attribute(1)]];
};

struct CurveVertexOut
{
	float4 position [[position]];
	float4 color;
};

struct PerspectiveData
{
	float4 observer;
	float4 center;
	float4 scaling;
};

struct ObjectData
{
	float4x4 transform;
	float4 displacement;
};

inline float4 qMul(float4 q0, float4 q1)
{
	return float4(
		q0.x * q1.x - q0.y * q1.y - q0.z * q1.z - q0.w * q1.w,
		q0.x * q1.y + q0.y * q1.x + q0.z * q1.w - q0.w * q1.z,
		q0.x * q1.z + q0.z * q1.x + q0.w * q1.y - q0.y * q1.w,
		q0.x * q1.w + q0.w * q1.x + q0.y * q1.z - q0.z * q1.y
	);
}

inline float4 qConj(float4 q)
{
	return float4(q.x, -q.yzw);
}

inline float4 qRot(float4 q, float4 pos)
{
	return qMul(qMul(q, pos), qConj(q));
}

inline float zTrans(float z)
{
	return 1.0 / (1.0 + exp(-z));
}

inline float4 r3ToScreenPos(float4 r3pos, constant PerspectiveData& perspective)
{
	float4 pos = r3pos - perspective.center;
	float4 rotq = qRot(perspective.observer, float4(0.0, pos.xyz));
	float3 rotpos = rotq.yzw;
	return float4(rotpos.xy * perspective.scaling.xy, zTrans(rotpos.z), 1.0);
}

vertex CurveVertexOut curve_global_vertex(
	CurvePositionVertex in [[stage_in]],
	constant PerspectiveData& perspective [[buffer(1)]],
	constant ObjectData& object [[buffer(2)]])
{
	CurveVertexOut out;
	float4 r3pos = object.transform * in.position;
	out.position = r3ToScreenPos(r3pos, perspective);
	out.position.xy += object.displacement.xy;
	out.color = float4(1.0);
	return out;
}

fragment float4 curve_global_fragment(CurveVertexOut in [[stage_in]], constant float4& color [[buffer(0)]])
{
	return color;
}

vertex CurveVertexOut curve_color_vertex(
	CurveColorVertex in [[stage_in]],
	constant PerspectiveData& perspective [[buffer(1)]],
	constant ObjectData& object [[buffer(2)]])
{
	CurveVertexOut out;
	float4 r3pos = object.transform * in.position;
	out.position = r3ToScreenPos(r3pos, perspective);
	out.position.xy += object.displacement.xy;
	out.color = in.color;
	return out;
}

fragment float4 curve_color_fragment(CurveVertexOut in [[stage_in]])
{
	return in.color;
}
)msl";

		const char* curveVertexEntry(render::CurveColoring coloring) noexcept
		{
			return coloring == render::CurveColoring::Global ? "curve_global_vertex" : "curve_color_vertex";
		}

		const char* curveFragmentEntry(render::CurveColoring coloring) noexcept
		{
			return coloring == render::CurveColoring::Global ? "curve_global_fragment" : "curve_color_fragment";
		}

		const char* scatterVertexEntry(render::ScatterColoring coloring) noexcept
		{
			return coloring == render::ScatterColoring::Global ? "curve_global_vertex" : "curve_color_vertex";
		}

		const char* scatterFragmentEntry(render::ScatterColoring coloring) noexcept
		{
			return coloring == render::ScatterColoring::Global ? "curve_global_fragment" : "curve_color_fragment";
		}

		render::BlendMode curveBlendMode(const render::CurveDesc& desc) noexcept
		{
			return desc.enable_transparency ? render::BlendMode::Alpha : render::BlendMode::Opaque;
		}

		render::BlendMode scatterBlendMode(render::ScatterBlending blending) noexcept
		{
			switch (blending)
			{
			case render::ScatterBlending::Transparent: return render::BlendMode::Alpha;
			case render::ScatterBlending::Glow: return render::BlendMode::Additive;
			case render::ScatterBlending::Opaque:
			default: return render::BlendMode::Opaque;
			}
		}

		render::DepthMode curveDepthMode(const render::CurveDesc& desc) noexcept
		{
			return desc.enable_transparency ? render::DepthMode::ReadOnly : render::DepthMode::ReadWrite;
		}

		render::DepthMode scatterDepthMode(render::ScatterBlending blending) noexcept
		{
			switch (blending)
			{
			case render::ScatterBlending::Glow:
			case render::ScatterBlending::Transparent:
				return render::DepthMode::ReadOnly;
			case render::ScatterBlending::Opaque:
			default:
				return render::DepthMode::ReadWrite;
			}
		}

		struct alignas(16) CurvePerspectiveData
		{
			Quaternion observer = 1.f;
			_float4vector center = {};
			_float4vector scaling = {};
		};

		struct alignas(16) CurveObjectData
		{
			_float4matrix transform = Matrix(1.f).getMatrix4();
			_float4vector displacement = {};
		};

		CurvePerspectiveData makeCurvePerspectiveData(const render::Camera& camera, Vector2i dimensions)
		{
			USER_CHECK(camera.observer, "The Metal Curve camera observer must be a non-zero quaternion.");

			const float width = dimensions.x > 0 ? static_cast<float>(dimensions.x) : 1.f;
			const float height = dimensions.y > 0 ? static_cast<float>(dimensions.y) : 1.f;

			return {
				camera.observer.normal(),
				camera.center.getVector4(),
				{ camera.scale / width, camera.scale / height, camera.scale, 0.f }
			};
		}
	}

	class MetalBuffer final : public render::Buffer
	{
	public:
		MetalBuffer(id<MTLDevice> device, const render::BufferDesc& desc)
			: size_bytes_(desc.size_bytes), stride_bytes_(desc.stride_bytes), usage_(desc.usage)
		{
			USER_CHECK(device != nil, "Cannot create a Metal buffer without a Metal device.");
			USER_CHECK(desc.size_bytes > 0, "Cannot create a zero-sized Metal buffer.");

			if (desc.data)
				buffer_ = [device newBufferWithBytes:desc.data length:(NSUInteger)desc.size_bytes options:MTLResourceStorageModeShared];
			else
				buffer_ = [device newBufferWithLength:(NSUInteger)desc.size_bytes options:MTLResourceStorageModeShared];

			USER_CHECK(buffer_ != nil, "Could not create a Metal buffer.");

			if (desc.debug_name)
				[buffer_ setLabel:makeString(desc.debug_name)];
		}

		render::BackendType backendType() const noexcept override { return render::BackendType::Metal; }
		unsigned long long sizeBytes() const noexcept override { return size_bytes_; }
		unsigned strideBytes() const noexcept override { return stride_bytes_; }

		void update(const void* data, unsigned long long size_bytes, unsigned long long offset_bytes) override
		{
			USER_CHECK(usage_ == render::BufferUsage::Dynamic, "Cannot update an immutable Metal buffer.");
			USER_CHECK(data != nullptr, "Cannot update a Metal buffer from a null pointer.");
			USER_CHECK(size_bytes > 0, "Cannot update a Metal buffer with zero bytes.");
			USER_CHECK(offset_bytes + size_bytes <= size_bytes_, "Metal buffer update exceeds the buffer size.");

			void* contents = [buffer_ contents];
			USER_CHECK(contents != nullptr, "Could not map Metal buffer contents.");

			std::memcpy(static_cast<unsigned char*>(contents) + offset_bytes, data, size_bytes);
			[buffer_ didModifyRange:NSMakeRange((NSUInteger)offset_bytes, (NSUInteger)size_bytes)];
		}

		id<MTLBuffer> native() const noexcept { return buffer_; }

	private:
		id<MTLBuffer> buffer_ = nil;
		unsigned long long size_bytes_ = 0;
		unsigned stride_bytes_ = 0;
		render::BufferUsage usage_ = render::BufferUsage::Immutable;
	};

	class MetalShaderLibrary final : public render::ShaderLibrary
	{
	public:
		MetalShaderLibrary(id<MTLDevice> device, const render::ShaderLibraryDesc& desc)
		{
			USER_CHECK(device != nil, "Cannot create a Metal shader library without a Metal device.");
			USER_CHECK(desc.language == render::ShaderLanguage::MetalShadingLanguage, "Metal backend only accepts MSL shader sources.");
			USER_CHECK(desc.source != nullptr, "Cannot create a Metal shader library with a null source pointer.");

			NSString* source = nil;
			if (desc.source_size)
			{
				source = [[NSString alloc]
					initWithBytes:desc.source
					length:(NSUInteger)desc.source_size
					encoding:NSUTF8StringEncoding
				];
			}
			else
			{
				source = makeString(desc.source);
			}

			USER_CHECK(source != nil, "Could not decode Metal shader source as UTF-8.");

			NSError* error = nil;
			library_ = [device newLibraryWithSource:source options:nil error:&error];
			if (!library_)
				failWithNSError(error, "Could not compile Metal shader library.");

			if (desc.debug_name)
				[library_ setLabel:makeString(desc.debug_name)];
		}

		render::BackendType backendType() const noexcept override { return render::BackendType::Metal; }

		id<MTLLibrary> native() const noexcept { return library_; }

	private:
		id<MTLLibrary> library_ = nil;
	};

	class MetalPipeline final : public render::Pipeline
	{
	public:
		MetalPipeline(id<MTLDevice> device, const render::PipelineDesc& desc)
			: topology_(desc.topology)
		{
			USER_CHECK(device != nil, "Cannot create a Metal pipeline without a Metal device.");
			USER_CHECK(desc.shader_library != nullptr, "Cannot create a Metal pipeline without a shader library.");
			USER_CHECK(desc.shader_library->backendType() == render::BackendType::Metal, "Metal pipeline received a shader library from another backend.");

			const auto& library = static_cast<const MetalShaderLibrary&>(*desc.shader_library);
			id<MTLFunction> vertex = [library.native() newFunctionWithName:makeString(desc.vertex_entry)];
			id<MTLFunction> fragment = [library.native() newFunctionWithName:makeString(desc.fragment_entry)];

			USER_CHECK(vertex != nil, "Could not find the requested Metal vertex function.");
			USER_CHECK(fragment != nil, "Could not find the requested Metal fragment function.");

			MTLRenderPipelineDescriptor* pipeline_desc = [MTLRenderPipelineDescriptor new];
			pipeline_desc.vertexFunction = vertex;
			pipeline_desc.fragmentFunction = fragment;
			pipeline_desc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;
			pipeline_desc.depthAttachmentPixelFormat = MTLPixelFormatDepth32Float;
			configureBlend(pipeline_desc.colorAttachments[0], desc.blend_mode);

			if (desc.debug_name)
				[pipeline_desc setLabel:makeString(desc.debug_name)];

			if (desc.vertex_layout.attribute_count)
			{
				USER_CHECK(desc.vertex_layout.attributes != nullptr, "Metal pipeline received a null vertex attribute list.");
				USER_CHECK(desc.vertex_layout.stride_bytes > 0, "Metal pipeline received a zero vertex stride.");

				MTLVertexDescriptor* vertex_desc = [MTLVertexDescriptor vertexDescriptor];
				bool used_slots[31] = {};

				for (unsigned i = 0; i < desc.vertex_layout.attribute_count; ++i)
				{
					const auto& attr = desc.vertex_layout.attributes[i];
					USER_CHECK(attr.location < 31, "Metal vertex attribute index is out of range.");
					USER_CHECK(attr.buffer_slot < 31, "Metal vertex buffer slot is out of range.");

					vertex_desc.attributes[attr.location].format = makeVertexFormat(attr.format);
					vertex_desc.attributes[attr.location].offset = attr.offset_bytes;
					vertex_desc.attributes[attr.location].bufferIndex = attr.buffer_slot;
					used_slots[attr.buffer_slot] = true;
				}

				for (unsigned slot = 0; slot < 31; ++slot)
				{
					if (!used_slots[slot])
						continue;

					vertex_desc.layouts[slot].stride = desc.vertex_layout.stride_bytes;
					vertex_desc.layouts[slot].stepFunction = MTLVertexStepFunctionPerVertex;
					vertex_desc.layouts[slot].stepRate = 1;
				}

				pipeline_desc.vertexDescriptor = vertex_desc;
			}

			NSError* error = nil;
			pipeline_ = [device newRenderPipelineStateWithDescriptor:pipeline_desc error:&error];
			if (!pipeline_)
				failWithNSError(error, "Could not create a Metal render pipeline.");

			MTLDepthStencilDescriptor* depth_desc = [MTLDepthStencilDescriptor new];
			switch (desc.depth_mode)
			{
			case render::DepthMode::Disabled:
				depth_desc.depthCompareFunction = MTLCompareFunctionAlways;
				depth_desc.depthWriteEnabled = NO;
				break;

			case render::DepthMode::ReadOnly:
				depth_desc.depthCompareFunction = MTLCompareFunctionLessEqual;
				depth_desc.depthWriteEnabled = NO;
				break;

			case render::DepthMode::ReadWrite:
			default:
				depth_desc.depthCompareFunction = MTLCompareFunctionLessEqual;
				depth_desc.depthWriteEnabled = YES;
				break;
			}

			depth_state_ = [device newDepthStencilStateWithDescriptor:depth_desc];
			USER_CHECK(depth_state_ != nil, "Could not create a Metal depth stencil state.");
		}

		render::BackendType backendType() const noexcept override { return render::BackendType::Metal; }
		render::PrimitiveTopology topology() const noexcept override { return topology_; }

		id<MTLRenderPipelineState> native() const noexcept { return pipeline_; }
		id<MTLDepthStencilState> nativeDepthState() const noexcept { return depth_state_; }

	private:
		id<MTLRenderPipelineState> pipeline_ = nil;
		id<MTLDepthStencilState> depth_state_ = nil;
		render::PrimitiveTopology topology_ = render::PrimitiveTopology::TriangleList;
	};

	class MetalCommandEncoder final : public render::CommandEncoder
	{
	public:
		render::BackendType backendType() const noexcept override { return render::BackendType::Metal; }

		void reset(id<MTLRenderCommandEncoder> encoder)
		{
			encoder_ = encoder;
			topology_ = render::PrimitiveTopology::TriangleList;
		}

		void finish()
		{
			if (encoder_)
				[encoder_ endEncoding];
			encoder_ = nil;
		}

		void setPipeline(const render::Pipeline& pipeline) override
		{
			USER_CHECK(encoder_ != nil, "Cannot bind a Metal pipeline without an active frame encoder.");
			USER_CHECK(pipeline.backendType() == render::BackendType::Metal, "Metal command encoder received a pipeline from another backend.");

			const auto& metal_pipeline = static_cast<const MetalPipeline&>(pipeline);
			[encoder_ setRenderPipelineState:metal_pipeline.native()];
			[encoder_ setDepthStencilState:metal_pipeline.nativeDepthState()];
			topology_ = pipeline.topology();
		}

		void setVertexBuffer(const render::Buffer& buffer, unsigned slot, unsigned long long offset_bytes) override
		{
			USER_CHECK(encoder_ != nil, "Cannot bind a Metal vertex buffer without an active frame encoder.");
			USER_CHECK(buffer.backendType() == render::BackendType::Metal, "Metal command encoder received a buffer from another backend.");
			USER_CHECK(offset_bytes <= buffer.sizeBytes(), "Metal vertex buffer offset exceeds the buffer size.");

			const auto& metal_buffer = static_cast<const MetalBuffer&>(buffer);
			[encoder_ setVertexBuffer:metal_buffer.native() offset:(NSUInteger)offset_bytes atIndex:slot];
		}

		void setFragmentBuffer(const render::Buffer& buffer, unsigned slot, unsigned long long offset_bytes) override
		{
			USER_CHECK(encoder_ != nil, "Cannot bind a Metal fragment buffer without an active frame encoder.");
			USER_CHECK(buffer.backendType() == render::BackendType::Metal, "Metal command encoder received a buffer from another backend.");
			USER_CHECK(offset_bytes <= buffer.sizeBytes(), "Metal fragment buffer offset exceeds the buffer size.");

			const auto& metal_buffer = static_cast<const MetalBuffer&>(buffer);
			[encoder_ setFragmentBuffer:metal_buffer.native() offset:(NSUInteger)offset_bytes atIndex:slot];
		}

		void draw(unsigned vertex_count, unsigned start_vertex) override
		{
			USER_CHECK(encoder_ != nil, "Cannot issue a Metal draw call without an active frame encoder.");
			USER_CHECK(vertex_count > 0, "Cannot issue a draw call with zero vertices.");

			[encoder_
				drawPrimitives:makePrimitiveType(topology_)
				vertexStart:(NSUInteger)start_vertex
				vertexCount:(NSUInteger)vertex_count
			];
		}

	private:
		id<MTLRenderCommandEncoder> encoder_ = nil;
		render::PrimitiveTopology topology_ = render::PrimitiveTopology::TriangleList;
	};

	class MetalDevice final : public render::Device
	{
	public:
		explicit MetalDevice(id<MTLDevice> device)
			: device_(device)
		{
			USER_CHECK(device_ != nil, "Cannot wrap a null Metal device.");
		}

		render::BackendType backendType() const noexcept override { return render::BackendType::Metal; }

		std::unique_ptr<render::Buffer> createBuffer(const render::BufferDesc& desc) override
		{
			return std::make_unique<MetalBuffer>(device_, desc);
		}

		std::unique_ptr<render::ShaderLibrary> createShaderLibrary(const render::ShaderLibraryDesc& desc) override
		{
			return std::make_unique<MetalShaderLibrary>(device_, desc);
		}

		std::unique_ptr<render::Pipeline> createPipeline(const render::PipelineDesc& desc) override
		{
			return std::make_unique<MetalPipeline>(device_, desc);
		}

	private:
		id<MTLDevice> device_ = nil;
	};

	struct Curve::Impl
	{
		render::Device* device = nullptr;
		render::CurveGeometry geometry;
		std::unique_ptr<render::Buffer> vertex_buffer;
		std::unique_ptr<render::Buffer> perspective_buffer;
		std::unique_ptr<render::Buffer> object_buffer;
		std::unique_ptr<render::Buffer> global_color_buffer;
		std::unique_ptr<render::ShaderLibrary> shader_library;
		std::unique_ptr<render::Pipeline> pipeline;
		CurveObjectData object_data = {};
		Matrix distortion = Matrix(1.f);
		Quaternion rotation = 1.f;
		Vector3f position = {};

		Impl(render::Device& render_device, const render::CurveDesc& desc)
			: device(&render_device), geometry(desc)
		{
			USER_CHECK(device->backendType() == render::BackendType::Metal, "Metal Curve received a device from another backend.");
			createResources();
		}

		void createResources()
		{
			vertex_buffer = device->createBuffer(geometry.vertexBufferDesc());

			CurvePerspectiveData perspective_data = {};
			perspective_buffer = device->createBuffer({
				&perspective_data,
				sizeof(perspective_data),
				sizeof(perspective_data),
				render::BufferUsage::Dynamic,
				"chaotic_curve_perspective"
			});

			refreshObjectData();
			object_buffer = device->createBuffer({
				&object_data,
				sizeof(object_data),
				sizeof(object_data),
				render::BufferUsage::Dynamic,
				"chaotic_curve_object"
			});

			render::ShaderLibraryDesc shader_desc = {};
			shader_desc.language = render::ShaderLanguage::MetalShadingLanguage;
			shader_desc.source = kCurveShaderSource;
			shader_desc.source_size = sizeof(kCurveShaderSource) - 1u;
			shader_desc.debug_name = "chaotic_curve_shader_library";
			shader_library = device->createShaderLibrary(shader_desc);

			render::PipelineDesc pipeline_desc = {};
			pipeline_desc.shader_library = shader_library.get();
			pipeline_desc.vertex_entry = curveVertexEntry(geometry.coloring());
			pipeline_desc.fragment_entry = curveFragmentEntry(geometry.coloring());
			pipeline_desc.vertex_layout = geometry.vertexLayout();
			pipeline_desc.topology = geometry.topology();
			pipeline_desc.blend_mode = curveBlendMode(geometry.desc());
			pipeline_desc.depth_mode = curveDepthMode(geometry.desc());
			pipeline_desc.debug_name = "chaotic_curve_pipeline";
			pipeline = device->createPipeline(pipeline_desc);

			if (geometry.coloring() == render::CurveColoring::Global)
			{
				_float4color color = geometry.globalColor();
				global_color_buffer = device->createBuffer({
					&color,
					sizeof(color),
					sizeof(color),
					render::BufferUsage::Dynamic,
					"chaotic_curve_global_color"
				});
			}
		}

		void updateVertexBuffer()
		{
			vertex_buffer->update(geometry.vertexData(), geometry.vertexByteSize());
		}

		void updatePerspectiveBuffer(const render::DrawContext& context)
		{
			CurvePerspectiveData perspective_data = makeCurvePerspectiveData(context.camera, context.dimensions);
			perspective_buffer->update(&perspective_data, sizeof(perspective_data));
		}

		void refreshObjectData()
		{
			Matrix linear = rotation.getMatrix() * distortion;
			object_data.transform = linear.getMatrix4(position);
		}

		void updateObjectBuffer()
		{
			refreshObjectData();
			object_buffer->update(&object_data, sizeof(object_data));
		}
	};

	struct Scatter::Impl
	{
		render::Device* device = nullptr;
		render::ScatterGeometry geometry;
		std::unique_ptr<render::Buffer> vertex_buffer;
		std::unique_ptr<render::Buffer> perspective_buffer;
		std::unique_ptr<render::Buffer> object_buffer;
		std::unique_ptr<render::Buffer> global_color_buffer;
		std::unique_ptr<render::ShaderLibrary> shader_library;
		std::unique_ptr<render::Pipeline> pipeline;
		CurveObjectData object_data = {};
		Matrix distortion = Matrix(1.f);
		Quaternion rotation = 1.f;
		Vector3f position = {};

		Impl(render::Device& render_device, const render::ScatterDesc& desc)
			: device(&render_device), geometry(desc)
		{
			USER_CHECK(device->backendType() == render::BackendType::Metal, "Metal Scatter received a device from another backend.");
			createResources();
		}

		void createResources()
		{
			vertex_buffer = device->createBuffer(geometry.vertexBufferDesc());

			CurvePerspectiveData perspective_data = {};
			perspective_buffer = device->createBuffer({
				&perspective_data,
				sizeof(perspective_data),
				sizeof(perspective_data),
				render::BufferUsage::Dynamic,
				"chaotic_scatter_perspective"
			});

			refreshObjectData();
			object_buffer = device->createBuffer({
				&object_data,
				sizeof(object_data),
				sizeof(object_data),
				render::BufferUsage::Dynamic,
				"chaotic_scatter_object"
			});

			render::ShaderLibraryDesc shader_desc = {};
			shader_desc.language = render::ShaderLanguage::MetalShadingLanguage;
			shader_desc.source = kCurveShaderSource;
			shader_desc.source_size = sizeof(kCurveShaderSource) - 1u;
			shader_desc.debug_name = "chaotic_scatter_shader_library";
			shader_library = device->createShaderLibrary(shader_desc);

			render::PipelineDesc pipeline_desc = {};
			pipeline_desc.shader_library = shader_library.get();
			pipeline_desc.vertex_entry = scatterVertexEntry(geometry.coloring());
			pipeline_desc.fragment_entry = scatterFragmentEntry(geometry.coloring());
			pipeline_desc.vertex_layout = geometry.vertexLayout();
			pipeline_desc.topology = geometry.topology();
			pipeline_desc.blend_mode = scatterBlendMode(geometry.blending());
			pipeline_desc.depth_mode = scatterDepthMode(geometry.blending());
			pipeline_desc.debug_name = "chaotic_scatter_pipeline";
			pipeline = device->createPipeline(pipeline_desc);

			if (geometry.coloring() == render::ScatterColoring::Global)
			{
				_float4color color = geometry.globalColor();
				global_color_buffer = device->createBuffer({
					&color,
					sizeof(color),
					sizeof(color),
					render::BufferUsage::Dynamic,
					"chaotic_scatter_global_color"
				});
			}
		}

		void updateVertexBuffer()
		{
			vertex_buffer->update(geometry.vertexData(), geometry.vertexByteSize());
		}

		void updatePerspectiveBuffer(const render::DrawContext& context)
		{
			CurvePerspectiveData perspective_data = makeCurvePerspectiveData(context.camera, context.dimensions);
			perspective_buffer->update(&perspective_data, sizeof(perspective_data));
		}

		void refreshObjectData()
		{
			Matrix linear = rotation.getMatrix() * distortion;
			object_data.transform = linear.getMatrix4(position);
		}

		void updateObjectBuffer()
		{
			refreshObjectData();
			object_buffer->update(&object_data, sizeof(object_data));
		}
	};

	struct Window::Impl
	{
		id<MTLDevice> device = nil;
		id<MTLCommandQueue> command_queue = nil;
		MetalDevice* render_device = nullptr;
		NSWindow* window = nil;
		CAMetalLayer* metal_layer = nil;
		ChaoticMetalWindowDelegate* delegate = nil;
		id<MTLTexture> depth_texture = nil;
		CGSize depth_texture_size = CGSizeMake(0, 0);
		id<CAMetalDrawable> current_drawable = nil;
		id<MTLCommandBuffer> current_command_buffer = nil;
		MetalCommandEncoder frame_encoder;
		bool closed = false;

		explicit Impl(const WindowDesc& desc)
		{
			ensureApplication();

			device = MTLCreateSystemDefaultDevice();
			USER_CHECK(device != nil, "Metal is not available on this system.");

			command_queue = [device newCommandQueue];
			USER_CHECK(command_queue != nil, "Could not create a Metal command queue.");
			render_device = new MetalDevice(device);

			const int width = desc.dimensions.x > 0 ? desc.dimensions.x : 720;
			const int height = desc.dimensions.y > 0 ? desc.dimensions.y : 480;

			NSRect content_rect = NSMakeRect(0, 0, width, height);
			NSWindowStyleMask style =
				NSWindowStyleMaskTitled |
				NSWindowStyleMaskClosable |
				NSWindowStyleMaskMiniaturizable |
				NSWindowStyleMaskResizable;

			window = [[NSWindow alloc]
				initWithContentRect:content_rect
				styleMask:style
				backing:NSBackingStoreBuffered
				defer:NO
			];
			USER_CHECK(window != nil, "Could not create a Cocoa window for the Metal backend.");

			[window setReleasedWhenClosed:NO];
			[window setTitle:makeString(desc.title)];
			[window center];

			NSView* content_view = [window contentView];
			[content_view setWantsLayer:YES];

			metal_layer = [CAMetalLayer layer];
			metal_layer.device = device;
			metal_layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
			metal_layer.framebufferOnly = YES;
			metal_layer.contentsScale = [window backingScaleFactor];
			metal_layer.backgroundColor = CGColorGetConstantColor(kCGColorBlack);
			[content_view setLayer:metal_layer];

			delegate = [ChaoticMetalWindowDelegate new];
			delegate.closed = &closed;
			[window setDelegate:delegate];

			if (desc.dark_theme && [window respondsToSelector:@selector(setAppearance:)])
				[window setAppearance:[NSAppearance appearanceNamed:NSAppearanceNameDarkAqua]];

			updateDrawableSize();
			[window makeKeyAndOrderFront:nil];
			[NSApp activateIgnoringOtherApps:YES];
		}

		~Impl()
		{
			frame_encoder.finish();
			current_command_buffer = nil;
			current_drawable = nil;

			if (delegate)
				delegate.closed = nullptr;

			if (window)
			{
				[window setDelegate:nil];
				[window close];
			}

			delete render_device;
		}

		void updateDrawableSize()
		{
			if (!window || !metal_layer)
				return;

			NSView* content_view = [window contentView];
			NSRect bounds = [content_view bounds];
			const CGFloat scale = [window backingScaleFactor];

			metal_layer.frame = bounds;
			metal_layer.contentsScale = scale;
			metal_layer.drawableSize = CGSizeMake(
				bounds.size.width * scale,
				bounds.size.height * scale
			);

			const CGSize drawable_size = metal_layer.drawableSize;
			if (!depth_texture ||
				depth_texture_size.width != drawable_size.width ||
				depth_texture_size.height != drawable_size.height)
			{
				const NSUInteger width = drawable_size.width > 1 ? (NSUInteger)drawable_size.width : 1u;
				const NSUInteger height = drawable_size.height > 1 ? (NSUInteger)drawable_size.height : 1u;
				MTLTextureDescriptor* depth_desc = [MTLTextureDescriptor
					texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float
					width:width
					height:height
					mipmapped:NO
				];
				depth_desc.storageMode = MTLStorageModePrivate;
				depth_desc.usage = MTLTextureUsageRenderTarget;
				depth_texture = [device newTextureWithDescriptor:depth_desc];
				USER_CHECK(depth_texture != nil, "Could not create a Metal depth texture.");
				depth_texture_size = drawable_size;
			}
		}

		Vector2i dimensions() const
		{
			if (!window)
				return {};

			NSRect bounds = [[window contentView] bounds];
			return {
				static_cast<int>(bounds.size.width),
				static_cast<int>(bounds.size.height)
			};
		}
	};

	bool Backend::isAvailable() noexcept
	{
		@autoreleasepool
		{
			return MTLCreateSystemDefaultDevice() != nil;
		}
	}

	const char* Backend::name() noexcept
	{
		return "Metal";
	}

	Curve::Curve(render::Device& device, const render::CurveDesc& desc)
		: impl_(new Impl(device, desc))
	{
	}

	Curve::~Curve()
	{
		delete impl_;
	}

	Curve::Curve(Curve&& other) noexcept
		: impl_(std::exchange(other.impl_, nullptr))
	{
	}

	Curve& Curve::operator=(Curve&& other) noexcept
	{
		if (this == &other)
			return *this;

		delete impl_;
		impl_ = std::exchange(other.impl_, nullptr);
		return *this;
	}

	render::BackendType Curve::backendType() const noexcept
	{
		return render::BackendType::Metal;
	}

	void Curve::draw(const render::DrawContext& context)
	{
		USER_CHECK(impl_ != nullptr, "Trying to draw an invalid Metal Curve.");
		USER_CHECK(context.encoder.backendType() == render::BackendType::Metal, "Metal Curve received a command encoder from another backend.");

		impl_->updatePerspectiveBuffer(context);

		context.encoder.setPipeline(*impl_->pipeline);
		context.encoder.setVertexBuffer(*impl_->vertex_buffer, 0u);
		context.encoder.setVertexBuffer(*impl_->perspective_buffer, 1u);
		context.encoder.setVertexBuffer(*impl_->object_buffer, 2u);

		if (impl_->global_color_buffer)
			context.encoder.setFragmentBuffer(*impl_->global_color_buffer);

		context.encoder.draw(impl_->geometry.vertexCount());
	}

	void Curve::draw(render::CommandEncoder& encoder)
	{
		render::Camera camera = {};
		render::DrawContext context{ encoder, camera, { 720, 480 } };
		draw(context);
	}

	void Curve::updateRange(Vector2f range)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Curve.");
		impl_->geometry.updateRange(range);
		impl_->updateVertexBuffer();
	}

	void Curve::updateColors(const Color* color_list)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Curve.");
		impl_->geometry.updateColors(color_list);
		impl_->updateVertexBuffer();
	}

	void Curve::updateGlobalColor(Color color)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Curve.");
		impl_->geometry.updateGlobalColor(color);
		USER_CHECK(impl_->global_color_buffer != nullptr, "Trying to update a non-global Metal Curve color.");

		_float4color metal_color = impl_->geometry.globalColor();
		impl_->global_color_buffer->update(&metal_color, sizeof(metal_color));
	}

	void Curve::updateRotation(Quaternion rotation, bool multiplicative)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Curve.");
		USER_CHECK(rotation, "Invalid quaternion found when trying to update rotation on a Metal Curve.");

		if (multiplicative)
			impl_->rotation *= rotation.normal();
		else
			impl_->rotation = rotation;

		impl_->rotation.normalize();
		impl_->updateObjectBuffer();
	}

	void Curve::updatePosition(Vector3f position, bool additive)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Curve.");

		if (additive)
			impl_->position += position;
		else
			impl_->position = position;

		impl_->updateObjectBuffer();
	}

	void Curve::updateDistortion(Matrix distortion, bool multiplicative)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Curve.");

		if (multiplicative)
			impl_->distortion = distortion * impl_->distortion;
		else
			impl_->distortion = distortion;

		impl_->updateObjectBuffer();
	}

	void Curve::updateScreenPosition(Vector2f screen_displacement)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Curve.");
		impl_->object_data.displacement = screen_displacement.getVector4();
		impl_->object_buffer->update(&impl_->object_data, sizeof(impl_->object_data));
	}

	unsigned Curve::vertexCount() const noexcept
	{
		return impl_ ? impl_->geometry.vertexCount() : 0u;
	}

	Quaternion Curve::getRotation() const
	{
		USER_CHECK(impl_ != nullptr, "Trying to get the rotation of an invalid Metal Curve.");
		return impl_->rotation;
	}

	Vector3f Curve::getPosition() const
	{
		USER_CHECK(impl_ != nullptr, "Trying to get the position of an invalid Metal Curve.");
		return impl_->position;
	}

	Matrix Curve::getDistortion() const
	{
		USER_CHECK(impl_ != nullptr, "Trying to get the distortion of an invalid Metal Curve.");
		return impl_->distortion;
	}

	Vector2f Curve::getScreenPosition() const
	{
		USER_CHECK(impl_ != nullptr, "Trying to get the screen position of an invalid Metal Curve.");
		return { impl_->object_data.displacement.x, impl_->object_data.displacement.y };
	}

	Scatter::Scatter(render::Device& device, const render::ScatterDesc& desc)
		: impl_(new Impl(device, desc))
	{
	}

	Scatter::~Scatter()
	{
		delete impl_;
	}

	Scatter::Scatter(Scatter&& other) noexcept
		: impl_(std::exchange(other.impl_, nullptr))
	{
	}

	Scatter& Scatter::operator=(Scatter&& other) noexcept
	{
		if (this == &other)
			return *this;

		delete impl_;
		impl_ = std::exchange(other.impl_, nullptr);
		return *this;
	}

	render::BackendType Scatter::backendType() const noexcept
	{
		return render::BackendType::Metal;
	}

	void Scatter::draw(const render::DrawContext& context)
	{
		USER_CHECK(impl_ != nullptr, "Trying to draw an invalid Metal Scatter.");
		USER_CHECK(context.encoder.backendType() == render::BackendType::Metal, "Metal Scatter received a command encoder from another backend.");

		impl_->updatePerspectiveBuffer(context);

		context.encoder.setPipeline(*impl_->pipeline);
		context.encoder.setVertexBuffer(*impl_->vertex_buffer, 0u);
		context.encoder.setVertexBuffer(*impl_->perspective_buffer, 1u);
		context.encoder.setVertexBuffer(*impl_->object_buffer, 2u);

		if (impl_->global_color_buffer)
			context.encoder.setFragmentBuffer(*impl_->global_color_buffer);

		context.encoder.draw(impl_->geometry.pointCount());
	}

	void Scatter::draw(render::CommandEncoder& encoder)
	{
		render::Camera camera = {};
		render::DrawContext context{ encoder, camera, { 720, 480 } };
		draw(context);
	}

	void Scatter::updatePoints(const Vector3f* point_list)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Scatter.");
		impl_->geometry.updatePoints(point_list);
		impl_->updateVertexBuffer();
	}

	void Scatter::updateColors(const Color* color_list)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Scatter.");
		impl_->geometry.updateColors(color_list);
		impl_->updateVertexBuffer();
	}

	void Scatter::updateGlobalColor(Color color)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Scatter.");
		impl_->geometry.updateGlobalColor(color);
		USER_CHECK(impl_->global_color_buffer != nullptr, "Trying to update a non-global Metal Scatter color.");

		_float4color metal_color = impl_->geometry.globalColor();
		impl_->global_color_buffer->update(&metal_color, sizeof(metal_color));
	}

	void Scatter::updateRotation(Quaternion rotation, bool multiplicative)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Scatter.");
		USER_CHECK(rotation, "Invalid quaternion found when trying to update rotation on a Metal Scatter.");

		if (multiplicative)
			impl_->rotation *= rotation.normal();
		else
			impl_->rotation = rotation;

		impl_->rotation.normalize();
		impl_->updateObjectBuffer();
	}

	void Scatter::updatePosition(Vector3f position, bool additive)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Scatter.");

		if (additive)
			impl_->position += position;
		else
			impl_->position = position;

		impl_->updateObjectBuffer();
	}

	void Scatter::updateDistortion(Matrix distortion, bool multiplicative)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Scatter.");

		if (multiplicative)
			impl_->distortion = distortion * impl_->distortion;
		else
			impl_->distortion = distortion;

		impl_->updateObjectBuffer();
	}

	void Scatter::updateScreenPosition(Vector2f screen_displacement)
	{
		USER_CHECK(impl_ != nullptr, "Trying to update an invalid Metal Scatter.");
		impl_->object_data.displacement = screen_displacement.getVector4();
		impl_->object_buffer->update(&impl_->object_data, sizeof(impl_->object_data));
	}

	unsigned Scatter::pointCount() const noexcept
	{
		return impl_ ? impl_->geometry.pointCount() : 0u;
	}

	Quaternion Scatter::getRotation() const
	{
		USER_CHECK(impl_ != nullptr, "Trying to get the rotation of an invalid Metal Scatter.");
		return impl_->rotation;
	}

	Vector3f Scatter::getPosition() const
	{
		USER_CHECK(impl_ != nullptr, "Trying to get the position of an invalid Metal Scatter.");
		return impl_->position;
	}

	Matrix Scatter::getDistortion() const
	{
		USER_CHECK(impl_ != nullptr, "Trying to get the distortion of an invalid Metal Scatter.");
		return impl_->distortion;
	}

	Vector2f Scatter::getScreenPosition() const
	{
		USER_CHECK(impl_ != nullptr, "Trying to get the screen position of an invalid Metal Scatter.");
		return { impl_->object_data.displacement.x, impl_->object_data.displacement.y };
	}

	Window::Window(const WindowDesc& desc)
		: impl_(new Impl(desc))
	{
	}

	Window::~Window()
	{
		delete impl_;
	}

	Window::Window(Window&& other) noexcept
		: impl_(std::exchange(other.impl_, nullptr))
	{
	}

	Window& Window::operator=(Window&& other) noexcept
	{
		if (this == &other)
			return *this;

		delete impl_;
		impl_ = std::exchange(other.impl_, nullptr);
		return *this;
	}

	void Window::setTitle(const char* title)
	{
		if (!impl_ || !impl_->window)
			return;

		[impl_->window setTitle:makeString(title)];
	}

	void Window::setDimensions(Vector2i dimensions)
	{
		if (!impl_ || !impl_->window)
			return;

		const int width = dimensions.x > 0 ? dimensions.x : 1;
		const int height = dimensions.y > 0 ? dimensions.y : 1;
		[impl_->window setContentSize:NSMakeSize(width, height)];
		impl_->updateDrawableSize();
	}

	Vector2i Window::getDimensions() const
	{
		return dimensions();
	}

	bool Window::shouldClose() const
	{
		return !isOpen();
	}

	void Window::close()
	{
		if (!impl_)
			return;

		impl_->closed = true;
		if (impl_->window)
			[impl_->window close];
	}

	render::Device& Window::device()
	{
		USER_CHECK(impl_ && impl_->render_device, "Cannot access a Metal device from an invalid window.");
		return *impl_->render_device;
	}

	const render::Device& Window::device() const
	{
		USER_CHECK(impl_ && impl_->render_device, "Cannot access a Metal device from an invalid window.");
		return *impl_->render_device;
	}

	render::BackendType Window::backendType() const noexcept
	{
		return render::BackendType::Metal;
	}

	Vector2i Window::dimensions() const
	{
		return impl_ ? impl_->dimensions() : Vector2i{};
	}

	bool Window::isOpen() const noexcept
	{
		return impl_ && !impl_->closed;
	}

	render::CommandEncoder* Window::beginFrame(const render::FrameDesc& desc)
	{
		pollEvents();

		if (!impl_ || impl_->closed)
			return nullptr;

		if (impl_->current_command_buffer)
			endFrame();

		@autoreleasepool
		{
			impl_->updateDrawableSize();

			id<CAMetalDrawable> drawable = [impl_->metal_layer nextDrawable];
			if (!drawable)
				return nullptr;

			MTLRenderPassDescriptor* pass = [MTLRenderPassDescriptor renderPassDescriptor];
			pass.colorAttachments[0].texture = drawable.texture;
			pass.colorAttachments[0].loadAction = desc.clear_color_buffer ? MTLLoadActionClear : MTLLoadActionLoad;
			pass.colorAttachments[0].storeAction = MTLStoreActionStore;
			pass.colorAttachments[0].clearColor = makeClearColor(desc.clear_color);
			pass.depthAttachment.texture = impl_->depth_texture;
			pass.depthAttachment.loadAction = desc.clear_depth_buffer ? MTLLoadActionClear : MTLLoadActionLoad;
			pass.depthAttachment.storeAction = MTLStoreActionDontCare;
			pass.depthAttachment.clearDepth = desc.clear_depth;

			id<MTLCommandBuffer> command_buffer = [impl_->command_queue commandBuffer];
			USER_CHECK(command_buffer != nil, "Could not create a Metal command buffer.");

			id<MTLRenderCommandEncoder> encoder = [command_buffer renderCommandEncoderWithDescriptor:pass];
			USER_CHECK(encoder != nil, "Could not create a Metal render command encoder.");

			impl_->current_drawable = drawable;
			impl_->current_command_buffer = command_buffer;
			impl_->frame_encoder.reset(encoder);
		}

		return &impl_->frame_encoder;
	}

	void Window::endFrame()
	{
		if (!impl_ || !impl_->current_command_buffer)
			return;

		impl_->frame_encoder.finish();
		[impl_->current_command_buffer presentDrawable:impl_->current_drawable];
		[impl_->current_command_buffer commit];

		impl_->current_drawable = nil;
		impl_->current_command_buffer = nil;
	}

	bool Window::drawFrame(Color clear_color)
	{
		if (!beginFrame({ clear_color, true }))
			return false;

		endFrame();
		return isOpen();
	}

	void Window::pollEvents()
	{
		ensureApplication();

		@autoreleasepool
		{
			for (;;)
			{
				NSEvent* event = [NSApp
					nextEventMatchingMask:NSEventMaskAny
					untilDate:[NSDate distantPast]
					inMode:NSDefaultRunLoopMode
					dequeue:YES
				];

				if (!event)
					break;

				[NSApp sendEvent:event];
			}

			[NSApp updateWindows];
		}
	}
}
