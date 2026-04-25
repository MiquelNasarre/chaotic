#pragma once

#include "Error/_erDefault.h"
#include "chaotic/render/Renderer.h"

#include <cstddef>
#include <vector>

namespace chaotic::render
{
	enum class ScatterColoring
	{
		Point,
		Global,
	};

	enum class ScatterBlending
	{
		Transparent,
		Opaque,
		Glow,
	};

	struct ScatterDesc
	{
		const ::Vector3f* point_list = nullptr;
		unsigned point_count = 0u;
		ScatterColoring coloring = ScatterColoring::Global;
		::Color global_color = ::Color::White;
		const ::Color* color_list = nullptr;
		ScatterBlending blending = ScatterBlending::Glow;
		bool enable_updates = false;
		bool line_mesh = false;
	};

	struct ScatterPoint
	{
		::_float4vector position = {};
	};

	struct ScatterColorPoint
	{
		::_float4vector position = {};
		::_float4color color = {};
	};

	class ScatterGeometry
	{
	public:
		ScatterGeometry() = default;

		explicit ScatterGeometry(const ScatterDesc& desc)
		{
			reset(desc);
		}

		void reset(const ScatterDesc& desc)
		{
			desc_ = desc;
			validateDesc();
			initialized_ = true;
			rebuildAll();
		}

		bool isInitialized() const noexcept { return initialized_; }
		const ScatterDesc& desc() const noexcept { return desc_; }
		ScatterColoring coloring() const noexcept { return desc_.coloring; }
		ScatterBlending blending() const noexcept { return desc_.blending; }
		unsigned pointCount() const noexcept { return initialized_ ? desc_.point_count : 0u; }
		PrimitiveTopology topology() const noexcept { return desc_.line_mesh ? PrimitiveTopology::LineList : PrimitiveTopology::PointList; }

		bool hasPointColors() const noexcept
		{
			return desc_.coloring == ScatterColoring::Point;
		}

		const void* vertexData() const
		{
			USER_CHECK(initialized_, "Trying to access vertex data from an uninitialized ScatterGeometry.");
			return hasPointColors() ? static_cast<const void*>(color_points_.data()) : static_cast<const void*>(points_.data());
		}

		unsigned vertexStride() const noexcept
		{
			return hasPointColors() ? static_cast<unsigned>(sizeof(ScatterColorPoint)) : static_cast<unsigned>(sizeof(ScatterPoint));
		}

		unsigned long long vertexByteSize() const noexcept
		{
			return static_cast<unsigned long long>(pointCount()) * vertexStride();
		}

		VertexLayoutDesc vertexLayout() const noexcept
		{
			if (hasPointColors())
			{
				return {
					color_attributes_,
					static_cast<unsigned>(sizeof(color_attributes_) / sizeof(color_attributes_[0])),
					static_cast<unsigned>(sizeof(ScatterColorPoint))
				};
			}

			return {
				position_attributes_,
				static_cast<unsigned>(sizeof(position_attributes_) / sizeof(position_attributes_[0])),
				static_cast<unsigned>(sizeof(ScatterPoint))
			};
		}

		BufferDesc vertexBufferDesc() const
		{
			USER_CHECK(initialized_, "Trying to create a vertex buffer descriptor from an uninitialized ScatterGeometry.");
			return {
				vertexData(),
				vertexByteSize(),
				vertexStride(),
				desc_.enable_updates ? BufferUsage::Dynamic : BufferUsage::Immutable,
				"chaotic_scatter_points"
			};
		}

		::_float4color globalColor() const noexcept
		{
			return desc_.global_color.getColor4();
		}

		void updatePoints(const ::Vector3f* point_list)
		{
			USER_CHECK(initialized_, "Trying to update points on an uninitialized ScatterGeometry.");
			USER_CHECK(desc_.enable_updates, "Trying to update points on a ScatterGeometry with updates disabled.");
			USER_CHECK(point_list != nullptr, "Trying to update ScatterGeometry points from a null point list.");

			desc_.point_list = point_list;
			rebuildPointPositions();
		}

		void updateColors(const ::Color* color_list)
		{
			USER_CHECK(initialized_, "Trying to update colors on an uninitialized ScatterGeometry.");
			USER_CHECK(desc_.enable_updates, "Trying to update colors on a ScatterGeometry with updates disabled.");
			USER_CHECK(desc_.coloring == ScatterColoring::Point, "Trying to update colors on a ScatterGeometry with global coloring.");
			USER_CHECK(color_list != nullptr, "Trying to update ScatterGeometry colors from a null color list.");

			desc_.color_list = color_list;
			for (unsigned i = 0u; i < desc_.point_count; ++i)
				color_points_[i].color = color_list[i].getColor4();
		}

		void updateGlobalColor(::Color color)
		{
			USER_CHECK(initialized_, "Trying to update the global color on an uninitialized ScatterGeometry.");
			USER_CHECK(desc_.coloring == ScatterColoring::Global, "Trying to update the global color on a ScatterGeometry with point coloring.");
			desc_.global_color = color;
		}

	private:
		void validateDesc() const
		{
			USER_CHECK(desc_.point_list != nullptr, "Found nullptr when trying to create ScatterGeometry from a point list.");
			USER_CHECK(desc_.point_count > 0u, "Found zero point count when trying to create ScatterGeometry.");
			USER_CHECK(!desc_.line_mesh || desc_.point_count % 2u == 0u, "Line-mesh ScatterGeometry requires an even point count.");

			if (desc_.coloring == ScatterColoring::Point)
				USER_CHECK(desc_.color_list != nullptr, "Found nullptr when trying to create point-colored ScatterGeometry.");
		}

		void rebuildAll()
		{
			points_.clear();
			color_points_.clear();

			if (hasPointColors())
				color_points_.resize(desc_.point_count);
			else
				points_.resize(desc_.point_count);

			rebuildPointPositions();

			if (hasPointColors())
			{
				for (unsigned i = 0u; i < desc_.point_count; ++i)
					color_points_[i].color = desc_.color_list[i].getColor4();
			}
		}

		void rebuildPointPositions()
		{
			if (hasPointColors())
			{
				for (unsigned i = 0u; i < desc_.point_count; ++i)
					color_points_[i].position = desc_.point_list[i].getVector4();
			}
			else
			{
				for (unsigned i = 0u; i < desc_.point_count; ++i)
					points_[i].position = desc_.point_list[i].getVector4();
			}
		}

	private:
		inline static constexpr VertexAttributeDesc position_attributes_[1] =
		{
			{ 0u, 0u, static_cast<unsigned>(offsetof(ScatterPoint, position)), VertexFormat::Float4 },
		};

		inline static constexpr VertexAttributeDesc color_attributes_[2] =
		{
			{ 0u, 0u, static_cast<unsigned>(offsetof(ScatterColorPoint, position)), VertexFormat::Float4 },
			{ 1u, 0u, static_cast<unsigned>(offsetof(ScatterColorPoint, color)), VertexFormat::Float4 },
		};

		ScatterDesc desc_ = {};
		std::vector<ScatterPoint> points_;
		std::vector<ScatterColorPoint> color_points_;
		bool initialized_ = false;
	};
}
