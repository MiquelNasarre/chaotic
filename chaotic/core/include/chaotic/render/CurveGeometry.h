#pragma once

#include "Error/_erDefault.h"
#include "chaotic/render/Renderer.h"

#include <cstddef>
#include <vector>

namespace chaotic::render
{
	enum class CurveColoring
	{
		Function,
		List,
		Global,
	};

	struct CurveDesc
	{
		::Vector3f(*curve_function)(float) = nullptr;
		::Vector2f range = { -1.f, 1.f };
		unsigned vertex_count = 200u;
		CurveColoring coloring = CurveColoring::Global;
		::Color global_color = ::Color::White;
		::Color(*color_function)(float) = nullptr;
		const ::Color* color_list = nullptr;
		bool enable_transparency = false;
		bool enable_updates = false;
		bool border_points_included = true;
	};

	struct CurveVertex
	{
		::_float4vector position = {};
	};

	struct CurveColorVertex
	{
		::_float4vector position = {};
		::_float4color color = {};
	};

	class CurveGeometry
	{
	public:
		CurveGeometry() = default;

		explicit CurveGeometry(const CurveDesc& desc)
		{
			reset(desc);
		}

		void reset(const CurveDesc& desc)
		{
			desc_ = desc;
			validateDesc();
			initialized_ = true;
			rebuildAll();
		}

		bool isInitialized() const noexcept { return initialized_; }
		const CurveDesc& desc() const noexcept { return desc_; }
		CurveColoring coloring() const noexcept { return desc_.coloring; }
		unsigned vertexCount() const noexcept { return initialized_ ? desc_.vertex_count : 0u; }
		PrimitiveTopology topology() const noexcept { return PrimitiveTopology::LineStrip; }

		const std::vector<CurveVertex>& vertices() const noexcept { return vertices_; }
		const std::vector<CurveColorVertex>& colorVertices() const noexcept { return color_vertices_; }

		bool hasVertexColors() const noexcept
		{
			return desc_.coloring != CurveColoring::Global;
		}

		const void* vertexData() const
		{
			USER_CHECK(initialized_, "Trying to access vertex data from an uninitialized CurveGeometry.");
			return hasVertexColors() ? static_cast<const void*>(color_vertices_.data()) : static_cast<const void*>(vertices_.data());
		}

		unsigned vertexStride() const noexcept
		{
			return hasVertexColors() ? static_cast<unsigned>(sizeof(CurveColorVertex)) : static_cast<unsigned>(sizeof(CurveVertex));
		}

		unsigned long long vertexByteSize() const noexcept
		{
			return static_cast<unsigned long long>(vertexCount()) * vertexStride();
		}

		VertexLayoutDesc vertexLayout() const noexcept
		{
			if (hasVertexColors())
			{
				return {
					color_attributes_,
					static_cast<unsigned>(sizeof(color_attributes_) / sizeof(color_attributes_[0])),
					static_cast<unsigned>(sizeof(CurveColorVertex))
				};
			}

			return {
				position_attributes_,
				static_cast<unsigned>(sizeof(position_attributes_) / sizeof(position_attributes_[0])),
				static_cast<unsigned>(sizeof(CurveVertex))
			};
		}

		BufferDesc vertexBufferDesc() const
		{
			USER_CHECK(initialized_, "Trying to create a vertex buffer descriptor from an uninitialized CurveGeometry.");
			return {
				vertexData(),
				vertexByteSize(),
				vertexStride(),
				desc_.enable_updates ? BufferUsage::Dynamic : BufferUsage::Immutable,
				"chaotic_curve_vertices"
			};
		}

		::_float4color globalColor() const noexcept
		{
			return desc_.global_color.getColor4();
		}

		void updateRange(::Vector2f range = {})
		{
			USER_CHECK(initialized_, "Trying to update an uninitialized CurveGeometry.");
			USER_CHECK(desc_.enable_updates, "Trying to update a CurveGeometry with updates disabled.");

			if (range)
				desc_.range = range;

			rebuildForRangeUpdate(false);
		}

