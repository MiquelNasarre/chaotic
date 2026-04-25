#pragma once

#include "Error/_erDefault.h"
#include "Math/Quaternion.h"
#include "chaotic/render/Renderer.h"

#include <vector>

namespace chaotic::render
{
	struct Camera
	{
		::Quaternion observer = 1.f;
		::Vector3f center = {};
		float scale = 250.f;
	};

	struct DrawContext
	{
		CommandEncoder& encoder;
		const Camera& camera;
		::Vector2i dimensions = {};
	};

	class Drawable
	{
	public:
		virtual ~Drawable() = default;
		virtual BackendType backendType() const noexcept = 0;
		virtual void draw(const DrawContext& context) = 0;
	};

	class Scene
	{
	public:
		void pushDrawable(Drawable& drawable)
		{
			drawables_.push_back(&drawable);
		}

		bool eraseDrawable(const Drawable& drawable)
		{
			for (auto it = drawables_.begin(); it != drawables_.end(); ++it)
			{
				if (*it == &drawable)
				{
					drawables_.erase(it);
					return true;
				}
			}

			return false;
		}

		void clearDrawables()
		{
			drawables_.clear();
		}

		unsigned drawableCount() const noexcept
		{
			return static_cast<unsigned>(drawables_.size());
		}

		void setFrameDesc(const FrameDesc& desc)
		{
			frame_desc_ = desc;
		}

		void setClearColor(::Color color)
		{
			frame_desc_.clear_color = color;
			frame_desc_.clear_color_buffer = true;
		}

		const FrameDesc& frameDesc() const noexcept
		{
			return frame_desc_;
		}

		void setPerspective(::Quaternion observer, ::Vector3f center, float scale)
		{
			USER_CHECK(observer, "The scene observer must be a non-zero quaternion.");
			camera_.observer = observer.normal();
			camera_.center = center;
			camera_.scale = scale;
		}

		void setObserver(::Quaternion observer)
		{
			USER_CHECK(observer, "The scene observer must be a non-zero quaternion.");
			camera_.observer = observer.normal();
		}

		void setCenter(::Vector3f center)
		{
			camera_.center = center;
		}

		void setScale(float scale)
		{
			camera_.scale = scale;
		}

		const Camera& camera() const noexcept
		{
			return camera_;
		}

		bool render(RenderTarget& target)
		{
			CommandEncoder* encoder = target.beginFrame(frame_desc_);
			if (!encoder)
				return false;

			DrawContext context{ *encoder, camera_, target.dimensions() };

			for (Drawable* drawable : drawables_)
			{
				USER_CHECK(drawable != nullptr, "Scene contains a null drawable.");
				USER_CHECK(drawable->backendType() == target.backendType(), "Scene drawable backend does not match the render target backend.");
				drawable->draw(context);
			}

			target.endFrame();
			return target.isOpen();
		}

	private:
		Camera camera_ = {};
		FrameDesc frame_desc_ = {};
		std::vector<Drawable*> drawables_;
	};
}
