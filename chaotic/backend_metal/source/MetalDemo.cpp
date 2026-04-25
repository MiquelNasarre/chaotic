#include "chaotic_metal.h"

#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace
{
	Vector3f demoCurve(float t)
	{
		return {
			std::cos(t),
			std::sin(t),
			0.25f * std::sin(3.f * t)
		};
	}

	Color demoColor(float t)
	{
		const auto channel = [](float value) {
			return static_cast<unsigned char>(127.5f + 127.5f * value);
		};

		return {
			channel(std::sin(t)),
			channel(std::sin(t + 2.0943951f)),
			channel(std::sin(t + 4.1887902f)),
			255u
		};
	}
}

int main(int argc, char** argv)
{
	using namespace chaotic;

	int max_frames = 0;
	if (argc == 3 && std::strcmp(argv[1], "--frames") == 0)
		max_frames = std::atoi(argv[2]);

	if (!metal::Backend::isAvailable())
		return 1;

	metal::Window window({
		"Chaotic Metal Demo",
		{ 960, 640 },
		true
	});

	render::Scene scene;
	scene.setClearColor(Color(9u, 11u, 16u));
	scene.setPerspective(Quaternion(1.f), {}, 260.f);

	render::CurveDesc curve_desc = {};
	curve_desc.curve_function = demoCurve;
	curve_desc.range = { -6.2831853f, 6.2831853f };
	curve_desc.vertex_count = 720u;
	curve_desc.coloring = render::CurveColoring::Function;
	curve_desc.color_function = demoColor;

	metal::Curve curve(window.device(), curve_desc);
	scene.pushDrawable(curve);

	std::array<Vector3f, 6> axis_points =
	{
		Vector3f{ -1.35f, 0.f, 0.f }, Vector3f{ 1.35f, 0.f, 0.f },
		Vector3f{ 0.f, -1.35f, 0.f }, Vector3f{ 0.f, 1.35f, 0.f },
		Vector3f{ 0.f, 0.f, -1.35f }, Vector3f{ 0.f, 0.f, 1.35f }
	};

	std::array<Color, 6> axis_colors =
	{
		Color::Red, Color::Red,
		Color::Green, Color::Green,
		Color::Blue, Color::Blue
	};

	render::ScatterDesc axis_desc = {};
	axis_desc.point_list = axis_points.data();
	axis_desc.point_count = static_cast<unsigned>(axis_points.size());
	axis_desc.coloring = render::ScatterColoring::Point;
	axis_desc.color_list = axis_colors.data();
	axis_desc.blending = render::ScatterBlending::Glow;
	axis_desc.line_mesh = true;

	metal::Scatter axes(window.device(), axis_desc);
	scene.pushDrawable(axes);

	const auto start = std::chrono::steady_clock::now();

	int frame_count = 0;
	while (window.isOpen())
	{
		const auto now = std::chrono::steady_clock::now();
		const float seconds = std::chrono::duration<float>(now - start).count();

		curve.updateRotation(Quaternion::Rotation({ 0.35f, 1.f, 0.15f }, seconds * 0.55f));
		axes.updateRotation(Quaternion::Rotation({ 0.35f, 1.f, 0.15f }, seconds * 0.55f));

		if (!scene.render(window))
			break;

		if (max_frames > 0 && ++frame_count >= max_frames)
			break;

		std::this_thread::sleep_for(std::chrono::milliseconds(16));
	}

	return 0;
}
