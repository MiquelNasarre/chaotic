#pragma once

#include "chaotic_core.h"

namespace chaotic::metal
{
	struct WindowDesc
	{
		const char* title = "Chaotic Metal Window";
		Vector2i dimensions = { 720, 480 };
		bool dark_theme = true;
	};

	class Backend
	{
	public:
		static bool isAvailable() noexcept;
		static const char* name() noexcept;
	};

	class Window : public render::RenderTarget
	{
	public:
		explicit Window(const WindowDesc& desc = {});
		~Window();

		Window(Window&& other) noexcept;
		Window& operator=(Window&& other) noexcept;

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		void setTitle(const char* title);
		void setDimensions(Vector2i dimensions);
		Vector2i getDimensions() const;

		bool shouldClose() const;
		void close();
		render::Device& device() override;
		const render::Device& device() const override;

		render::BackendType backendType() const noexcept override;
		Vector2i dimensions() const override;
		bool isOpen() const noexcept override;
		render::CommandEncoder* beginFrame(const render::FrameDesc& desc = {}) override;
		void endFrame() override;

		// Polls macOS events, clears the Metal drawable, presents it, and
		// returns false once the user closes the window.
		bool drawFrame(Color clear_color = Color::Black);

		static void pollEvents();

	private:
		struct Impl;
		Impl* impl_ = nullptr;
	};

	class Curve : public render::Drawable
	{
	public:
		Curve(render::Device& device, const render::CurveDesc& desc);
		~Curve();

		Curve(Curve&& other) noexcept;
		Curve& operator=(Curve&& other) noexcept;

		Curve(const Curve&) = delete;
		Curve& operator=(const Curve&) = delete;

		render::BackendType backendType() const noexcept override;
		void draw(const render::DrawContext& context) override;
		void draw(render::CommandEncoder& encoder);
		void updateRange(Vector2f range = {});
		void updateColors(const Color* color_list);
		void updateGlobalColor(Color color);
		void updateRotation(Quaternion rotation, bool multiplicative = false);
		void updatePosition(Vector3f position, bool additive = false);
		void updateDistortion(Matrix distortion, bool multiplicative = false);
		void updateScreenPosition(Vector2f screen_displacement);

		unsigned vertexCount() const noexcept;
		Quaternion getRotation() const;
		Vector3f getPosition() const;
		Matrix getDistortion() const;
		Vector2f getScreenPosition() const;

	private:
		struct Impl;
		Impl* impl_ = nullptr;
	};

	class Scatter : public render::Drawable
	{
	public:
		Scatter(render::Device& device, const render::ScatterDesc& desc);
		~Scatter();

		Scatter(Scatter&& other) noexcept;
		Scatter& operator=(Scatter&& other) noexcept;

		Scatter(const Scatter&) = delete;
		Scatter& operator=(const Scatter&) = delete;

		render::BackendType backendType() const noexcept override;
		void draw(const render::DrawContext& context) override;
		void draw(render::CommandEncoder& encoder);
		void updatePoints(const Vector3f* point_list);
		void updateColors(const Color* color_list);
		void updateGlobalColor(Color color);
		void updateRotation(Quaternion rotation, bool multiplicative = false);
		void updatePosition(Vector3f position, bool additive = false);
		void updateDistortion(Matrix distortion, bool multiplicative = false);
		void updateScreenPosition(Vector2f screen_displacement);

		unsigned pointCount() const noexcept;
		Quaternion getRotation() const;
		Vector3f getPosition() const;
		Matrix getDistortion() const;
		Vector2f getScreenPosition() const;

	private:
		struct Impl;
		Impl* impl_ = nullptr;
	};
}