		void updateColors(const ::Color* color_list)
		{
			USER_CHECK(initialized_, "Trying to update colors on an uninitialized CurveGeometry.");
			USER_CHECK(desc_.enable_updates, "Trying to update colors on a CurveGeometry with updates disabled.");
			USER_CHECK(desc_.coloring == CurveColoring::List, "Trying to update colors on a CurveGeometry with a different coloring mode.");
			USER_CHECK(color_list != nullptr, "Trying to update CurveGeometry colors from a null color list.");

			desc_.color_list = color_list;
			for (unsigned i = 0u; i < desc_.vertex_count; ++i)
				color_vertices_[i].color = color_list[i].getColor4();
		}

		void updateGlobalColor(::Color color)
		{
			USER_CHECK(initialized_, "Trying to update the global color on an uninitialized CurveGeometry.");
			USER_CHECK(desc_.coloring == CurveColoring::Global, "Trying to update the global color on a CurveGeometry with a different coloring mode.");
			desc_.global_color = color;
		}

	private:
		void validateDesc() const
		{
			USER_CHECK(desc_.curve_function != nullptr, "Found nullptr when trying to create CurveGeometry from a curve function.");
			USER_CHECK(desc_.vertex_count >= 2u, "Found vertex count smaller than two when trying to create CurveGeometry.");

			if (desc_.coloring == CurveColoring::Function)
				USER_CHECK(desc_.color_function != nullptr, "Found nullptr when trying to create function-colored CurveGeometry.");

			if (desc_.coloring == CurveColoring::List)
				USER_CHECK(desc_.color_list != nullptr, "Found nullptr when trying to create list-colored CurveGeometry.");
		}

		float step() const noexcept
		{
			const float divisor = desc_.border_points_included ?
				static_cast<float>(desc_.vertex_count) - 1.f :
				static_cast<float>(desc_.vertex_count) + 1.f;
			return (desc_.range.y - desc_.range.x) / divisor;
		}

		float firstT(float dt) const noexcept
		{
			return desc_.border_points_included ? desc_.range.x : desc_.range.x + dt;
		}

		void rebuildAll()
		{
			vertices_.clear();
			color_vertices_.clear();

			if (hasVertexColors())
				color_vertices_.resize(desc_.vertex_count);
			else
				vertices_.resize(desc_.vertex_count);

			rebuildForRangeUpdate(true);
		}

		void rebuildForRangeUpdate(bool refresh_list_colors)
		{
			const float dt = step();
			const float t0 = firstT(dt);

			switch (desc_.coloring)
			{
			case CurveColoring::Global:
				for (unsigned i = 0u; i < desc_.vertex_count; ++i)
					vertices_[i].position = desc_.curve_function(t0 + static_cast<float>(i) * dt).getVector4();
				break;

			case CurveColoring::List:
				for (unsigned i = 0u; i < desc_.vertex_count; ++i)
					color_vertices_[i].position = desc_.curve_function(t0 + static_cast<float>(i) * dt).getVector4();
				break;

			case CurveColoring::Function:
				for (unsigned i = 0u; i < desc_.vertex_count; ++i)
				{
					const float t = t0 + static_cast<float>(i) * dt;
					color_vertices_[i].position = desc_.curve_function(t).getVector4();
					color_vertices_[i].color = desc_.color_function(t).getColor4();
				}
				break;
			}

			if (desc_.coloring == CurveColoring::List && refresh_list_colors)
			{
				for (unsigned i = 0u; i < desc_.vertex_count; ++i)
					color_vertices_[i].color = desc_.color_list[i].getColor4();
			}
		}

	private:
		inline static constexpr VertexAttributeDesc position_attributes_[1] =
		{
			{ 0u, 0u, static_cast<unsigned>(offsetof(CurveVertex, position)), VertexFormat::Float4 },
		};

		inline static constexpr VertexAttributeDesc color_attributes_[2] =
		{
			{ 0u, 0u, static_cast<unsigned>(offsetof(CurveColorVertex, position)), VertexFormat::Float4 },
			{ 1u, 0u, static_cast<unsigned>(offsetof(CurveColorVertex, color)), VertexFormat::Float4 },
		};

		CurveDesc desc_ = {};
		std::vector<CurveVertex> vertices_;
		std::vector<CurveColorVertex> color_vertices_;
		bool initialized_ = false;
	};
}
