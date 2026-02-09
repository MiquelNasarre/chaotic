#include "chaotic_defaults.h"
#ifdef _ENABLE_CHAOTIC_DEMO

// Demo Window base class. Since the demo is a multi window setting, some order is needed.
// This base window class provides that order by holding a list to all instances and adding 
// and deleting itself from the list when constructed/destructed, It also forces all windows
// to have a default selector to create new windows, and provides a function to make that 
// selector function properly with imGui.
class demoWindow : public Window
{
public:
	// Public global list that keeps track of currently active demoWindows. 
	// demoWindows add themselves upon creation and erase themselves during deletion.
	static inline vector<demoWindow*> active_windows = {};

	// Event and draw function that should do all event processing and issue 
	// the next draw call. All demoWindows must define this function.
	virtual void event_and_draw() = 0;

	// To be called before event_and_draw(). Events shared by all demo windows, like 
	// new window events, screenshots and some keyboard updates are managed here.
	bool new_window_event();

	// Virtual destructor, erases itself from the active windows list.
	virtual ~demoWindow() { std::erase(active_windows, this); }

private:
	// Defines all the types of demoWindows that exist in an enumerator, 
	// so that they can easily be chosen from the imgui selector.
	enum DEMO_TYPE : int
	{
		NONE,
		LORENTZ,
		HOPF_FIBRATION,
		GAME_OF_LIFE,
		BOUNCING_BALLS,
		OCEAN_PLANET,
		RUBIKS_CUBE,
		SIERPINSKI_TETRA,
		FOURIER,
	}
	add_new_window = NONE;

	// Stores the names of all the demoWindow types for the selector.
	static inline const char* demo_names[] =
	{
		"Lorenz Window",
		"Hopf Fibration Wallpaper",
		"Game of Life Window",
		"Bouncing Balls Simulator",
		"Ocean Planet Window",
		"Rubiks' Cube Window",
		"Sierpinski Tetrahedron",
		"Fourier",
	};

	// To activate/deactivate full screen from keyboard and imgui.
	int screen_mode = 0;
	int info_selector;

	// Capture vairables.
	Image screenshot;
	const char* screenshot_name;

	// Kyeboard shortcuts storage.
	bool ctrl_pressed = false;

protected:
#ifdef _INCLUDE_IMGUI
	// demoWindows will use a default imgui if enabled.
	defaultImGui imgui;
#endif

	// For the windows to know a capture has been scheduled.
	bool capture_scheduled = false;

	// Constructor, adds itself to the list and adds the new Window selector.
	demoWindow(const WINDOW_DESC* pDesc, const char* info, const char* screenshot_name) :
		Window(pDesc), screenshot_name { screenshot_name }
#ifdef _INCLUDE_IMGUI
		, imgui(*this) 
	{ 
		snprintf(imgui.title, 64, "Menu");
		imgui.pushSelector("New Window", { LORENTZ, FOURIER }, (int*)&add_new_window, demo_names);
		imgui.pushSelector("Info", { 0,0 }, &info_selector, &info);

		const char* names[4] = 
		{ 
			"Normal View      (esc)", 
			"Full Screen      (F11)", 
			"Capture Frame (Ctrl+S)", 
			"Hide ImGui    (Ctrl+M)"
		};
		imgui.pushSelector("View", { 1,4 }, &screen_mode, names);
#else
	{
#endif
		active_windows.push_back(this);
	}
};

// Lorenz Attractor demoWindow. This window plots a 3D curve of a Lorenz system.
// The object is interactive and the system parameters can be changed. 
// To do this it uses a curve drawable and updates its shape every draw call
// while shifting the initial position forward.
class LorenzWindow : public demoWindow
{
private:
	static inline const char* info =
		"\n"
		"  What better way to present a library named Chaotic than with one of the \n"
		"  most well-known chaotic systems: the Lorenz attractor. It describes the \n"
		"  motion of a particle evolving under a simple three-dimensional system of \n"
		"  ordinary differential equations.\n"
		"\n"
		"  This system was discovered by Edward Lorenz while studying atmospheric \n"
		"  convection. For certain parameter values, the system becomes extremely \n"
		"  sensitive to initial conditions, fitting the definition of chaos.\n"
		"\n"
		"  How it works:\n"
		"  We define a function that advances a position vector by a small time step \n"
		"  according to the system equations. A 'Curve' object is then created using \n"
		"  this function to compute its points. We also define a coloring function, \n"
		"  with intensity proportional to the parameter value.\n"
		"\n"
		"  Every frame, the initial position is advanced by a fixed amount and the \n"
		"  'Curve' is re-computed, producing the observed motion effect."
		"\n ";

	// Static window descriptor used for all instances of this class.
	static inline const WINDOW_DESC LorentzDescriptor =
	{
		"Chaotic Lorenz Window",
		WINDOW_DESC::WINDOW_MODE_NORMAL,
		{ 1080, 720 },
	};

	// Stores whether this is the first instance, for the initial popup.
	static inline bool is_first = true;
	// Whether the initial popup is showing.
	bool show_popup = false;
	// Window dimensions for the popup method to center itself.
	static inline Vector2i windim; 

	EventData data = {}; // Event data storage for this class.
	Curve attractor;	 // Drawable that represents the lorenz attractor.

	// Imgui function that plots the initial popup.
	static void initial_popup()
	{
# ifdef _INCLUDE_IMGUI
		constexpr Vector2i dim = { 478,300 };

		static const char* popup_message =
			"  This demonstration aims to showcase some of the capabilities \n"
			"  of the library, while also navigating interesting mathematical \n"
			"  concepts. This is done via a multi-window setting, where each \n"
			"  window corresponds to a different kind of plot.\n"
			"\n"
			"  All windows have a menu at the top with different options. To \n"
			"  open a new window, select the 'New Window' option on the menu \n"
			"  bar and choose any of the available ones.\n"
			"\n"
			"  Each window also has an 'Info' section. Read it when you enter \n"
			"  a new window to learn how to use it, the intention behind it, \n"
			"  and how it was implemented. The source code for this demo can \n"
			"  be found in 'chaotic/source/chaotic_demo.cpp'.\n"
			"\n"
			"  The 'View' menu provides display options such as fullscreen \n"
			"  mode, hiding the UI, and taking screenshots. Screenshots are \n"
			"  saved automatically to the executable path.\n"
			"\n"
			"  To close this popup press any key on the keyboard. To make it \n"
			"  appear again, on the Lorenz Window, press (P).";

		if (ImGui::Begin("Welcome to the Chaotic Demo!!", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoMouseInputs))
		{
			ImGui::SetWindowSize(ImVec2((float)dim.x, (float)dim.y), ImGuiCond_Once);
			ImGui::SetWindowPos(ImVec2((windim.x - dim.x) / 2.f, (windim.y - dim.y) / 2.f));

			ImGui::Text(popup_message);
		}
		ImGui::End();
#endif
	}

private:
	// Global private variables to be used by the static curve function.
	static inline Vector3f pos;
	static inline float delta, sigma, rho, beta;

	// Local private variables to store parameters of this specific instance.
	Vector3f my_pos = { 10.f,0.f,25.f };
	float my_delta = 0.001f;
	float my_sigma = 10.f;
	float my_rho = 28.f;
	float my_beta = 8.f / 3.f;
	float speed = 10.f;

	// Lorenz system step function
	static inline void one_step_lorenz(Vector3f& pi, float _delta)
	{
		pi.x += _delta * (sigma * (pi.y - pi.x));
		pi.y += _delta * (pi.x * (rho - pi.z) - pi.y);
		pi.z += _delta * (pi.x * pi.y - beta * pi.z);
	}

	// Lorenz system function, will be used as our curve function.
	static Vector3f system_lorenz(float)
	{
		// Do a step.
		one_step_lorenz(pos, delta);
		// Scale it down and move for convenience.
		return (pos - Vector3f(0.f, 0.f, 25.f)) / 12.f;
	}

public:
	// Constructor, initializes the attractor, sets the initial 
	// view and rotation and pushes the imgui sliders.
	LorenzWindow() : demoWindow(&LorentzDescriptor, info, "lorenz_screenshot")
	{
		// Check if it is the first one.
		if (is_first)
		{
			show_popup = true;
			is_first = false;
		}

		// Set initial window data
		setScale(400.f);
		data.window = this;
		data.rot_free = Quaternion::Rotation({ -1.f,0.f,0.f }, 3.14159265f / 2.f);
		data.d_rot_free = Quaternion::Rotation({ 0.f, -1.f, 0.f }, 0.0025f);

		// Set your parameters for initial draw
		pos = my_pos;
		delta = my_delta;
		sigma = my_sigma;
		rho = my_rho;
		beta = my_beta;

		// Create the curve descriptor
		CURVE_DESC desc = {};
		desc.vertex_count = 5000;
		desc.curve_function = system_lorenz;
		desc.coloring = CURVE_DESC::FUNCTION_COLORING;
		desc.color_function = [](float t) { return ((t + 0.2f) / 1.2f) * Color::White; };
		desc.enable_updates = true;

		// Initialize the curve
		attractor.initialize(&desc);

#ifdef _INCLUDE_IMGUI
		// Add imgui sliders and default settings.
		imgui.pushSlider(&my_sigma, { 0.f,20.f }, "sigma");
		imgui.pushSlider(&my_rho, { 0.f,50.f }, "rho");
		imgui.pushSlider(&my_beta, { 0.f,05.f }, "beta");
		imgui.pushSlider(&speed, { 0.f,50.f }, "speed");
		imgui.initial_size = { 315,150 };
#endif
	}

	// Moves forward, recomputes the attractor, does some default
	// event management and draws the next frame.
	void event_and_draw() override
	{
		// Look for popup.
		if (hasFocus())
			while (char c = Keyboard::popChar())
			{
				if (show_popup)
					show_popup = false;
				else if (c == 'p' || c == 'P')
					show_popup = true;
			}

		// Set your parameters
		sigma = my_sigma;
		rho = my_rho;
		beta = my_beta;
		delta = my_delta;

		// Step your position according to your speed and set it.
		float count, dec = modff(speed, &count);
		for (unsigned i = 0u; i < (unsigned)count; i++)
			one_step_lorenz(my_pos, delta);
		one_step_lorenz(my_pos, dec * delta);
		pos = my_pos;

		// Update the attractor.
		attractor.updateRange({ 0.f,1.f });

		// Do some event management.
		defaultEventManager(data);
		graphics().setScale(data.scale);
		attractor.updateRotation(data.rot_free);

		// Clear buffers and set target.
		graphics().setRenderTarget();
		graphics().clearBuffer();
		// Draw.
		attractor.Draw();
#ifdef _INCLUDE_IMGUI
		// Popup if active.
		windim = getDimensions();
		if (show_popup)
			imgui.inject(&initial_popup);
		else
			imgui.inject(nullptr);
#endif
		// Push.
		graphics().pushFrame();
		return;
	}
};

// Have you ever wanted to have cool moving mathematically based wallpapers? I have. That's
// why the windows in this library can also be created as desktop backgrounds. This demoWindow
// is a small showcase of what can be done with these, in this case you can create your own 
// projection of the 4D sphere to have as a background! Based on a stereographics projection 
// of Hopf fibers, you can customize it as much as you want.
class HopfFibrationWallpaper : demoWindow
{
private:
	static inline const char* info =
		"\n"
		"  Who doesn't want a cool interactive wallpaper on their desktop background? With \n"
		"  Chaotic, you can have it. This window is just an example of what that could look \n"
		"  like. In Chaotic, all windows can be initialized as wallpapers, so there are \n"
		"  virtually no limitations to what you can plot there.\n"
		"\n"
		"  If you are using a multi-monitor setup, you can also select which monitor displays \n"
		"  the background. This setting can be found next to the 'Info' section.\n"
		"\n"
		"  Now, what are we looking at? For centuries, the fourth dimension has inspired \n"
		"  mathematicians. It is the basis of many constructions used in applied mathematics \n"
		"  today, yet it often feels too disconnected from our spatial intuition to even \n"
		"  attempt to visualize. That, however, has not stopped us from trying.\n"
		"\n"
		"  Discovered by Heinz Hopf in 1931, the Hopf fibration describes a map from S^3 \n"
		"  to S^2, where each point's preimage corresponds to a circle on the hypersphere. \n"
		"  If you take a set of points on S^2, find their circular preimages in S^3, and \n"
		"  then project them back to R^3 using a stereographic projection, you obtain a \n"
		"  cool visualization of the hypersphere.\n"
		"\n"
		"  In this case, the circles shown in the helper window around the small sphere \n"
		"  represent the set of points for which we generate fibrations. Thisis a fixed \n"
		"  amount that can be adjusted by the user in the settings (250 fibrations per \n"
		"  circle by default).\n"
		"\n"
		"  Playing with the settings and sliders highlights the level of customization that \n"
		"  can be achieved with just a few functions. You can add new axes, rotate them \n"
		"  individually or globally, and modify multiple function parameters.\n"
		"\n"
		"  How it works:\n"
		"  This plot uses two different windows: one initialized as a wallpaper, which lives \n"
		"  on the desktop background and does not receive focus, and a helper window that \n"
		"  allows direct interaction.\n"
		"\n"
		"  To display the inner reference sphere, we define a spherical surface of radius \n"
		"  one with dim lighting. For the surrounding circles, we define a basis on the \n"
		"  sphere using their axis and a theta value, which represents the circle's angular \n"
		"  distance from the pole. By recomputing this for each circle using the same \n"
		"  drawable, we obtain all reference plots.\n"
		"\n"
		"  For the fibrations, we follow a similar approach: using a single 'Curve', we \n"
		"  assign an S^2 position to each fiber, compute a reference point in R^4, and \n"
		"  generate a circle around it. Repeating this process for every fiber produces \n"
		"  the complete visualization."
		"\n ";

	// Static window descriptor used for the helper window.
	static inline const WINDOW_DESC HelperDescriptor =
	{
		"Hopf Wallpaper Helper Window",
		WINDOW_DESC::WINDOW_MODE_NORMAL,
		{ 1080, 720 },
	};

	// Static window descriptor used for the wallpaper.
	static inline const WINDOW_DESC WallpaperDescriptor =
	{
		"Hopf Fibration Wallpaper",
		WINDOW_DESC::WINDOW_MODE_WALLPAPER,
		{ 1920, 1080 },
	};

	EventData data = {};         // Event data storage for this class.
	Window wallpaper;            // Window used for drawing the wallpaper.

	Curve fibrations;            // Surface that represent S2 circle projections.
	Curve circles;				 // S2 circles to be embedded into S3.
	Surface reference;			 // S2 translucid reference sphere.

	Image wallpaper_screenshot;  // TO store the wallpaper screenshot.

	// Imgui variable that stores whether to update the monitor.
	int update_monitor = -2;

	// Window dimensions for the popup method to center itself.
	static inline Vector2i windim;

	// Simple initial popup imgui method.
	static void initial_popup()
	{
# ifdef _INCLUDE_IMGUI
		constexpr Vector2i dim = { 400,58 };

		if (ImGui::Begin("Hopf Window Popup", NULL, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoTitleBar))
		{
			ImGui::SetWindowSize(ImVec2((float)dim.x, (float)dim.y), ImGuiCond_Once);
			ImGui::SetWindowPos(ImVec2((windim.x - dim.x) / 2.f, (windim.y - dim.y) / 2.f));

			ImGui::Text("\n  LOOK AT YOUR DESKTOP BACKGROUND!!  (press any key)");
		}
		ImGui::End();
#endif
	}

	// Sees what monitors are available, creates the selector accordingly and takes 
	// care of monitor updates if requested. It adds the axis selector and updates 
	// axis data accordingly.
	void createSelectors()
	{
#ifdef _INCLUDE_IMGUI
		// Delete previous selectors.
		imgui.eraseSelector(3);
		imgui.eraseSelector(3);

		if (wallpaper.isWallpaperWindow())
		{
			// Update monitor if requested.
			if (update_monitor != -2)
			{
				wallpaper.setWallpaperMonitor(update_monitor);
				update_monitor = -2;
			}

			// Count how many monitors you have.
			int monitor_count = 0;
			while (hasMonitor(monitor_count))
				monitor_count++;

			// Create monitor names
			string* names = new string[monitor_count + 1];
			const char** c_names = new const char* [monitor_count + 1];
			names[0] = "Expand to All";
			for (int i = 0; i < monitor_count; i++)
				names[i + 1] = "Monitor " + std::to_string(i);
			for (int i = 0; i < monitor_count + 1; i++)
				c_names[i] = names[i].c_str();

			// Create selector accordingly.
			imgui.pushSelector("Monitor", { -1, monitor_count - 1 }, &update_monitor, c_names);
			delete[] names;
			delete[] c_names;
		}
		// If your device could not create the wallpaper do not give monitor selection options.
		else
		{
			const char* name = " Wallpaper mode unavailable";
			imgui.pushSelector("Monitor", { 0, 0 }, &update_monitor, &name);
		}

		// Add a new axis if requested.
		if (current_axis == -3)
		{
			my_axis.push_back({
				1.f,
				1.f
			});

			current_axis = (int)my_axis.size() - 1;
		}
		if (current_axis == -2)
		{
			if (my_axis.size())
				my_axis.pop_back();
			current_axis = 0;
		}

		// Create axis names
		string* names = new string[my_axis.size() + 3];
		const char** c_names = new const char* [my_axis.size() + 3];
		names[0] = "New axis";
		names[1] = "Pop axis";
		names[2] = "All axis";
		for (int i = 0; i < my_axis.size(); i++)
			names[i + 3] = "Axis " + std::to_string(i);
		for (int i = 0; i < my_axis.size() + 3; i++)
			c_names[i] = names[i].c_str();

		// Create selector accordingly.
		imgui.pushSelector("Axis", { -3, (int)my_axis.size() - 1 }, &current_axis, c_names);
		delete[] names;
		delete[] c_names;
#endif
	}

private:
	static constexpr unsigned num_points_curve = 100u;	// How many points we consider for each curve.
	static inline float* cache_sin_t = nullptr;			// Cached sinf() values for efficiency
	static inline float* cache_cos_t = nullptr;			// Cached cosf() values for efficiency

	static inline _float4vector r4 = {1.f,0.f,0.f,0.f}; // Reference position on S3 to define the fiber.
	static inline Vector3f axis = { 0.f,0.f,1.f };		// Axis perpendicular to the S2 circles.
	static inline Vector3f ei = { 0.f,1.f,0.f };		// Base element for the S2 circles.
	static inline Vector3f ej = { 1.f,0.f,0.f };		// Base element for the S2 circles.
	static inline float theta = 0.f;					// Latitude value for the S2 circles.

	static inline float g_hue_offset = 0.f;				// HUE offset value for coloring.
	static inline float g_hue_speed = 0.20f;			// HUE range value for coloring.
	static inline unsigned alpha = 88;					// Defines the alpha value for the coloring.

	static inline float pole = 1.02f;					// Defines distance of the poles for the projection.
	static inline float minimum = 0.02f;				// Defines the minimum denominator for the projection.

	struct AXIS // Struct to define an axis.
	{
		Quaternion rotation = 1.f;
		Quaternion d_rotation = 1.f;
	};
	vector<AXIS> my_axis;		// List of currently active axis.
	int current_axis = 0;		// Currently interactive axis.

	float my_pole = 1.02f;		// Local pole value for storage and imgui.
	float my_infinity = 45.f;	// Local minimum inverse for storage and imgui.
	unsigned my_alpha = 80;		// Local alpha value for storage and imgui.

	float squeezing = 0.9f;		// Defines how close together the S2 circles are.
	unsigned num_circles = 3;	// Defines the number of S2 circles per axis.
	unsigned num_fibers = 250;	// Defines the number of fibers to plot for each circle.

	// Initializes sin/cos cache values based on num_points_curve.
	static void init_cache()
	{
		if (cache_cos_t) 
			return;

		cache_sin_t = new float[num_points_curve];
		cache_cos_t = new float[num_points_curve];

		const float step = 2.f * 3.14159265f / (num_points_curve - 1.f);
		for (unsigned i = 0; i < num_points_curve; i++)
		{
			cache_cos_t[i] = cosf(i * step);
			cache_sin_t[i] = sinf(i * step);
		}
	}

	// Function for the S2 circles.
	static Vector3f Circle(float)
	{
		static unsigned count = 0u;
		static float* p_sin_t = cache_sin_t;
		static float* p_cos_t = cache_cos_t;

		const float cos_t = *p_cos_t++, sin_t = *p_sin_t++;
		if (++count == num_points_curve)
		{
			count = 0u;
			p_sin_t = cache_sin_t;
			p_cos_t = cache_cos_t;
		}

		return { cosf(theta) * axis + sinf(theta) * (cos_t * ei + sin_t * ej) };
	}

	// Function that describes the Fibration circles.
	static Vector3f Fibration(float)
	{
		static unsigned count = 0u;
		static float* p_sin_t = cache_sin_t;
		static float* p_cos_t = cache_cos_t;

		const float cos_t = *p_cos_t++, sin_t = *p_sin_t++;
		if (++count == num_points_curve)
		{
			count = 0u;
			p_sin_t = cache_sin_t;
			p_cos_t = cache_cos_t;
		}

		const float re_z1 = r4.x * cos_t - r4.y * sin_t;
		const float im_z1 = r4.x * sin_t + r4.y * cos_t;
		const float re_z2 = r4.z * cos_t - r4.w * sin_t;
		const float im_z2 = r4.z * sin_t + r4.w * cos_t;

		float proj = (pole - im_z2);
		if (proj >= 0.f && proj < minimum) proj = minimum;
		if (proj < 0.f && proj > -minimum) proj = -minimum;

		return { re_z1 / proj, im_z1 / proj, re_z2 / proj };
	}

	// HUE coloring to get cool gradients.
	static Color Coloring(float)
	{
		static unsigned count = 0u;
		static float* p_cos_t = cache_cos_t;

		const float cos_t = *p_cos_t++;
		if (++count == num_points_curve)
		{
			count = 0u;
			p_cos_t = cache_cos_t;
		}

		float h = g_hue_offset + g_hue_speed * cos_t; h -= floorf(h);

		const float hp = h * 6.f;
		const float x = 1.f - fabsf(fmodf(hp, 2.f) - 1.f);

		float r = 0.f, g = 0.f, b = 0.f;
		if (0.f <= hp && hp < 1.f) r = 1.f, g = x, b = 0.f;
		else if (1.f <= hp && hp < 2.f) r = x, g = 1.f, b = 0.f;
		else if (2.f <= hp && hp < 3.f) r = 0.f, g = 1.f, b = x;
		else if (3.f <= hp && hp < 4.f) r = 0.f, g = x, b = 1.f;
		else if (4.f <= hp && hp < 5.f) r = x, g = 0.f, b = 1.f;
		else                            r = 1.f, g = 0.f, b = x;

		return Color(
			(unsigned char)(255.f * r),
			(unsigned char)(255.f * g),
			(unsigned char)(255.f * b),
			(unsigned char)alpha
		);
	}

	// Runs through all points of all circles of all axis and plots the fibrations
	// and S2 circles as specified by the settings.
	void plot_fibrations()
	{
		pole = my_pole;
		alpha = my_alpha;
		minimum = 1.f / my_infinity;

		g_hue_offset = 0.2f;

		for (AXIS& a : my_axis)
		{
			g_hue_offset += 0.45f - 0.2f;

			axis = (a.rotation * Quaternion(0.f,0.f,1.f,0.f) * a.rotation.inv()).getVector();
			Vector3f a = (axis.x < 0.95f && axis.x > -0.95f) ? Vector3f{ 1.f, 0.f, 0.f } : Vector3f{ 0.f, 1.f, 0.f };

			ei = (axis * a).normal();
			ej = (axis * ei).normal();

			float step_theta = 3.14159265f / (num_circles + 1);
			for (unsigned circ = 0; circ < num_circles; circ++)
			{
				g_hue_offset += 0.2f / num_circles - 0.2f;

				theta = ((1 + circ) * step_theta + 3.14159265f / 2.f * (squeezing - 1.f)) / squeezing;

				graphics().setRenderTarget();
				circles.updateRange();
				circles.Draw();
				wallpaper.graphics().setRenderTarget();

				float step_phi = 2.f * 3.14159265f / num_fibers;
				for (unsigned i = 0; i < num_fibers; i++)
				{
					g_hue_offset += 0.2f / num_fibers;

					float phi = i * step_phi;

					Vector3f r3 = (cosf(theta) * axis + sinf(theta) * (cosf(phi) * ei + sinf(phi) * ej)).normal();

					r4.z = sqrtf((1.f - r3.z) / 2.f);
					r4.w = 0.f;
					r4.x = r3.x * 0.5f / r4.z;
					r4.y = r3.y * 0.5f / r4.z;

					fibrations.updateRange();
					fibrations.Draw();
				}
			}
		}
	}

public:
	// Constructor, initializes the window and the wallpaper, and Sets initial values. 
	// Initializes the reference sphere, the S2 circle and the fibration curve, and 
	// pushes all the imgui sliders.
	HopfFibrationWallpaper() : demoWindow(&HelperDescriptor, info, "hopf_helper_screenshot"), wallpaper(&WallpaperDescriptor)
	{
		// Initialize imaginary circle for efficiency.
		init_cache();
		// Enable transparency on the wallpaper.
		setScale(400.f);
		wallpaper.enableTransparency();
		wallpaper.setScale(200.f);

		// Set initial axis.
		my_axis.push_back({
			Quaternion::Rotation({0.f,0.f,1.f}, 3.14159265f / 5.f),
			Quaternion::Rotation({ 1.f, 1.f, -1.f }, -0.008f)
		});

		// Initialize sphere.
		SURFACE_DESC surf_desc = {};
		surf_desc.enable_illuminated = true;
		surf_desc.default_initial_lights = false;
		surf_desc.global_color = Color(45, 45, 45);
		surf_desc.type = SURFACE_DESC::SPHERICAL_SURFACE;
		surf_desc.spherical_func = [](float, float, float) { return 0.85f; };
		reference.initialize(&surf_desc);
		reference.updateLight(0, { 950.f,200.f }, Color::White, { 10.f, 20.f, -30.f });

		// Initialize the fibrations.
		CURVE_DESC desc = {};
		desc.coloring = CURVE_DESC::FUNCTION_COLORING;
		desc.curve_function = Fibration;
		desc.color_function = Coloring;
		desc.enable_updates = true;
		desc.enable_transparency = true;
		desc.range = { 0.f,2.f * 3.14159265f };
		desc.vertex_count = num_points_curve;
		fibrations.initialize(&desc);

		// Initialize circles.
		desc.enable_transparency = false;
		desc.curve_function = Circle;
		circles.initialize(&desc);

#ifdef _INCLUDE_IMGUI
		imgui.pushSlider(&my_pole, { 0.5f,1.5f }, "Pole");
		imgui.pushSlider(&my_infinity, { 1.f,50.f }, "Infinity");
		imgui.pushSlider(&squeezing, { 0.01f,4.f }, "Squeeze");
		imgui.pushSliderInt((int*)&num_circles, {1,20}, "Circles");
		imgui.pushSliderInt((int*)&num_fibers, {1,400}, "Fibers");
		imgui.pushSliderInt((int*)&my_alpha, { 0,255 }, "Alpha");
		imgui.initial_size = { 315,190 };
		if (wallpaper.isWallpaperWindow())
			imgui.inject(&initial_popup);
#endif
	}

	// Calls for monitor and axis selecting management, computes the new axis rotation
	// based on the current active axis, calls for the plot of all drawables and pushes
	// the next frame.
	void event_and_draw() override
	{
		// Take care of monitor selecting.
		createSelectors();

		// Popup managements
#ifdef _INCLUDE_IMGUI
		if (hasFocus() && Keyboard::popChar() != '\0')
			imgui.inject(nullptr);
		windim = getDimensions();
#endif

		// Set the delta rotation to the current axis one.
		if (my_axis.size())
		{
			if (current_axis == -1)
				data.d_rot_free = my_axis[0].d_rotation;
			else
				data.d_rot_free = my_axis[current_axis].d_rotation;
		}

		// Get the new rotation.
		if (hasFocus())
		{
			// We get the scene perspective data from the window.
			Vector2i dim = getDimensions() / 2;

			// Get the mouse wheel spin change
			data.d_mouse_wheel = (float)Mouse::getWheel();
			data.scale = 400.f;

			// Get the mouse new position. 
			data.last_mouse = data.new_mouse;
			data.new_mouse = Mouse::getPosition();
			if (!data.dragging && Mouse::isButtonPressed(Mouse::Left))
				data.last_mouse = data.new_mouse, data.dragging = true;
			else if (!Mouse::isButtonPressed(Mouse::Left))
				data.dragging = false;

			// Convert the mouse position to R2 given the window dimensions and scale.
			data.R2_last_mouse = { (data.last_mouse.x - dim.x) / data.scale, -(data.last_mouse.y - dim.y) / data.scale };
			data.R2_new_mouse = { (data.new_mouse.x - dim.x) / data.scale, -(data.new_mouse.y - dim.y) / data.scale };

			// Convert the positions to the sphere given the observer and the projections of the mouse to the sphere.
			Vector3f p0 = { data.R2_last_mouse.x, data.R2_last_mouse.y, -data.sensitivity };
			Vector3f p1 = { data.R2_new_mouse.x,  data.R2_new_mouse.y, -data.sensitivity };

			data.S2_last_mouse = p0.normal();
			data.S2_new_mouse = p1.normal();

			if (data.dragging)
			{
				// Get the rotation quaternon that would take you in a straight line from last S2 mouse position to new S2 mouse position.
				Quaternion rot = (Quaternion(data.S2_new_mouse * data.S2_last_mouse) + 1.f + (data.S2_last_mouse ^ data.S2_new_mouse)).normal();

				// Add some wheel spin into the mix.
				Quaternion wheel_spin = Quaternion::Rotation(data.S2_new_mouse, data.d_mouse_wheel / 18000.f);

				// Add some momentum into the mix based on how much the new mouse position would drag you down.
				Quaternion momentum = fabsf(data.d_rot_free.r) < 1.f - 1e-6f ?
					(data.d_rot_free + (1.f - fabsf(data.d_rot_free.getVector().normal() ^ data.S2_new_mouse))).normal() :
					Quaternion(1.f);

				// Compose to get the final free rotation
				data.d_rot_free = wheel_spin * rot * momentum;
			}
			// If not dragging use the wheel value to update the scale
			else wallpaper.graphics().setPerspective(Quaternion::Rotation({ 0.f,0.f,-1.f }, 3.14159265f / 2.f), {}, wallpaper.getScale() * powf(1.1f, data.d_mouse_wheel / 120.f));
		}

		// Update rotations.
		if (current_axis == -1)
			for (AXIS& a : my_axis)
				a.d_rotation = data.d_rot_free;
		else if (my_axis.size())
			my_axis[current_axis].d_rotation = data.d_rot_free;
		for (AXIS& a : my_axis)
			a.rotation *= a.d_rotation;

		// Clear buffers.
		graphics().clearBuffer();
		wallpaper.graphics().clearBuffer();

		// Plot reference sphere.
		graphics().setRenderTarget();
		reference.Draw();

		// Plot everything.
		plot_fibrations();

		// If a capture is scheduled also screenshot the wallpaper.
		if (capture_scheduled)
			wallpaper.scheduleFrameCapture(&wallpaper_screenshot);

		// Push.
		graphics().pushFrame();
		wallpaper.graphics().pushFrame();

		// Save screenshot
		if (capture_scheduled)
		{
			wallpaper_screenshot.save("hopf_wallpaper_screenshot");
			wallpaper_screenshot.reset(0, 0);
		}
		return;
	}
};

// Conway's game of life has always been a very fun concept, and also very easy to write in code.
// With this library you can also bring it to life in your computer and freely interact with it 
// for a couple hundred lines of code. This is definitely one of the funnier demo windows to play 
// with. Choose the correct brush and start creating life!
class GameOfLifeWindow : public demoWindow
{
private:
	static inline const char* info =
		"\n"
		"  Conway's Game of Life has always been a very fun concept, and also very easy to \n"
		"  write in code. With this library, you can also bring it to life on your computer \n"
		"  and freely interact with it, for just a couple hundred lines of code.\n"
		"\n"
		"  Game of Life rules are simple. Every step, the following happens: if a cell is \n"
		"  alive and it has 2 or 3 alive cells around it, it survives; if it has less than \n"
		"  2, it dies of underpopulation; if it has more than 3, it dies of overpopulation. \n"
		"  If a dead cell has exactly 3 alive cells around it, it comes to life.\n"
		"\n"
		"  This window gives you a 500x500 canvas, starting with a glider gun in the middle. \n"
		"  With the left click, you can explore around the board and use the mouse wheel to \n"
		"  zoom. With the right click, you can paint! Choose the correct brush in the menu \n"
		"  and create your own Game of Life boards.\n"
		"\n"
		"  How it works:\n"
		"  Two classes are key to creating this kind of arcade game. The 'Image' class gives \n"
		"  you a canvas to paint to, and something to look at. The 'Background' class allows \n"
		"  us to display images directly onto the screen, or a subrectangle of them. If we \n"
		"  set the background to pixelated, we are done with the setup.\n"
		"\n"
		"  For the game itself, every time we want to update, we iterate through the pixels \n"
		"  of the 'Image'. Following the game rules, we update the colors between black and \n"
		"  white accordingly, update the Texture with the new image on the Background, and \n"
		"  we have a step done.\n"
		"\n"
		"  For user interaction, we keep track of the mouse position and movements to \n"
		"  determine which cell it is hovering over, and paint there if the right button is \n"
		"  clicked. If the left button is clicked, we keep that cell fixed with respect to \n"
		"  the mouse while dragging, updating the image rectangle on the background \n"
		"  accordingly."
		"\n ";

	// Static window descriptor used by all instances of the class.
	static inline const WINDOW_DESC GameWindowDescriptor =
	{
		"Game of Life Window",
		WINDOW_DESC::WINDOW_MODE_NORMAL,
		{ 1080, 720 },
	};

	Image game_board; // Image where the game of life is stored.
	Image buffer;	  // Buffer used for board updates.
	Background back;  // Drawable where the image will be displayed.

private:
	int speed = 10u;			// Imgui toggle for game speed.
	unsigned frames_wait = 10u; // frames to wait between updates.
	unsigned frame_count = 1u;  // Current frame count for next update.
	unsigned pixels_cell = 10u; // How many pixels each cell occupies.

	Vector2f top_left_view = { 196.f,214.f }; // Where the top left point of the screen is in cells.

	// The different brush types enumerator.
	enum BRUSH_TYPE : int
	{
		SPAWN,
		ERASE,
		GLIDER,
		GLIDER_GUN,
	} brush = SPAWN;

	// Updates the game of life board.
	void updateBoard()
	{
		bool alive;
		unsigned neighboors;

		const unsigned h = game_board.height();
		const unsigned w = game_board.width();

		buffer = game_board;
		for (unsigned r = 0u; r < h; r++)
			for (unsigned c = 0u; c < w; c++)
			{
				alive = (game_board(r, c).R > 0u);
				neighboors = 0u;

				if (r > 0u    && game_board(r - 1, c).R > 0u) neighboors++;
				if (r < h - 1 && game_board(r + 1, c).R > 0u) neighboors++;
				if (c > 0u    && game_board(r, c - 1).R > 0u) neighboors++;
				if (c < w - 1 && game_board(r, c + 1).R > 0u) neighboors++;

				if (r > 0u    && c > 0u    && game_board(r - 1, c - 1).R > 0u) neighboors++;
				if (r < h - 1 && c > 0u    && game_board(r + 1, c - 1).R > 0u) neighboors++;
				if (r > 0u    && c < w - 1 && game_board(r - 1, c + 1).R > 0u) neighboors++;
				if (r < h - 1 && c < w - 1 && game_board(r + 1, c + 1).R > 0u) neighboors++;

				if (alive && (neighboors < 2 || neighboors > 3)) buffer(r, c) = Color::Transparent;
				else if (!alive && neighboors == 3) buffer(r, c) = Color::White;
			}

		game_board = buffer;
	}

	// Spawns a glider gun.
	void spawnGun(int r, int c)
	{
		if (r < 4  || r >= (int)game_board.height() - 4) return;
		if (c < 19 || c >= (int)game_board.width() - 16) return;

		// Left dot
		game_board(r + 250 - 250, c + 231 - 250) = Color::White;
		game_board(r + 250 - 250, c + 232 - 250) = Color::White;
		game_board(r + 251 - 250, c + 231 - 250) = Color::White;
		game_board(r + 251 - 250, c + 232 - 250) = Color::White;

		// Right dot
		game_board(r + 248 - 250, c + 265 - 250) = Color::White;
		game_board(r + 248 - 250, c + 266 - 250) = Color::White;
		game_board(r + 249 - 250, c + 265 - 250) = Color::White;
		game_board(r + 249 - 250, c + 266 - 250) = Color::White;

		// Left glider
		game_board(r + 250 - 250, c + 241 - 250) = Color::White;
		game_board(r + 251 - 250, c + 241 - 250) = Color::White;
		game_board(r + 252 - 250, c + 241 - 250) = Color::White;
		game_board(r + 249 - 250, c + 242 - 250) = Color::White;
		game_board(r + 253 - 250, c + 242 - 250) = Color::White;
		game_board(r + 248 - 250, c + 243 - 250) = Color::White;
		game_board(r + 254 - 250, c + 243 - 250) = Color::White;
		game_board(r + 248 - 250, c + 244 - 250) = Color::White;
		game_board(r + 254 - 250, c + 244 - 250) = Color::White;
		game_board(r + 251 - 250, c + 245 - 250) = Color::White;
		game_board(r + 249 - 250, c + 246 - 250) = Color::White;
		game_board(r + 253 - 250, c + 246 - 250) = Color::White;
		game_board(r + 250 - 250, c + 247 - 250) = Color::White;
		game_board(r + 251 - 250, c + 247 - 250) = Color::White;
		game_board(r + 252 - 250, c + 247 - 250) = Color::White;
		game_board(r + 251 - 250, c + 248 - 250) = Color::White;

		// Right glider
		game_board(r + 248 - 250, c + 251 - 250) = Color::White;
		game_board(r + 249 - 250, c + 251 - 250) = Color::White;
		game_board(r + 250 - 250, c + 251 - 250) = Color::White;
		game_board(r + 248 - 250, c + 252 - 250) = Color::White;
		game_board(r + 249 - 250, c + 252 - 250) = Color::White;
		game_board(r + 250 - 250, c + 252 - 250) = Color::White;
		game_board(r + 247 - 250, c + 253 - 250) = Color::White;
		game_board(r + 251 - 250, c + 253 - 250) = Color::White;
		game_board(r + 247 - 250, c + 255 - 250) = Color::White;
		game_board(r + 246 - 250, c + 255 - 250) = Color::White;
		game_board(r + 251 - 250, c + 255 - 250) = Color::White;
		game_board(r + 252 - 250, c + 255 - 250) = Color::White;
	}

public:
	// Constructor, creates the board as an image and spawns the first glider gun. Creates the 
	// descriptor and the background drawable. Pushes slider and brush selector to imgui.
	GameOfLifeWindow() : demoWindow(&GameWindowDescriptor, info, "game_of_life_screenshot")
	{
		game_board.reset(500, 500);
		spawnGun(250, 250);

		BACKGROUND_DESC desc = {};
		desc.pixelated_texture = true;
		desc.texture_updates = true;
		desc.override_buffers = true;
		desc.image = &game_board;

		back.initialize(&desc);
#ifdef _INCLUDE_IMGUI
		imgui.pushSliderInt(&speed, { 0,20 }, "Speed");
		imgui.initial_size = { 315,75 };
		// Add the brush selector.
		const char* names[] =
		{
			"Spawn",
			"Erase",
			"Glider",
			"Glider Gun",
		};
		imgui.pushSelector("Brush", { SPAWN, GLIDER_GUN }, (int*)&brush, names);
#endif
	}

	// Manages step events, and if the window has focus manages painting, displacements 
	// and resizing. Once all the events are taken care of it updates the texture, updates 
	// rectangle and pushes a new frame.
	void event_and_draw() override
	{
		// Update wait time.
		frames_wait = 20 - speed;
		if (speed < 10) frames_wait = unsigned(float(frames_wait) * powf(10.f, (10.f - speed) / 10.f));

		// Check for board updates.
		if (speed && frame_count >= frames_wait)
		{
			updateBoard();
			frame_count = 0u;
		}
		else frame_count++;

		// Check for dragging and resize,
		static bool dragging = false;
		if (hasFocus())
		{
			if (Mouse::isButtonPressed(Mouse::Right))
			{
				Vector2i pos = Mouse::getPosition();
				Vector2f board_pos = top_left_view + Vector2f(pos) / float(pixels_cell);

				unsigned row = unsigned(board_pos.y);
				unsigned col = unsigned(board_pos.x);

				switch (brush)
				{
				case SPAWN:
					game_board(row, col) = Color::White;
					break;

				case ERASE:
					game_board(row, col) = Color::Transparent;
					break;

				case GLIDER:
					if (row < 1 || col < 1 || row > game_board.height() - 2 || col > game_board.width() - 2)
						break;
					game_board(row - 1, col) = Color::White;
					game_board(row + 1, col) = Color::White;
					game_board(row, col - 1) = Color::White;
					game_board(row + 1, col - 1) = Color::White;
					game_board(row + 1, col + 1) = Color::White;
					break;

				case GLIDER_GUN:
					spawnGun(row, col);
					break;

				default:
					break;
				}
			}

			static Vector2i last_mouse = {};

			if (!Mouse::isButtonPressed(Mouse::Left))
				dragging = false;

			else if (!dragging)
			{
				last_mouse = Mouse::getPosition();
				dragging = true;
			}
			else
			{
				Vector2i new_mouse = Mouse::getPosition();
				Vector2i displacement = new_mouse - last_mouse;
				last_mouse = new_mouse;

				top_left_view -= Vector2f(displacement) / float(pixels_cell);
			}

			if (int wheel = Mouse::getWheel())
			{
				Mouse::resetWheel();

				Vector2i mouse_pos = Mouse::getPosition();
				Vector2f board_pos = top_left_view + Vector2f(mouse_pos) / float(pixels_cell);

				if (wheel > 0)
					pixels_cell++;
				else if (
					float(game_board.width())  > float(getDimensions().x) / (pixels_cell - 1) &&
					float(game_board.height()) > float(getDimensions().y) / (pixels_cell - 1)
					)
					pixels_cell--;

				top_left_view = board_pos - Vector2f(mouse_pos) / float(pixels_cell);
			}
		}
		else dragging = false;

		// Update texture.
		back.updateTexture(&game_board);

		// Sanity check.
		while (
			float(game_board.width()) < float(getDimensions().x) / pixels_cell ||
			float(game_board.height()) < float(getDimensions().y) / pixels_cell)
			pixels_cell++;
		if (top_left_view.x > game_board.width() - float(getDimensions().x) / pixels_cell) top_left_view.x = game_board.width() - float(getDimensions().x) / pixels_cell;
		if (top_left_view.y > game_board.height() - float(getDimensions().y) / pixels_cell) top_left_view.y = game_board.height() - float(getDimensions().y) / pixels_cell;
		if (top_left_view.x < 0.f) top_left_view.x = 0.f;
		if (top_left_view.y < 0.f) top_left_view.y = 0.f;

		// Update rectangle view.
		back.updateRectangle(top_left_view, top_left_view + Vector2f(getDimensions()) / float(pixels_cell));

		// Set target.
		graphics().setRenderTarget();
		// Draw.
		back.Draw();
		// Push.
		graphics().pushFrame();
	}
};

// Physics simulations are a big part of my love for programming, mathematically it is quite complex
// to calculate some movements precisely, but once you discretize it and plug it into a computer it 
// becomes trivial. This dummy simulation is just a bunch of balls jumping inside a cube. But still, 
// almost impossible to calculate, really easy to compute, and with this library, you can plot it.
class BouncingBallsWindow : public demoWindow
{
private:
	static inline const char* info =
		"\n"
		"  Physics simulations are a big part of my love for programming. From a mathematical \n"
		"  point of view, it is quite complex to calculate some movements precisely, but once \n"
		"  you discretize them and plug them into a computer, they become trivial. This dummy \n"
		"  simulation is just a bunch of balls bouncing inside a cube. Still, it is almost \n"
		"  impossible to calculate analytically, really easy to compute, and with this library,\n"
		"  you can plot it!\n"
		"\n"
		"  Use the mouse to move the cube around; the balls will stay in place, and react \n"
		"  accordingly when hitting the cube boundaries or bouncing against each other. You \n"
		"  can press 'A' to add more balls or 'C' to clear them. You will also find some \n"
		"  sliders on the screen to change different variables of the simulation.\n"
		"\n"
		"  How it works:\n"
		"  We define a single ball as a spherical surface with fixed radius. This ball is then \n"
		"  reused to plot all the balls, by deforming it to the desired radius, setting a \n"
		"  different color, moving it to the correct position, and drawing it, repeating the \n"
		"  process for each ball in the list.\n"
		"\n"
		"  For the cube, we define a single polyhedron with transparency enabled and plot it \n"
		"  twice: a smaller one with side length 1.00, and a bigger one with side length 1.05, \n"
		"  obtained through a linear deformation. This gives the effect of thickness.\n"
		"\n"
		"  Cube rotation is handled by the default event manager, and the rest comes down to \n"
		"  how interactions are defined. In this case, collisions are checked 100 steps per \n"
		"  frame, the cube is assumed to have infinite mass, and collisions are considered \n"
		"  elastic with some energy loss."
		"\n ";

	// Static window descriptor used by all instances of the class.
	static inline const WINDOW_DESC BouncingDescriptor =
	{
		"Bouncing Balls Simulator",
		WINDOW_DESC::WINDOW_MODE_NORMAL,
		{ 1080, 720 },
	};

	EventData data = {}; // Stores the event data for the window.
	Surface surf;		 // Drawable to plot the spheres.
	Polyhedron cube;	 // Drawable to plot the cube.
	Scatter axis;		 // Drawable to plot the balls refernce axis.

private:
	static constexpr unsigned steps_per_frame = 100;
	static constexpr Vector3f gravity_step = { 0.f,-0.0000005f,0.0f };

	
	float radius = 0.2f; // Sets the global radius of the spheres.
	float speed = 0.5f;  // Sets the speed of the simulation.
	float gravity = 1.f; // Sets the gravity of the simulation.
	float loss = 0.05f;  // Sets the momentum loss in collisions.

	enum class ACTION : int
	{
		NONE,
		ADD,
		CLEAR
	} action = ACTION::NONE; // Imgui selector

	vector<Vector3f> pos; // Stores the positions of all the balls.
	vector<Vector3f> vel; // Stores the velocities of all the balls.
	vector<Color> colors; // Stores the colors of all the balls.

	// This is the core of the simulation, steps all balls and checks 
	// for collisions, doing momentum transfers accordingly.
	void do_frame_step()
	{
		// Get all the plains accounting for rotation.
		const Vector3f planes[6] =
		{
			(data.rot_free * Quaternion(Vector3f( 1.f, 0.f, 0.f)) * data.rot_free.inv()).getVector(),
			(data.rot_free * Quaternion(Vector3f(-1.f, 0.f, 0.f)) * data.rot_free.inv()).getVector(),
			(data.rot_free * Quaternion(Vector3f( 0.f, 1.f, 0.f)) * data.rot_free.inv()).getVector(),
			(data.rot_free * Quaternion(Vector3f( 0.f,-1.f, 0.f)) * data.rot_free.inv()).getVector(),
			(data.rot_free * Quaternion(Vector3f( 0.f, 0.f, 1.f)) * data.rot_free.inv()).getVector(),
			(data.rot_free * Quaternion(Vector3f( 0.f, 0.f,-1.f)) * data.rot_free.inv()).getVector(),
		};
		// Get rotation axis and angular momentum if rotating.
		const Vector3f rotation_axis = data.d_rot_free.r < 0.9999f ? data.d_rot_free.getVector().normal() : Vector3f{1.f, 0.f, 0.f};
		const float angular_speed = speed ? acosf(data.d_rot_free.r) / steps_per_frame / speed : 0.f;

		// Do this times per frame.
		for (unsigned i = 0; i < steps_per_frame; i++)
		{
			// Step all balls.
			for (unsigned ball = 0; ball < pos.size(); ball++)
			{
				vel[ball] += gravity_step * gravity * speed;
				pos[ball] += vel[ball] * speed;
			}

			// Check for collisions with any object.
			for (unsigned ball = 0; ball < pos.size(); ball++)
			{
				// Check against all plains.
				for (const Vector3f& plain : planes)
				{
					// Find colliding cases.
					float dist = (pos[ball] ^ plain) + radius;
					if (dist >= 1.f)
					{
						// Get the collision speed (moving plain).
						const Vector3f collision_point = pos[ball] * (2.f - dist + radius);
						const Vector3f point_velocity = collision_point * rotation_axis * angular_speed;
						const float extra_kick = -(point_velocity ^ plain);
						// move inside and invert speed + kick.
						pos[ball] -= (dist - 1.f) * plain;
						vel[ball] -= ((vel[ball] ^ plain) * (2.f - loss) + extra_kick * (1.f - loss)) * plain;
					}
				}
				// Check against other balls.
				for (unsigned other = ball + 1; other < pos.size(); other++)
				{
					// Get direction and distance between them.
					Vector3f dir = pos[ball] - pos[other];
					float dist = dir.abs();

					// If not touching continue.
					if (dist >= 2.f * radius) 
						continue;

					// Get normal vector and penetration.
					Vector3f normal = dir / dist;
					float penetration = 2.f * radius - dist;

					// Separate the balls.
					pos[ball] += 0.5f * penetration * normal;
					pos[other] -= 0.5f * penetration * normal;

					// Relative normal velocity.
					float vn = (vel[ball] - vel[other]) ^ normal;

					// If they are not moving towards each othe continue.
					if (vn > 0.f)
						continue;

					// Collide.
					float restitution = 1.f - loss / 2.f;
					Vector3f impulse = -vn * restitution * normal;
					vel[ball] += impulse;
					vel[other] -= impulse;
				}
			}
		}
	}

	// Adds a random ball.
	void add_a_ball()
	{
		Vector3f position = { (rand() / 16384.f) - 1.f,(rand() / 16384.f) - 1.f,(rand() / 16384.f) - 1.f };
		position *= 1.f - radius;
		position = (data.rot_free * Quaternion(position) * data.rot_free.inv()).getVector();

		Vector3f velocity = 0.0003f * Vector3f((rand() / 16384.f) - 1.f, (rand() / 16384.f) - 1.f, (rand() / 16384.f) - 1.f);

		Color color;
		static unsigned next_color = 2;
		next_color++; next_color %= 8;
		switch (next_color)
		{
		case 0:
			color = Color::Red;
			break;
		case 1:
			color = Color::Green;
			break;
		case 2:
			color = Color::Blue;
			break;
		case 3:
			color = Color::Cyan;
			break;
		case 4:
			color = Color::Orange;
			break;
		case 5:
			color = Color::White;
			break;
		case 6:
			color = Color::Purple;
			break;
		case 7:
			color = Color::Yellow;
			break;
		}

		pos.push_back(position);
		vel.push_back(velocity);
		colors.push_back(color);
	}

	// Clears all balls.
	void clear_all()
	{
		pos.clear();
		vel.clear();
		colors.clear();
	}

public:
	// Constructor, creates the ball and cube objects. Adds the first two balls to 
	// the simulation and pushes all the imgui widgets for the window.
	BouncingBallsWindow() : demoWindow(&BouncingDescriptor, info, "bouncing_balls_screenshot")
	{
		// Set initial window data.
		data.window = this;
		data.rot_free = Quaternion::Rotation({ -0.3f,1.f,0.f }, 3.14159265f / 5.f);
		enableTransparency();
		setScale(300.f);

		// Cube vertices.
		Vector3f vertices[8] =
		{
			{-1.f,-1.f,-1.f },
			{-1.f,-1.f, 1.f },
			{-1.f, 1.f,-1.f },
			{-1.f, 1.f, 1.f },
			{ 1.f,-1.f,-1.f },
			{ 1.f,-1.f, 1.f },
			{ 1.f, 1.f,-1.f },
			{ 1.f, 1.f, 1.f },
		};
		// Cube triangles.
		Vector3i triangles[12] =
		{
			{ 1, 0, 2 }, { 1, 2, 3 },
			{ 4, 5, 6 }, { 6, 5, 7 },

			{ 2, 0, 4 }, { 2, 4, 6 },
			{ 1, 3, 5 }, { 5, 3, 7 },

			{ 0, 1, 4 }, { 4, 1, 5 },
			{ 3, 2, 6 }, { 3, 6, 7 },
		};
		// Cube descriptor.
		POLYHEDRON_DESC poli_desc = {};
		poli_desc.enable_transparency = true;
		poli_desc.vertex_list = vertices;
		poli_desc.triangle_list = triangles;
		poli_desc.triangle_count = 12;
		poli_desc.default_initial_lights = false;
		// Initialize cube.
		cube.initialize(&poli_desc);

		// Simple sphere descriptor.
		SURFACE_DESC surf_desc = {};
		surf_desc.type = SURFACE_DESC::SPHERICAL_SURFACE;
		surf_desc.icosphere_depth = 4u;
		surf_desc.spherical_func = [](float, float, float) { return 1.f; };
		surf_desc.default_initial_lights = false;
		// Initialize ball.
		surf.initialize(&surf_desc);

		// Scatter descriptor for single line.
		SCATTER_DESC scatter_desc = {};
		scatter_desc.line_mesh = true;
		scatter_desc.blending = SCATTER_DESC::TRANSPARENT_POINTS;
		scatter_desc.global_color = Color(128, 128, 128, 64);
		scatter_desc.enable_updates = true;
		scatter_desc.point_count = 6u;
		Vector3f points[6] = {};
		scatter_desc.point_list = points;
		// Initialize axis
		axis.initialize(&scatter_desc);

		// Add lightsource.
		cube.updateLight(0, { 950.f,350.f }, Color::White, { 10.f, 20.f, -30.f });
		surf.updateLight(0, { 950.f,350.f }, Color::White, { 10.f, 20.f, -30.f });

		// Create first two balls.
		// --- 1 ---
		pos.push_back({ -0.5f,0.3f,-0.5f });
		vel.push_back({ 0.0003f,0.f,0.0003f });
		colors.push_back(Color::Red);
		// --- 2 ---
		pos.push_back({ 0.5f,0.3f,0.5f });
		vel.push_back({ -0.0003f,0.f,-0.0003f });
		colors.push_back(Color::Blue);

#ifdef _INCLUDE_IMGUI
		// Push imgui stuff.
		imgui.pushSlider(&radius, { 0.f,0.5f }, "Radius");
		imgui.pushSlider(&speed, { 0.f, 1.f }, "Speed");
		imgui.pushSlider(&gravity, { 0.f,5.f }, "Gravity");
		imgui.pushSlider(&loss, { 0.f,1.0f }, "Loss");
		const char* names[] = { "Add (A)", "Clear (C)" };
		imgui.pushSelector("Action", { (int)ACTION::ADD, (int)ACTION::CLEAR }, (int*)&action, names);
		imgui.initial_size = { 315,150 };
#endif
	}

	// Updates rotation on the cube, watches for keyboard and imgui actions, calls to 
	// do a simulation step and prints all the balls and the cube.
	void event_and_draw() override
	{
		// Do some event management.
		defaultEventManager(data);
		setScale(data.scale);
		cube.updateRotation(data.rot_free);
		axis.updateRotation(data.rot_free);
		surf.updateDistortion(Matrix(radius));

		// Watch for keyboard events.
		if (hasFocus())
			while (char c = Keyboard::popChar())
			{
				// If 'a' is pressed add one.
				if (c == 'a' || c == 'A')
					add_a_ball();
				// If 'c' is pressed clear all.
				else if (c == 'c' || c == 'C')
					clear_all();
			}

		// Check for imgui updates.
		if (action == ACTION::ADD)
		{
			add_a_ball();
			action = ACTION::NONE;
		}
		else if (action == ACTION::CLEAR)
		{
			clear_all();
			action = ACTION::NONE;
		}

		// Do a step.
		do_frame_step();

		// Clear buffers and set target.
		setRenderTarget();
		clearBuffer();
		// Draw balls (opaque) first.
		for (unsigned ball = 0; ball < pos.size(); ball++)
		{
			surf.updateGlobalColor(colors[ball]);
			surf.updatePosition(pos[ball]);
			surf.Draw();
		}
		// Draw axis for each ball.
		for (unsigned ball = 0; ball < pos.size(); ball++)
		{
			Vector3f relative_pos = (data.rot_free.inv() * Quaternion(pos[ball]) * data.rot_free).getVector();
			Vector3f points[6] =
			{
				{ -1.f, relative_pos.y, relative_pos.z },
				{  1.f, relative_pos.y, relative_pos.z },
				{ relative_pos.x, -1.f, relative_pos.z },
				{ relative_pos.x,  1.f, relative_pos.z },
				{ relative_pos.x, relative_pos.y, -1.f },
				{ relative_pos.x, relative_pos.y,  1.f },
			};
			axis.updatePoints(points);
			axis.Draw();
		}
		// Draw cubes.
		cube.updateGlobalColor(Color(255, 255, 255, 24));
		cube.updateDistortion(Matrix::Identity());
		cube.Draw();
		cube.updateGlobalColor(Color(255, 255, 255, 48));
		cube.updateDistortion(Matrix(1.05f));
		cube.Draw();
		// Push.
		pushFrame();
		return;
	}
};

// Waves are always cool, and if you are treating with a cool wave function you always want to 
// plot it. This window is a great example of that. This window also introduces dynamic backgrounds. 
// Due to the library being self sufficient unfortunately it does not contain a real image, so it
// generates a fake night sky. But if you want the real immersion, go online, download an equirectangular
// projection, transform it into an uncompressed bitmap, place it on the executable path with the 
// name "equirect.bmp" and the demo will use that image instead of generating one.
class OceanPlanetWindow : public demoWindow
{
private:
	static inline const char* info =
		"\n"
		"  Despite this library not being intended as a game engine, its flexibility allows for \n"
		"  surprisingly good-looking plots with a lot of detail. This window intends to serve as \n"
		"  a showcase of those capabilities.\n"
		"\n"
		"  It contains two distinct objects: a wave planet ball in the middle, which you can \n"
		"  fully customize by tweaking its parameters and choosing your favourite color palette, \n"
		"  and a dynamic background that adds to the immersion, making it feel like that planet \n"
		"  is actually somewhere.\n"
		"\n"
		"  Since this library is self-contained inside the '.lib' file, I decided against using \n"
		"  real images as textures in the demo, as that would require additional resources. As \n"
		"  a bonus, however, and to further showcase the immersion capabilities of the library, \n"
		"  you can provide your own equirectangular image. If you download one and place it in \n"
		"  the executable path, it will be used to generate the background instead of the night \n"
		"  sky. The image must be an uncompressed bitmap (.bmp) named 'equirect.bmp'.\n"
		"\n"
		"  How it works:\n"
		"  The wave planet is defined as a spherical surface. The function that determines its \n"
		"  radius embeds the coordinates in a different basis to create the swirling currents, \n"
		"  and then computes the wave height using several sine waves with irrational frequencies \n"
		"  to avoid repetition. For coloring, it performs a linear interpolation between four \n"
		"  colors based on the resulting wave height. Time advances every frame and the surface \n"
		"  is updated accordingly.\n"
		"\n"
		"  For the background, the library provides an option to make it dynamic. In this mode, \n"
		"  it expects a cube projection of a sphere as input and projects the sphere points onto \n"
		"  the screen as if viewed through a window. In the synthetic night sky case, the cube \n"
		"  projection is generated directly. If an image is provided, a quality-of-life function \n"
		"  of the library converts the equirectangular image into cube projections. This function \n"
		"  is 'ToCube::from_equirect()', found in 'Image.h'.\n"
		"\n"
		"  Both drawables use the same quaternion for rotation, obtained via the default manager. \n"
		"  The color selector is used directly from the 'defaultImGui' class."
		"\n ";


	// Static window descriptor used by all instances of the class.
	static inline const WINDOW_DESC OceanDescriptor =
	{
		"Ocean Planet Window",
		WINDOW_DESC::WINDOW_MODE_NORMAL,
		{ 1080, 720 },
	};

	EventData data = {}; // Stores the event data for the window.
	Surface surf;		 // Drawable to plot the wave sphere.
	Background sky;		 // Drawable to plot the dynamic background.

private:
	// Enumeration of the colors used for blending.
	enum class COLORING : int
	{
		NONE,
		DEEP,
		MID,
		SHALLOW,
		FOAM
	} edit_color = COLORING::NONE;

	// Irrational frequency ratios for quasi-periodic (chaotic-looking) waves
	static constexpr float phi   = 1.618034f;
	static constexpr float sqrt2 = 1.414213f;
	static constexpr float sqrt3 = 1.732051f;

	// Struct of variables needed by the function.
	struct WAVE_DESC
	{
		float t         = 0.f;	// Time value
		float amp       = 0.1f;	// Controls the wave's amplitude
		float scale     = 1.6f;	// Controls the frequency of the waves
		float chop      = 0.8f;	// Controls the wave's steepness
		float swirl     = 1.5f; // Controls the swirl
		Color deep      = Color(  0,  25,  72);	// Deep water color.
		Color mid       = Color( 25,  55, 144);	// Mid water color.
		Color shallow   = Color( 85, 145, 192);	// Shallow water color.
		Color foam      = Color(210, 210, 255);	// Foam water color.
	};

	static inline WAVE_DESC desc; // Global variables for the static function.
	static inline Color next;	  // Next color for value -> color concatenation.

	WAVE_DESC my_desc = {};	// Class variables for storage and modification.
	float speed = 1.f;		// Controls time increase rate.
	float fov = 1.f;		// Controls the background FOV.

	// Helper function for the wave function, creates the wave patterns.
	static inline float trochoid(float x, float y, float z, float freq, float speed, float sharpness)
	{
		const float wave =
			sinf(x * freq         + desc.t * speed) +
			sinf(y * freq * phi   + desc.t * speed * 0.9f) +
			sinf(z * freq * sqrt2 + desc.t * speed * 1.1f);
		return (wave > 0.f) ? powf(fabsf(wave / 3.f), sharpness) : -powf(fabsf(wave / 3.f), sharpness);
	}

	// Wave function used. It takes in a point on the unit sphere, warps it and creates the 
	// waves based on the warped coordinates. Next color is coputed by interpolation of the height.
	static float wave_func(float x, float y, float z)
	{
		// Domain warping in 3D - creates swirling currents
		const float wx = x + desc.swirl * sinf(y * phi * 2.f   + z * sqrt2 + desc.t * 0.20f);
		const float wy = y + desc.swirl * sinf(z * sqrt3 * 2.f + x * phi   + desc.t * 0.17f);
		const float wz = z + desc.swirl * sinf(x * sqrt2 * 2.f + y * sqrt3 + desc.t * 0.23f);

		// Accumulate wave layers with golden ratio frequency scaling
		// Layer 1: Large ocean swells
		float h = trochoid(wx, wy, wz, desc.scale, 0.5f, desc.chop) * 1.0f +
			// Layer 2: Medium waves
			trochoid(wy + wx, wz - wy, wx + wz, desc.scale * phi, 0.7f, desc.chop * 1.1f) * 0.5f +
			// Layer 3: Smaller chop
			trochoid(wx - wz, wy + wx, wz - wy, desc.scale * phi * phi, 1.0f, desc.chop * 0.9f) * 0.25f +
			// Layer 4: Fine detail
			trochoid(wz, wx, wy, desc.scale * phi * phi * phi, 1.5f, 1.0f) * 0.12f +
			// Layer 5: Micro texture
			sinf((wx * 7.f + wy * 11.f + wz * 13.f) * desc.scale + desc.t * 2.f) * 0.06f;

		// Normalize height for coloring.
		float ch = (h > 2.f) ? 1.f : (h < -2.f) ? 0.f : (h + 2.f) / 4.f;

		// Assign next color based on height.
		next =
			(ch < 0.35f) ? desc.deep * (1.f - ch / 0.35f) + desc.mid * (ch / 0.35f) :
			(ch < 0.65f) ? desc.mid * (1.f - (ch - 0.35f) / 0.3f) + desc.shallow * ((ch - 0.35f) / 0.3f) :
			desc.shallow * (1.f - (ch - 0.65f) / 0.35f) + desc.foam * ((ch - 0.65f) / 0.35f);

		// Return height displaced
		return 1.5f + h * desc.amp;
	}

	// This function generates the night sky background.
	static inline void generateSky(Image& image)
	{
		if (image.load("equirect.bmp"))
		{
			Image* cube = ToCube::from_equirect(image, 1000);
			image = *cube;
			delete cube;
			return;
		}

		unsigned w = 1000;
		image.reset(w, 6 * w); // +X, -X, +Y, -Y, +Z, -Z

		auto R3toCoord = [w](Vector3f dir)
		{
			bool sign_x = (dir.x > 0.f);
			bool sign_y = (dir.y > 0.f);
			bool sign_z = (dir.z > 0.f);
			float abs_x = sign_x ? dir.x : -dir.x;
			float abs_y = sign_y ? dir.y : -dir.y;
			float abs_z = sign_z ? dir.z : -dir.z;

			unsigned face, row, col;
			if (abs_x >= abs_y && abs_x >= abs_z)
			{
				face = sign_x ? 0 : 1;
				row = sign_x ? unsigned(w * (face + (-dir.y / dir.x + 1.f) * 0.5f)) : unsigned(w * (face + (dir.y / dir.x + 1.f) * 0.5f));
				col = sign_x ? unsigned(w * ((-dir.z / dir.x + 1.f) * 0.5f)) : unsigned(w * ((-dir.z / dir.x + 1.f) * 0.5f));
			}
			else if (abs_y >= abs_x && abs_y >= abs_z)
			{
				face = sign_y ? 2 : 3;
				row = sign_y ? unsigned(w * (face + (dir.z / dir.y + 1.f) * 0.5f)) : unsigned(w * (face + (dir.z / dir.y + 1.f) * 0.5f));
				col = sign_y ? unsigned(w * ((dir.x / dir.y + 1.f) * 0.5f)) : unsigned(w * ((-dir.x / dir.y + 1.f) * 0.5f));
			}
			else
			{
				face = sign_z ? 4 : 5;
				row = sign_z ? unsigned(w * (face + (-dir.y / dir.z + 1.f) * 0.5f)) : unsigned(w * (face + (dir.y / dir.z + 1.f) * 0.5f));
				col = sign_z ? unsigned(w * ((dir.x / dir.z + 1.f) * 0.5f)) : unsigned(w * ((dir.x / dir.z + 1.f) * 0.5f));
			}
			return Vector2i(row, col);
		};

		for (unsigned i = 0; i < 1000000; i++)
		{
			Vector3f dir;
			float abs2 = 0.f;
			while (abs2 > 1.f || abs2 < 0.02f)
			{
				dir = Vector3f(rand() / 16384.f - 1.f, rand() / 16384.f - 1.f, rand() / 16384.f - 1.f);
				abs2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
			}
			float intensity = 0.02f / abs2;

			Vector2i coor = R3toCoord(dir);
			image(coor.x, coor.y) = Color::White * intensity;
		}

		const Vector3f galaxy_axis = Vector3f(-1.f, 1.f, -0.2f).normal();

		for (unsigned i = 0; i < 1000000; i++)
		{
			Vector3f dir;
			float abs2 = 0.f;
			while (abs2 > 1.f || abs2 < 0.04f)
			{
				dir = Vector3f(rand() / 16384.f - 1.f, rand() / 16384.f - 1.f, rand() / 16384.f - 1.f);
				abs2 = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
			}
			float intensity = 0.04f / abs2;

			float squeeze = dir ^ galaxy_axis;
			Vector3f diff = (squeeze > 0.f) ? (squeeze - squeeze * squeeze) * galaxy_axis : (squeeze + squeeze * squeeze) * galaxy_axis;
			dir -= diff;

			Vector2i coor = R3toCoord(dir);
			image(coor.x, coor.y) = Color(192 + unsigned char(rand() / 512.f), 192 + unsigned char(rand() / 512.f), 192 + unsigned char(rand() / 512.f)) * intensity;
		}
	}

public:
	// Constructor, initializes the surface, generates the background and pushes the imgui widgets.
	OceanPlanetWindow() : demoWindow(&OceanDescriptor, info, "ocean_planet_screenshot")
	{
		// Set yourself in data.
		data.window = this;
		
		// Create the surface descriptor
		SURFACE_DESC surf_desc = {};
		surf_desc.enable_updates = true;
		surf_desc.enable_illuminated = false;
		surf_desc.type = SURFACE_DESC::SPHERICAL_SURFACE;
		surf_desc.coloring = SURFACE_DESC::OUTPUT_FUNCTION_COLORING;
		surf_desc.spherical_func = wave_func;
		surf_desc.output_color_func = [](float, float, float) { return next; };

		// Initialize the surface
		surf.initialize(&surf_desc);

		// Create the background image.
		Image image;
		generateSky(image);
		
		// Create the background descriptor.
		BACKGROUND_DESC back_desc = {};
		back_desc.override_buffers = true;
		back_desc.type = BACKGROUND_DESC::DYNAMIC_BACKGROUND;
		back_desc.image = &image;

		// Initialize the background.
		sky.initialize(&back_desc);

#ifdef _INCLUDE_IMGUI
		// Push imGui values, sliders and selectors.
		imgui.initial_size = { 315,190 };
		imgui.pushSlider(  &my_desc.amp, { 0.f,1.f }, "Amplitude");
		imgui.pushSlider(&my_desc.scale, { 0.f,4.f }, "Scale");
		imgui.pushSlider( &my_desc.chop, { 0.f,2.f }, "Chop");
		imgui.pushSlider(&my_desc.swirl, { 0.f,2.f }, "Swirl");
		imgui.pushSlider(        &speed, { 0.f,5.f }, "Speed");
		imgui.pushSlider( 		   &fov, {0.1f,4.f }, "FOV");
		
		const char* names[] = { "Deep", "Mid", "Shallow", "Foam" };
		imgui.pushSelector("Color Editor", { (int)COLORING::DEEP, (int)COLORING::FOAM }, (int*)&edit_color, names);
#endif
	}

	// Checks for color editing, runs tima and updates the surface. 
	// Runs the event manager and pushes the next frame.
	void event_and_draw() override
	{
		// Look for color editing events
#ifdef _INCLUDE_IMGUI
		if (edit_color != COLORING::NONE)
		{
			imgui.popColor();
			switch (edit_color)
			{
			case COLORING::DEEP:
				imgui.editColor(&my_desc.deep);
				break;
			case COLORING::MID:
				imgui.editColor(&my_desc.mid);
				break;
			case COLORING::SHALLOW:
				imgui.editColor(&my_desc.shallow);
				break;
			case COLORING::FOAM:
				imgui.editColor(&my_desc.foam);
				break;
			default:
				break;
			}
			edit_color = COLORING::NONE;
		}
#endif

		// Set your descriptor and update
		my_desc.t += 0.05f * speed;
		desc = my_desc;
		surf.updateShape();

		// Do some event management.
		defaultEventManager(data);
		graphics().setScale(data.scale);
		surf.updateRotation(data.rot_free);
		sky.updateRotation(data.rot_free);
		sky.updateFieldOfView({ fov,fov });

		// Set target.
		graphics().setRenderTarget();
		// Draw.
		sky.Draw();
		surf.Draw();
		// Push.
		graphics().pushFrame();
		return;
	}
};

// Who doesn't want to have a Rubik's cube app? Then with this library you can make it real. As 
// the name advertises this is just a fully functional rubiks cube, all moves are defined including
// middle layer moves and full cube rotations, and you can solve it as a regular cube.
// My best time on an official scrambble is 1:22.88, can you beat it? :)
class RubiksWindow : public demoWindow
{
private:
	static inline const char* info =
		"\n"
		"  It is no secret that I am a big fan of Rubik's Cubes, and there was no way I was not \n"
		"  going to add this to the demo. Who doesn't want to have a Rubik's Cube app? With this \n"
		"  library, you can make it real!\n"
		"\n"
		"  As the name suggests, this is just a fully functional Rubik's Cube. You can move it \n"
		"  around with the mouse and it will always return to a default viewing position, which \n"
		"  can be modified via the sliders. You can also adjust the turn speed using the TPS \n"
		"  (turns per second) slider.\n"
		"\n"
		"  Turning it is quite simple, just use standard notation. The notation is as follows:\n"
		"\n"
		"  * Outer layer moves: R (right), L (left), U (up), D (down), F (front), B (back).\n"
		"  * Middle layer moves: M (middle like L), S (middle like F), E (middle like D).\n"
		"  * Rotations: X (rotates R direction), Y (rotates U direction), Z (rotates F direction).\n"
		"\n"
		"  To perform clockwise moves, press the corresponding key. To perform the reversed \n"
		"  moves, hold Shift while pressing the key.\n"
		"\n"
		"  My personal best on this cube is 1:22.88 on an official scramble. Can you beat it? :)\n"
		"\n"
		"  How it works:\n"
		"  To build the cube, I first create all the pieces as 'Polyhedrons'. All of them share \n"
		"  the same cube vertices, but each piece is displaced into its correct position so \n"
		"  that rotations are easy to apply without weird behavior.\n"
		"\n"
		"  Colors are assigned based on the starting position of each piece, ensuring the \n"
		"  correct color layout at the beginning. The smooth appearance comes from custom \n"
		"  normal vectors: instead of letting 'Polyhedron' compute normals from triangle \n"
		"  orientation, slightly offset normals are provided per vertex, producing a rounded \n"
		"  effect.\n"
		"\n"
		"  So how do we turn it? By simply updating the rotation of the pieces. Every move is \n"
		"  characterized by a rotation axis and a permutation of pieces. All moves are defined \n"
		"  this way in the function 'play_move()', and we keep track of the piece positions \n"
		"  after each move.\n"
		"\n"
		"  To keep the motion smooth, rotations are applied incrementally rather than all at \n"
		"  once. This works cleanly thanks to how the pieces are defined. Interestingly, no \n"
		"  checks are ever performed on the final quaternion state, so after enough turns, \n"
		"  floating-point approximation errors may cause pieces to appear slightly displaced."
		"\n ";


	// Static window descriptor used by all instances of the class.
	static inline const WINDOW_DESC RubiksDescriptor =
	{
		"Rubik's Cube Window",
		WINDOW_DESC::WINDOW_MODE_NORMAL,
		{ 1080, 720 },
	};

	Polyhedron edge_pieces[12];  // Stores the edge pieces of the cube.
	Polyhedron corner_pieces[8]; // Stores the corner pieces of the cube.
	Polyhedron center_pieces[6]; // Stores the center pieces of the cube.
	EventData data = {};		 // Stores the event data for the window.

	float side_view = -0.2f;     // Horizintal cube position.
	float vert_view =  0.2f;	 // Vertical cube position.
	float pitch = -0.0f;		 // View pitch.

private:
	float tps = 5.f;				  // Speed of the cube moves.
	bool moving = false;			  // Whether a move is being performed.
	vector<unsigned> moving_edges;    // What edges are being moved.
	vector<unsigned> moving_corners;  // What corners are being moved.
	vector<unsigned> moving_centers;  // What centers are being moved.
	float accum_rotation = 0.f;		  // Accumulated rotation on the current move.
	Vector3f rotation_axis = {};	  // Axis of rotation.

	vector<char> move_queue;  // Stores all characters on the queue.

	Quaternion edge_quat[12] = { 1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f }; // Stores the edge quaternions.
	Quaternion corner_quat[8] = { 1.f,1.f,1.f,1.f,1.f,1.f,1.f,1.f };				// Stores the corner quaternions.
	Quaternion center_quat[6] = { 1.f,1.f,1.f,1.f,1.f,1.f };						// Stores the center quaternions.

	unsigned edge_slots[12] = { 0,1,2,3,4,5,6,7,8,9,10,11 }; // Stores where the edges are in the cube.
	unsigned corner_slots[8] = { 0,1,2,3,4,5,6,7 };			 // Stores where the corners are in the cube.
	unsigned center_slots[6] = { 0,1,2,3,4,5 };				 // Stores where the centers are in the cube.

	// Enumerator for all moves available on the cube.
	enum class MOVES
	{
		R, U, F, D, L, B, 
		Rp, Up, Fp, Dp, Lp, Bp, 
		S, M, E, 
		Sp, Mp, Ep,
		X, Y, Z,
		Xp, Yp, Zp,
	};

	// Generates all the polyhedrons of the cube and assigns the colors based 
	// on each piece position. Normals are also defined to give this round look.
	void generateCube()
	{
		const float len = 0.30f;
		const float dist = 2 * len + 0.02f;

		Vector3f vertices[8] = {};
		Color colors[36] = {};

		Vector3f global_vertices[8] =
		{
			{-len,-len,-len },
			{-len,-len, len },
			{-len, len,-len },
			{-len, len, len },
			{ len,-len,-len },
			{ len,-len, len },
			{ len, len,-len },
			{ len, len, len },
		};

		Vector3i triangles[12] =
		{
			{ 1, 0, 2 }, { 1, 2, 3 },
			{ 4, 5, 6 }, { 6, 5, 7 },

			{ 2, 0, 4 }, { 2, 4, 6 },
			{ 1, 3, 5 }, { 5, 3, 7 },

			{ 0, 1, 4 }, { 4, 1, 5 },
			{ 3, 2, 6 }, { 3, 6, 7 },
		};

		float distortion = 1.5f;
		Vector3f normals[36] =
		{
			Vector3f(-1.f, 0.f, 0.f) + distortion * global_vertices[triangles[0].x],
			Vector3f(-1.f, 0.f, 0.f) + distortion * global_vertices[triangles[0].y],
			Vector3f(-1.f, 0.f, 0.f) + distortion * global_vertices[triangles[0].z],
			Vector3f(-1.f, 0.f, 0.f) + distortion * global_vertices[triangles[1].x],
			Vector3f(-1.f, 0.f, 0.f) + distortion * global_vertices[triangles[1].y],
			Vector3f(-1.f, 0.f, 0.f) + distortion * global_vertices[triangles[1].z],
			Vector3f( 1.f, 0.f, 0.f) + distortion * global_vertices[triangles[2].x],
			Vector3f( 1.f, 0.f, 0.f) + distortion * global_vertices[triangles[2].y],
			Vector3f( 1.f, 0.f, 0.f) + distortion * global_vertices[triangles[2].z],
			Vector3f( 1.f, 0.f, 0.f) + distortion * global_vertices[triangles[3].x],
			Vector3f( 1.f, 0.f, 0.f) + distortion * global_vertices[triangles[3].y],
			Vector3f( 1.f, 0.f, 0.f) + distortion * global_vertices[triangles[3].z],
			Vector3f( 0.f, 0.f,-1.f) + distortion * global_vertices[triangles[4].x],
			Vector3f( 0.f, 0.f,-1.f) + distortion * global_vertices[triangles[4].y],
			Vector3f( 0.f, 0.f,-1.f) + distortion * global_vertices[triangles[4].z],
			Vector3f( 0.f, 0.f,-1.f) + distortion * global_vertices[triangles[5].x],
			Vector3f( 0.f, 0.f,-1.f) + distortion * global_vertices[triangles[5].y],
			Vector3f( 0.f, 0.f,-1.f) + distortion * global_vertices[triangles[5].z],
			Vector3f( 0.f, 0.f, 1.f) + distortion * global_vertices[triangles[6].x],
			Vector3f( 0.f, 0.f, 1.f) + distortion * global_vertices[triangles[6].y],
			Vector3f( 0.f, 0.f, 1.f) + distortion * global_vertices[triangles[6].z],
			Vector3f( 0.f, 0.f, 1.f) + distortion * global_vertices[triangles[7].x],
			Vector3f( 0.f, 0.f, 1.f) + distortion * global_vertices[triangles[7].y],
			Vector3f( 0.f, 0.f, 1.f) + distortion * global_vertices[triangles[7].z],
			Vector3f( 0.f,-1.f, 0.f) + distortion * global_vertices[triangles[8].x],
			Vector3f( 0.f,-1.f, 0.f) + distortion * global_vertices[triangles[8].y],
			Vector3f( 0.f,-1.f, 0.f) + distortion * global_vertices[triangles[8].z],
			Vector3f( 0.f,-1.f, 0.f) + distortion * global_vertices[triangles[9].x],
			Vector3f( 0.f,-1.f, 0.f) + distortion * global_vertices[triangles[9].y],
			Vector3f( 0.f,-1.f, 0.f) + distortion * global_vertices[triangles[9].z],
			Vector3f( 0.f, 1.f, 0.f) + distortion * global_vertices[triangles[10].x],
			Vector3f( 0.f, 1.f, 0.f) + distortion * global_vertices[triangles[10].y],
			Vector3f( 0.f, 1.f, 0.f) + distortion * global_vertices[triangles[10].z],
			Vector3f( 0.f, 1.f, 0.f) + distortion * global_vertices[triangles[11].x],
			Vector3f( 0.f, 1.f, 0.f) + distortion * global_vertices[triangles[11].y],
			Vector3f( 0.f, 1.f, 0.f) + distortion * global_vertices[triangles[11].z],

		};

		Vector3f edge_positions[12] =
		{
			{ 0.0f, dist, dist },
			{ 0.0f, dist,-dist },
			{ 0.0f,-dist, dist },
			{ 0.0f,-dist,-dist },
			{ dist, 0.0f, dist },
			{ dist, 0.0f,-dist },
			{-dist, 0.0f, dist },
			{-dist, 0.0f,-dist },
			{ dist, dist, 0.0f },
			{ dist,-dist, 0.0f },
			{-dist, dist, 0.0f },
			{-dist,-dist, 0.0f },
		};

		Vector3f corner_positions[8] =
		{
			{ dist, dist, dist },
			{ dist, dist,-dist },
			{ dist,-dist, dist },
			{ dist,-dist,-dist },
			{-dist, dist, dist },
			{-dist, dist,-dist },
			{-dist,-dist, dist },
			{-dist,-dist,-dist },
		};

		Vector3f center_positions[6] =
		{
			{ 0.0f, 0.0f, dist },
			{ 0.0f, 0.0f,-dist },
			{ 0.0f, dist, 0.0f },
			{ 0.0f,-dist, 0.0f },
			{ dist, 0.0f, 0.0f },
			{-dist, 0.0f, 0.0f },
		};

		POLYHEDRON_DESC desc = {};
		desc.default_initial_lights = false;
		desc.coloring = POLYHEDRON_DESC::PER_VERTEX_COLORING;
		desc.normal_computation = POLYHEDRON_DESC::PER_TRIANGLE_LIST_NORMALS;
		desc.triangle_count = 12;
		desc.triangle_list = triangles;
		desc.vertex_list = vertices;
		desc.color_list = colors;
		desc.normal_vectors_list = normals;

		Color dark_gray = Color(100, 100, 100);
		auto set_colors = [&](Vector3f pos)
			{
				if (pos.x < 0.f) for (unsigned j = 0u; j < 6u; j++) colors[j] = Color::Orange;
				else for (unsigned j = 0u; j < 6u; j++) colors[j] = dark_gray;

				if (pos.x > 0.f) for (unsigned j = 6u; j < 12u; j++) colors[j] = Color::Red;
				else for (unsigned j = 6u; j < 12u; j++) colors[j] = dark_gray;

				if (pos.z < 0.f) for (unsigned j = 12u; j < 18u; j++) colors[j] = Color::Green;
				else for (unsigned j = 12u; j < 18u; j++) colors[j] = dark_gray;

				if (pos.z > 0.f) for (unsigned j = 18u; j < 24u; j++) colors[j] = Color::Blue;
				else for (unsigned j = 18u; j < 24u; j++) colors[j] = dark_gray;

				if (pos.y < 0.f) for (unsigned j = 24u; j < 30u; j++) colors[j] = Color::Yellow;
				else for (unsigned j = 24u; j < 30u; j++) colors[j] = dark_gray;

				if (pos.y > 0.f) for (unsigned j = 30u; j < 36u; j++) colors[j] = Color::White;
				else for (unsigned j = 30u; j < 36u; j++) colors[j] = dark_gray;
			};

		for (unsigned i = 0; i < 12; i++)
		{
			Vector3f pos = edge_positions[i];
			for (unsigned v = 0; v < 8; v++)
				vertices[v] = global_vertices[v] + pos;

			set_colors(pos);

			edge_pieces[i].initialize(&desc);
			edge_pieces[i].updateLight(0, { 800.f,400.f }, Color::White, { 10.f, 20.f, -30.f });
		}
		for (unsigned i = 0; i < 8; i++)
		{
			Vector3f pos = corner_positions[i];
			for (unsigned v = 0; v < 8; v++)
				vertices[v] = global_vertices[v] + pos;

			set_colors(pos);

			corner_pieces[i].initialize(&desc);
			corner_pieces[i].updateLight(0, { 800.f,400.f }, Color::White, { 10.f, 20.f, -30.f });
		}
		for (unsigned i = 0; i < 6; i++)
		{
			Vector3f pos = center_positions[i];
			for (unsigned v = 0; v < 8; v++)
				vertices[v] = global_vertices[v] + pos;

			set_colors(pos);

			center_pieces[i].initialize(&desc);
			center_pieces[i].updateLight(0, { 800.f,400.f }, Color::White, { 10.f, 20.f, -30.f });
		}
	}

	// Helper functions for piece assignement and permutation.
	inline void assign_and_permute_edges(unsigned idx0, unsigned idx1, unsigned idx2, unsigned idx3)
	{
		moving_edges = { edge_slots[idx0], edge_slots[idx1], edge_slots[idx2], edge_slots[idx3] };

		unsigned temp = edge_slots[idx3];
		edge_slots[idx3] = edge_slots[idx2];
		edge_slots[idx2] = edge_slots[idx1];
		edge_slots[idx1] = edge_slots[idx0];
		edge_slots[idx0] = temp;
	}
	inline void assign_and_permute_corners(unsigned idx0, unsigned idx1, unsigned idx2, unsigned idx3)
	{
		moving_corners = { corner_slots[idx0], corner_slots[idx1], corner_slots[idx2], corner_slots[idx3] };

		unsigned temp = corner_slots[idx3];
		corner_slots[idx3] = corner_slots[idx2];
		corner_slots[idx2] = corner_slots[idx1];
		corner_slots[idx1] = corner_slots[idx0];
		corner_slots[idx0] = temp;
	}
	inline void assign_and_permute_centers(unsigned idx0, unsigned idx1, unsigned idx2, unsigned idx3)
	{
		moving_centers = { center_slots[idx0], center_slots[idx1], center_slots[idx2], center_slots[idx3] };

		unsigned temp = center_slots[idx3];
		center_slots[idx3] = center_slots[idx2];
		center_slots[idx2] = center_slots[idx1];
		center_slots[idx1] = center_slots[idx0];
		center_slots[idx0] = temp;
	}

	// Given a move decides which pieces are to be permuted and rotated and in what axis.
	void play_move(MOVES move)
	{
		switch (move)
		{
		case MOVES::R:
			rotation_axis = { 1.f,0.f,0.f };
			assign_and_permute_edges(9, 5, 8, 4);
			assign_and_permute_corners(0, 2, 3, 1);
			moving_centers = { center_slots[4] };
			break;

		case MOVES::Rp:
			rotation_axis = {-1.f,0.f,0.f };
			assign_and_permute_edges(4, 8, 5, 9);
			assign_and_permute_corners(1, 3, 2, 0);
			moving_centers = { center_slots[4] };
			break;

		case MOVES::U:
			rotation_axis = { 0.f,1.f,0.f };
			assign_and_permute_edges(0, 8, 1, 10);
			assign_and_permute_corners(0, 1, 5, 4);
			moving_centers = { center_slots[2] };
			break;

		case MOVES::Up:
			rotation_axis = { 0.f,-1.f,0.f };
			assign_and_permute_edges(10, 1, 8, 0);
			assign_and_permute_corners(4, 5, 1, 0);
			moving_centers = { center_slots[2] };
			break;

		case MOVES::F:
			rotation_axis = { 0.f,0.f,-1.f };
			assign_and_permute_edges(1, 5, 3, 7);
			assign_and_permute_corners(1, 3, 7, 5);
			moving_centers = { center_slots[1] };
			break;

		case MOVES::Fp:
			rotation_axis = { 0.f,0.f,1.f };
			assign_and_permute_edges(7, 3, 5, 1);
			assign_and_permute_corners(5, 7, 3, 1);
			moving_centers = { center_slots[1] };
			break;

		case MOVES::L:
			rotation_axis = { -1.f,0.f,0.f };
			assign_and_permute_edges(6, 10, 7, 11);
			assign_and_permute_corners(4, 5, 7, 6);
			moving_centers = { center_slots[5] };
			break;

		case MOVES::Lp:
			rotation_axis = { 1.f,0.f,0.f };
			assign_and_permute_edges(11, 7, 10, 6);
			assign_and_permute_corners(6, 7, 5, 4);
			moving_centers = { center_slots[5] };
			break;

		case MOVES::D:
			rotation_axis = { 0.f,-1.f,0.f };
			assign_and_permute_edges(2, 11, 3, 9);
			assign_and_permute_corners(2, 6, 7, 3);
			moving_centers = { center_slots[3] };
			break;

		case MOVES::Dp:
			rotation_axis = { 0.f,1.f,0.f };
			assign_and_permute_edges(9, 3, 11, 2);
			assign_and_permute_corners(3, 7, 6, 2);
			moving_centers = { center_slots[3] };
			break;

		case MOVES::B:
			rotation_axis = { 0.f,0.f,1.f };
			assign_and_permute_edges(0, 6, 2, 4);
			assign_and_permute_corners(0, 4, 6, 2);
			moving_centers = { center_slots[0] };
			break;

		case MOVES::Bp:
			rotation_axis = { 0.f,0.f,-1.f };
			assign_and_permute_edges(4, 2, 6, 0);
			assign_and_permute_corners(2, 6, 4, 0);
			moving_centers = { center_slots[0] };
			break;

		case MOVES::M:
			rotation_axis = { -1.f,0.f,0.f };
			assign_and_permute_edges(1, 3, 2, 0);
			assign_and_permute_centers(2, 1, 3, 0);
			moving_corners = {};
			break;

		case MOVES::Mp:
			rotation_axis = { 1.f,0.f,0.f };
			assign_and_permute_edges(0, 2, 3, 1);
			assign_and_permute_centers(0, 3, 1, 2);
			moving_corners = {};
			break;

		case MOVES::S:
			rotation_axis = { 0.f,0.f,-1.f };
			assign_and_permute_edges(8, 9, 11, 10);
			assign_and_permute_centers(2, 4, 3, 5);
			moving_corners = {};
			break;

		case MOVES::Sp:
			rotation_axis = { 0.f,0.f,1.f };
			assign_and_permute_edges(10, 11, 9, 8);
			assign_and_permute_centers(5, 3, 4, 2);
			moving_corners = {};
			break;

		case MOVES::E:
			rotation_axis = { 0.f,-1.f,0.f };
			assign_and_permute_edges(4, 6, 7, 5);
			assign_and_permute_centers(0, 5, 1, 4);
			moving_corners = {};
			break;

		case MOVES::Ep:
			rotation_axis = { 0.f,1.f,0.f };
			assign_and_permute_edges(5, 7, 6, 4);
			assign_and_permute_centers(4, 1, 5, 0);
			moving_corners = {};
			break;

		case MOVES::X:
			rotation_axis = { 1.f,0.f,0.f };
			// R
			assign_and_permute_edges(9, 5, 8, 4);
			assign_and_permute_corners(0, 2, 3, 1);
			// M'
			assign_and_permute_edges(0, 2, 3, 1);
			assign_and_permute_centers(0, 3, 1, 2);
			// L'
			assign_and_permute_edges(11, 7, 10, 6);
			assign_and_permute_corners(6, 7, 5, 4);
			// All pieces move
			moving_edges = { 0,1,2,3,4,5,6,7,8,9,10,11 };
			moving_corners = { 0,1,2,3,4,5,6,7 };
			moving_centers = { 0,1,2,3,4,5 };
			break;

		case MOVES::Xp:
			rotation_axis = { -1.f,0.f,0.f };
			// R'
			assign_and_permute_edges(4, 8, 5, 9);
			assign_and_permute_corners(1, 3, 2, 0);
			// M
			assign_and_permute_edges(1, 3, 2, 0);
			assign_and_permute_centers(2, 1, 3, 0);
			// L
			assign_and_permute_edges(6, 10, 7, 11);
			assign_and_permute_corners(4, 5, 7, 6);
			// All pieces move
			moving_edges = { 0,1,2,3,4,5,6,7,8,9,10,11 };
			moving_corners = { 0,1,2,3,4,5,6,7 };
			moving_centers = { 0,1,2,3,4,5 };
			break;

		case MOVES::Y:
			rotation_axis = { 0.f,1.f,0.f };
			// U
			assign_and_permute_edges(0, 8, 1, 10);
			assign_and_permute_corners(0, 1, 5, 4);
			// E'
			assign_and_permute_edges(5, 7, 6, 4);
			assign_and_permute_centers(4, 1, 5, 0);
			// D'
			assign_and_permute_edges(9, 3, 11, 2);
			assign_and_permute_corners(3, 7, 6, 2);
			// All pieces move
			moving_edges = { 0,1,2,3,4,5,6,7,8,9,10,11 };
			moving_corners = { 0,1,2,3,4,5,6,7 };
			moving_centers = { 0,1,2,3,4,5 };
			break;

		case MOVES::Yp:
			rotation_axis = { 0.f,-1.f,0.f };
			// U'
			assign_and_permute_edges(10, 1, 8, 0);
			assign_and_permute_corners(4, 5, 1, 0);
			// E
			assign_and_permute_edges(4, 6, 7, 5);
			assign_and_permute_centers(0, 5, 1, 4);
			// D
			assign_and_permute_edges(2, 11, 3, 9);
			assign_and_permute_corners(2, 6, 7, 3);
			// All pieces move
			moving_edges = { 0,1,2,3,4,5,6,7,8,9,10,11 };
			moving_corners = { 0,1,2,3,4,5,6,7 };
			moving_centers = { 0,1,2,3,4,5 };
			break;

		case MOVES::Z:
			rotation_axis = { 0.f,0.f,-1.f };
			// F
			assign_and_permute_edges(1, 5, 3, 7);
			assign_and_permute_corners(1, 3, 7, 5);
			// S
			assign_and_permute_edges(8, 9, 11, 10);
			assign_and_permute_centers(2, 4, 3, 5);
			// B'
			assign_and_permute_edges(4, 2, 6, 0);
			assign_and_permute_corners(2, 6, 4, 0);
			// All pieces move
			moving_edges = { 0,1,2,3,4,5,6,7,8,9,10,11 };
			moving_corners = { 0,1,2,3,4,5,6,7 };
			moving_centers = { 0,1,2,3,4,5 };
			break;

		case MOVES::Zp:
			rotation_axis = { 0.f,0.f,1.f };
			// F'
			assign_and_permute_edges(7, 3, 5, 1);
			assign_and_permute_corners(5, 7, 3, 1);
			// S'
			assign_and_permute_edges(10, 11, 9, 8);
			assign_and_permute_centers(5, 3, 4, 2);
			// B
			assign_and_permute_edges(0, 6, 2, 4);
			assign_and_permute_corners(0, 4, 6, 2);
			// All pieces move
			moving_edges = { 0,1,2,3,4,5,6,7,8,9,10,11 };
			moving_corners = { 0,1,2,3,4,5,6,7 };
			moving_centers = { 0,1,2,3,4,5 };
			break;

		default:
			return;
		};

		moving = true;
	}

	// Step move function, rotates the pieces according to the TPS.
	void keep_moving()
	{
		float step_size = 3.14159265f / 2.f / 60.f * tps;
		if (step_size > 3.14159265f / 2.f - accum_rotation)
		{
			step_size = 3.14159265f / 2.f - accum_rotation;
			moving = false;
			accum_rotation = 0.f;
		}
		else accum_rotation += step_size;


		Quaternion rotation = Quaternion::Rotation(rotation_axis, step_size);

		for (unsigned p : moving_edges)
			edge_quat[p] *= rotation;
		for (unsigned p : moving_corners)
			corner_quat[p] *= rotation;
		for (unsigned p : moving_centers)
			center_quat[p] *= rotation;
		
	}

public:
	// Constructor, initializes the data, pushes the imgui sliders 
	// and calls for the cube generation.
	RubiksWindow() : demoWindow(&RubiksDescriptor, info, "rubiks_cube_screenshot")
	{
		// Initialize data
		setScale(350.f);
		data.window = this;
		data.rot_free =
			Quaternion::Rotation({ 0.f,0.f,-1.f }, pitch) *
			Quaternion::Rotation({ -1.f,0.f,0.f }, vert_view) *
			Quaternion::Rotation({ 0.f,-1.f,0.f }, side_view);

		// Create the cube
		generateCube();	

#ifdef _INCLUDE_IMGUI
		// Add all the sliders
		imgui.initial_size = { 315,150 };
		imgui.pushSlider(&tps, { 0.f,20.f }, "TPS");
		imgui.pushSlider(&side_view, { -3.14159265f / 2.f,3.14159265f / 2.f }, "Side View");
		imgui.pushSlider(&vert_view, { -3.14159265f / 2.f,3.14159265f / 2.f }, "Vert. View");
		imgui.pushSlider(&pitch, { -3.14159265f / 2.f,3.14159265f / 2.f }, "Pitch");
#endif
	}

	// Event manager, does the move step if exists, else it checks for other moves to be done.
	// Does a custom mouse rotation step so that the cube returns to its original position 
	// when not held. Finally updates all pieces rotation accordingly and pushes the next frame.
	void event_and_draw() override
	{
		// Check for movement or new moves.
		if (hasFocus())
		{
			// Store all keyboard inputs.
			while (char c = Keyboard::popChar())
				move_queue.push_back(c);

			// If not moving check for move inputs in storage.
			while (!moving && move_queue.size())
			{
				char c = move_queue[0];
				move_queue.erase(move_queue.begin());

				switch (c)
				{
				case 'r':
					play_move(MOVES::R);
					break;
				case 'u':
					play_move(MOVES::U);
					break;
				case 'f':
					play_move(MOVES::F);
					break;
				case 'd':
					play_move(MOVES::D);
					break;
				case 'l':
					play_move(MOVES::L);
					break;
				case 'b':
					play_move(MOVES::B);
					break;
				case 'R':
					play_move(MOVES::Rp);
					break;
				case 'U':
					play_move(MOVES::Up);
					break;
				case 'F':
					play_move(MOVES::Fp);
					break;
				case 'D':
					play_move(MOVES::Dp);
					break;
				case 'L':
					play_move(MOVES::Lp);
					break;
				case 'B':
					play_move(MOVES::Bp);
					break;
				case 'm':
					play_move(MOVES::M);
					break;
				case 's':
					play_move(MOVES::S);
					break;
				case 'e':
					play_move(MOVES::E);
					break;
				case 'M':
					play_move(MOVES::Mp);
					break;
				case 'S':
					play_move(MOVES::Sp);
					break;
				case 'E':
					play_move(MOVES::Ep);
					break;
				case 'x':
					play_move(MOVES::X);
					break;
				case 'y':
					play_move(MOVES::Y);
					break;
				case 'z':
					play_move(MOVES::Z);
					break;
				case 'X':
					play_move(MOVES::Xp);
					break;
				case 'Y':
					play_move(MOVES::Yp);
					break;
				case 'Z':
					play_move(MOVES::Zp);
					break;
				default:
					break;
				}
			}
		}

		if (moving)
			keep_moving();

		// Update rotations and perspective. Since we do a custom rotation we will 
		// need to write it down ourselves, very similar to the default manager though.
		// If no focus, accumulate rotation and leave.
		if (!hasFocus())
		{
			constexpr float force = 0.04f;
			Quaternion desired_position =
				Quaternion::Rotation({ 0.f,0.f,-1.f }, pitch) *
				Quaternion::Rotation({ -1.f,0.f,0.f }, vert_view) *
				Quaternion::Rotation({ 0.f,-1.f,0.f }, side_view);
			Quaternion attraction = (desired_position * data.rot_free.inv() + 1.f / force);
			data.rot_free = (attraction * data.rot_free).normal();
		}
		else
		{
			// We get the scene perspective data from the window.
			Vector2i dim = getDimensions() / 2;
			data.scale = graphics().getScale();

			// Use the wheel value to update the scale
			data.scale *= powf(1.1f, Mouse::getWheel() / 120.f);

			// Get the mouse new position. If you were not dragging just in 
			// case set the last position to the new one. We don't want big 
			// spins due to sudden mouse changes.
			data.last_mouse = data.new_mouse;
			data.new_mouse = Mouse::getPosition();
			if (!data.dragging && Mouse::isButtonPressed(Mouse::Left))
				data.last_mouse = data.new_mouse, data.dragging = true;
			else if (!Mouse::isButtonPressed(Mouse::Left))
				data.dragging = false;

			// Convert the mouse position to R2 given the window dimensions and scale.
			data.R2_last_mouse = { (data.last_mouse.x - dim.x) / data.scale, -(data.last_mouse.y - dim.y) / data.scale };
			data.R2_new_mouse = { (data.new_mouse.x - dim.x) / data.scale, -(data.new_mouse.y - dim.y) / data.scale };

			// Convert the positions to the sphere given the observer and the projections of the mouse to the sphere.
			Vector3f p0 = Vector3f(data.R2_last_mouse.x, data.R2_last_mouse.y, -data.sensitivity).normal();
			Vector3f p1 = Vector3f(data.R2_new_mouse.x,  data.R2_new_mouse.y, -data.sensitivity).normal();

			if (data.dragging)
			{
				// Get the rotation quaternon that would take you in a straight line from last S2 mouse position to new S2 mouse position.
				Quaternion rot = (Quaternion(p1 * p0) + 1.f + (p0 ^ p1)).normal();
				// Accumulate the rotation.
				data.rot_free *= rot;
			}
			else
			{
				constexpr float force = 0.04f;
				Quaternion desired_position = 
					Quaternion::Rotation({ 0.f,0.f,-1.f }, pitch) *
					Quaternion::Rotation({ -1.f,0.f,0.f }, vert_view) *
					Quaternion::Rotation({ 0.f,-1.f,0.f }, side_view);
				Quaternion attraction = (desired_position * data.rot_free.inv() + 1.f / force);
				data.rot_free = (attraction * data.rot_free).normal();
			}
		}
		
		graphics().setScale(data.scale);
		for (unsigned i = 0u; i < 12; i++)
			edge_pieces[i].updateRotation(data.rot_free * edge_quat[i]);
		for (unsigned i = 0u; i < 8; i++)
			corner_pieces[i].updateRotation(data.rot_free * corner_quat[i]);
		for (unsigned i = 0u; i < 6; i++)
			center_pieces[i].updateRotation(data.rot_free * center_quat[i]);

		// Clear buffers and set target.
		graphics().setRenderTarget();
		graphics().clearBuffer();
		// Draw.
		for (Polyhedron& p : corner_pieces)
			p.Draw();
		for (Polyhedron& p : edge_pieces)
			p.Draw();
		for (Polyhedron& p : center_pieces)
			p.Draw();
		// Push.
		graphics().pushFrame();
		return;
	}
};

// Sometimes you just have a very dumb idea, and you want to plot it. This is a great example 
// of that case. If a Sierpinski tetrahedron crosses your mind, and you want to see how it would 
// look like if you broke it apart into pieces due to inertia, just for the fun of it, with this 
// library you can make it real! It serves no purpose, but it is definitely a lot of fun.
class SierpinskiWindow : public demoWindow
{
private:
	static inline const char* info =
		"\n"
		"  Fractals are a really pretty mathematical concept, and visualizing them has always \n"
		"  amazed me. In this case, we are plotting a simple Sierpinski tetrahedron. But a line \n"
		"  mesh to plot a fractal like this does not sound fun enough, so how can we spice it up?\n"
		"\n"
		"  To also showcase the power of updating line or point meshes, without going through \n"
		"  the boring example of a gravity simulation, I decided to make this fractal break up \n"
		"  into small pieces when it is moved, and then return to its original position when \n"
		"  the object stands still.\n"
		"\n"
		"  Despite this being clearly a silly idea, I find it quite entertaining, and due to \n"
		"  the randomness added to the motion of the particles, it feels alive. So turn it \n"
		"  around, play with the sliders, and have some fun!\n"
		"\n"
		"  How it works:\n"
		"  This window is meant to showcase the capabilities of point and line meshes in this \n"
		"  library, so the only object drawn is a single line mesh. First, all the lines are \n"
		"  generated by subdividing a tetrahedron six times following the fractal pattern. \n"
		"  The original line positions are stored, while a separate pointer holds the actual \n"
		"  vertex positions.\n"
		"\n"
		"  Every frame, each point checks where it is supposed to be and adds velocity toward \n"
		"  that position, roughly proportional to the distance. Additionally, a small random \n"
		"  walk is applied, scaled by the velocity, to spice things up.\n"
		"\n"
		"  The desired position of each point is based on the rotation provided by the default \n"
		"  event manager, and the line mesh is updated every frame to produce the motion effect."
		"\n ";


	// Static window descriptor used by all instances of the class.
	static inline const WINDOW_DESC SierpinskiDescriptor =
	{
		"Sierpinski Tetrahedron Window",
		WINDOW_DESC::WINDOW_MODE_NORMAL,
		{ 1080, 720 },
	};

	Scatter scatter;     // Drawable that draws the lines of the set.
	EventData data = {}; // Stores the event data for the window.

private:
	static constexpr unsigned depth = 6u;     // Depth at which the set is generated.

	float drag_coef = 0.95f; // Drag coefficient for moving points.
	float time_step = 0.15f; // Time elapsed between every frame.

	Color* point_colors = nullptr;			// Pointer to the set colors.
	Vector3f* set_lines = nullptr;			// Pointer to the set lines.
	Vector3f* original_positions = nullptr; // Pointer to the set original positions.
	Vector3f* point_velocities = nullptr;	// Pointer to the set velocities.
	unsigned count = 0u;					// Stores the total number of points in the set.
	
	// Function to generate the Sierpinski tetrahdron.
	void generateSet()
	{
		if (set_lines)
			delete[] set_lines;

		unsigned tetras = 1u;
		count = tetras * 12u;

		set_lines = new Vector3f[count];

		const Vector3f v0 = Vector3f{  1.f,  1.f,  1.f };
		const Vector3f v1 = Vector3f{  1.f, -1.f, -1.f };
		const Vector3f v2 = Vector3f{ -1.f,  1.f, -1.f };
		const Vector3f v3 = Vector3f{ -1.f, -1.f,  1.f };

		set_lines[ 0] = v0; set_lines[ 1] = v1;
		set_lines[ 2] = v1; set_lines[ 3] = v2;
		set_lines[ 4] = v2; set_lines[ 5] = v0;
		set_lines[ 6] = v3; set_lines[ 7] = v0;
		set_lines[ 8] = v3; set_lines[ 9] = v1;
		set_lines[10] = v3; set_lines[11] = v2;

		for (unsigned d = 0; d < depth; d++)
		{
			unsigned old_tetras = tetras;
			tetras *= 4u;
			count = tetras * 12u;
			Vector3f* new_set_lines = new Vector3f[count];

			Vector3f* old_idx = set_lines;
			Vector3f* new_idx = new_set_lines;

			for (unsigned i = 0u; i < old_tetras; i++)
			{
				const Vector3f v00 = old_idx[0];
				const Vector3f v01 = (old_idx[0] + old_idx[2]) / 2.f;
				const Vector3f v02 = (old_idx[0] + old_idx[4]) / 2.f;
				const Vector3f v03 = (old_idx[0] + old_idx[6]) / 2.f;

				const Vector3f v10 = old_idx[2];
				const Vector3f v11 = (old_idx[2] + old_idx[0]) / 2.f;
				const Vector3f v12 = (old_idx[2] + old_idx[4]) / 2.f;
				const Vector3f v13 = (old_idx[2] + old_idx[6]) / 2.f;

				const Vector3f v20 = old_idx[4];
				const Vector3f v21 = (old_idx[4] + old_idx[2]) / 2.f;
				const Vector3f v22 = (old_idx[4] + old_idx[0]) / 2.f;
				const Vector3f v23 = (old_idx[4] + old_idx[6]) / 2.f;

				const Vector3f v30 = old_idx[6];
				const Vector3f v31 = (old_idx[6] + old_idx[2]) / 2.f;
				const Vector3f v32 = (old_idx[6] + old_idx[4]) / 2.f;
				const Vector3f v33 = (old_idx[6] + old_idx[0]) / 2.f;

				new_idx[12 * 0 +  0] = v00; new_idx[12 * 0 +  1] = v01;
				new_idx[12 * 0 +  2] = v01; new_idx[12 * 0 +  3] = v02;
				new_idx[12 * 0 +  4] = v02; new_idx[12 * 0 +  5] = v00;
				new_idx[12 * 0 +  6] = v03; new_idx[12 * 0 +  7] = v00;
				new_idx[12 * 0 +  8] = v03; new_idx[12 * 0 +  9] = v01;
				new_idx[12 * 0 + 10] = v03; new_idx[12 * 0 + 11] = v02;

				new_idx[12 * 1 +  0] = v10; new_idx[12 * 1 +  1] = v11;
				new_idx[12 * 1 +  2] = v11; new_idx[12 * 1 +  3] = v12;
				new_idx[12 * 1 +  4] = v12; new_idx[12 * 1 +  5] = v10;
				new_idx[12 * 1 +  6] = v13; new_idx[12 * 1 +  7] = v10;
				new_idx[12 * 1 +  8] = v13; new_idx[12 * 1 +  9] = v11;
				new_idx[12 * 1 + 10] = v13; new_idx[12 * 1 + 11] = v12;

				new_idx[12 * 2 +  0] = v20; new_idx[12 * 2 +  1] = v21;
				new_idx[12 * 2 +  2] = v21; new_idx[12 * 2 +  3] = v22;
				new_idx[12 * 2 +  4] = v22; new_idx[12 * 2 +  5] = v20;
				new_idx[12 * 2 +  6] = v23; new_idx[12 * 2 +  7] = v20;
				new_idx[12 * 2 +  8] = v23; new_idx[12 * 2 +  9] = v21;
				new_idx[12 * 2 + 10] = v23; new_idx[12 * 2 + 11] = v22;

				new_idx[12 * 3 +  0] = v30; new_idx[12 * 3 +  1] = v31;
				new_idx[12 * 3 +  2] = v31; new_idx[12 * 3 +  3] = v32;
				new_idx[12 * 3 +  4] = v32; new_idx[12 * 3 +  5] = v30;
				new_idx[12 * 3 +  6] = v33; new_idx[12 * 3 +  7] = v30;
				new_idx[12 * 3 +  8] = v33; new_idx[12 * 3 +  9] = v31;
				new_idx[12 * 3 + 10] = v33; new_idx[12 * 3 + 11] = v32;

				new_idx += 48;
				old_idx += 12;
			}

			delete[] set_lines;
			set_lines = new_set_lines;
		}

		const Color minus_x = Color::Blue;
		const Color plus_x  = Color::Yellow / 2;
		const Color minus_y = Color::Green;
		const Color plus_y  = Color::Orange;
		const Color minus_z = Color::Red;
		const Color plus_z  = Color::Purple / 2;

		point_colors = new Color[count];
		for (unsigned i = 0; i < count; i++)
			point_colors[i] =
				minus_x * ((1.f - set_lines[i].x) / 6.f) +
				minus_y * ((1.f - set_lines[i].y) / 6.f) +
				minus_z * ((1.f - set_lines[i].z) / 6.f) +
				plus_x  * ((1.f + set_lines[i].x) / 6.f) +
				plus_x  * ((1.f + set_lines[i].x) / 6.f) +
				plus_x  * ((1.f + set_lines[i].x) / 6.f) + 
			Color(unsigned char(rand() / 2048.f), unsigned char(rand() / 2048.f), unsigned char(rand() / 2048.f));

		original_positions = new Vector3f[count];
		memcpy(original_positions, set_lines, count * sizeof(Vector3f));

		point_velocities = new Vector3f[count];
		memset(point_velocities, 0, count * sizeof(Vector3f));
	}
	
	// Recomputes point velocities and position based on rotation.
	void updatePoints()
	{
		for (unsigned i = 0; i < count / 2u; i++)
		{
			Vector3f to_be = (data.rot_free * Quaternion(original_positions[2 * i]) * data.rot_free.inv()).getVector();
			Vector3f force = (to_be - set_lines[2 * i]);

			point_velocities[2 * i] *= drag_coef;
			point_velocities[2 * i] += force * time_step;

			to_be = (data.rot_free * Quaternion(original_positions[2 * i + 1]) * data.rot_free.inv()).getVector();
			force = (to_be - set_lines[2 * i + 1]);

			point_velocities[2 * i + 1] *= drag_coef;
			point_velocities[2 * i + 1] += force * time_step;

			const Vector3f random_vel = (Vector3f(rand() / 32768.f, rand() / 32768.f, rand() / 32768.f) + 4 * set_lines[2 * i] / set_lines[2 * i].abs()) * point_velocities[2 * i].abs() / 30.f;
			point_velocities[2 * i] += random_vel;
			point_velocities[2 * i + 1] += random_vel;

			set_lines[2 * i] += point_velocities[2 * i] * time_step;
			set_lines[2 * i + 1] += point_velocities[2 * i + 1] * time_step;
		}
	}

public:
	// Constructor, initializes the Sierpinski tetrahedron, creates the drawable, and pushes the imgui sliders.
	SierpinskiWindow() : demoWindow(&SierpinskiDescriptor, info, "sierpinski_screenshot")
	{
		// Set initial data.
		data.window = this;

		// Generate Sierpinski tetrahedron.
		generateSet();

		// Create descriptor.
		SCATTER_DESC desc = {};
		desc.coloring = SCATTER_DESC::POINT_COLORING;
		desc.blending = SCATTER_DESC::OPAQUE_POINTS;
		desc.line_mesh = true;
		desc.point_list = set_lines;
		desc.color_list = point_colors;
		desc.point_count = count;
		desc.enable_updates = true;
		// Initialize scatter plot.
		scatter.initialize(&desc);

#ifdef _INCLUDE_IMGUI
		// Push imgui sliders.
		imgui.initial_size = { 315,100 };
		imgui.pushSlider(&drag_coef, { 0.f,1.f }, "Momentum");
		imgui.pushSlider(&time_step, { 0.f,1.f }, "Step Size");
#endif
	}

	// Destructor, frees the internal pointers.
	~SierpinskiWindow() { delete[] set_lines; delete[] point_velocities; delete[] original_positions; delete[] point_colors; }

	// Updates the window data, calls the inertia update function and redraws the scatter plot.
	// Then it pushes the next frame to the window.
	void event_and_draw() override
	{
		// Do some event management.
		defaultEventManager(data);
		graphics().setPerspective({ 0.65f,0.25f,-0.5f,0.1f }, {}, data.scale);

		// Recompute points.
		updatePoints();

		// Update scatter.
		scatter.updatePoints(set_lines);

		// Clear buffers and set target.
		graphics().setRenderTarget();
		graphics().clearBuffer();
		// Draw.
		scatter.Draw();
		// Push.
		graphics().pushFrame();
		return;
	}
};

// Fourier demoWindow. In honor of my bachelor's thesis, that inspired the creation of this
// library, I rebuilt the spherical harmonics drawn on my original program but with a much 
// more efficient algorithm I was not able to add due to time constraints. 
// The spherical harmonics are a very important set of functions that find applications in 
// many areas such as chemistry, with the atomic orbitals, astrophysics, with the calculations
// of orbits, and also math. In my case I was using them as a Fourier basis to represent star-
// shaped objects. The original program, with those functionalities, can still be found in 
// my GitHub page, the code is a mess though :)
// Link: https://github.com/MiquelNasarre/FourierS2.git
class FourierWindow : public demoWindow
{
private:
	static inline const char* info =
		"\n"
		"  In honor of my bachelor's thesis, which inspired the creation of this library, I \n"
		"  rebuilt the spherical harmonics drawn in my original program, but using a much \n"
		"  more efficient algorithm that I was not able to implement in the thesis due to \n"
		"  time constraints.\n"
		"\n"
		"  Spherical harmonics are a very important set of functions that find applications \n"
		"  in many areas such as chemistry, with atomic orbitals, astrophysics, with orbit \n"
		"  calculations, and mathematics. In my case, I was using them as a Fourier basis \n"
		"  to represent star-shaped objects.\n"
		"\n"
		"  The original program, with those functionalities, can still be found on my \n"
		"  GitHub page. The code is a mess, though. :)\n"
		"\n"
		"  Despite not having the Fourier series functionality, what this specific window \n"
		"  does better is the computation of spherical harmonics. The original formula that \n"
		"  defines these functions is expressed in spherical coordinates, which requires \n"
		"  multiple trigonometric computations and, as always, tends to produce poorly \n"
		"  behaved results, especially near the poles.\n"
		"\n"
		"  There is, however, a much cleaner way to define them computationally, based on \n"
		"  the proof I followed in my thesis to show that they indeed form a basis for \n"
		"  the Fourier series on S^2. The proof centers around showing that spherical \n"
		"  harmonics can also be written as harmonic polynomials in their R^3 coordinates, \n"
		"  ultimately yielding an explicit formula for such polynomials.\n"
		"\n"
		"  This polynomial formulation makes the computation much simpler and cleaner, and \n"
		"  it is the one implemented to plot the surfaces in this window, as well as to \n"
		"  derive them and compute the normal vectors exactly.\n"
		"\n"
		"  How it works:\n"
		"  This window consists of a single surface, namely the spherical harmonic. Every \n"
		"  time the parameter L or M is changed, the surface is recomputed.\n"
		"\n"
		"  Fittingly, the surface type is spherical, so it receives (x,y,z) coordinates \n"
		"  from S^22 as input, exactly the values required by the polynomials. It returns \n"
		"  the absolute value of the resulting radius and caches the sign to color the \n"
		"  function accordingly."
		"\n ";


	// Static window descriptor used for all instances of this class.
	static inline const WINDOW_DESC FourierDescriptor =
	{
		"Chaotic Fourier Window",
		WINDOW_DESC::WINDOW_MODE_NORMAL,
		{ 1080, 720 },
	};

	EventData data = {}; // Event data storage for this class.
	Surface spherical;	 // Drawable that represents the spherical harmonics.

private:
	// Global maximum value the variable L can take.
	static inline constexpr unsigned maxL = 32u;

	// Global parameters to be used by the spherical functions.
	static inline int L;
	static inline int M;
	static inline double* Kn;
	static inline Color next_color;

	// Local storage for the personal parameters of this window.
	int imguiL = 7, imguiM = -4;
	int myL = 7, myM = -4;
	double myKn[maxL / 2 + 1];

	// Helper function for Kn computation.
	static inline double factorial(int min, int max)
	{
		double fact = 1.0;
		for (int i = min + 1; i <= max; i++)
			fact *= double(i);
		return fact;
	}

	// Computes constants that will be used across the entire spherical computation.
	void compute_kns()
	{
		int m = abs(myM);
		int l = myL;

		bool parity = ((l - m) % 2);

		double frac = (m % 2) ? 1.0 : -1.0;
		frac /= pow(2.0, l);

		double Klm = sqrt((2.0 * l + 1.0) / factorial(l - m, l + m));
		if (!m) Klm /= sqrt(2.0);

		if (parity) m += 1;
		for (int n = 0; n <= (l - m) / 2; n++)
		{
			double an = ((n + (l + m) / 2 + 1) % 2) ? -1.0 : 1.0;
			an *= factorial(2 * n, 2 * n + l + m);
			an /= factorial(1, n + (l + m) / 2);
			an /= factorial(1, (l - m) / 2 - n);
			if (parity) an /= (2.0 * n + 1.0);

			myKn[n] = frac * an * Klm;
		}
	}

	// Spherical function that returns the radius and stores the next color.
	static float spherical_harmonics(float x, float y, float z)
	{
		double re = 1.f;
		double im = 0.f;

		double temp_re, temp_im;
		
		int absM = abs(M);
		for (int i = 0; i < absM; i++)
		{
			temp_re = re * x - im * y;
			temp_im = re * y + im * x;

			re = temp_re; im = temp_im;
		}
		double P = (M >= 0) ? re : im;

		double Q = 0.f;
		if ((L - absM) % 2)
			for (int n = 0; n <= (L - absM - 1) / 2; n++)
				Q += Kn[n] * pow((double)z, 2.0 * n + 1.0);
		
		else 
			for (int n = 0; n <= (L - absM) / 2; n++)
				Q += Kn[n] * pow((double)z, 2.0 * n);

		float r = float(P * Q);
		next_color = (r > 0.f) ? Color::Blue : Color::Yellow;
		return fabsf(r);
	}

	// Placeholder function that returns the color saved by spherical function.
	static Color output_function_coloring(float, float, float) { return next_color; }

	// Spherical function to compute the normal vectors of the surface, this is a fun one.
	static Vector3f normal_spherical_harmonic(float _x, float _y, float _z)
	{
		float norm = sqrtf(_x * _x + _y * _y + _z * _z);
		if (norm < 1e-6f) 
			return {};

		const double x = double(_x) / norm;
		const double y = double(_y) / norm;
		const double z = double(_z) / norm;

		double re = 1.f;
		double im = 0.f;

		double temp_re, temp_im;

		int absM = abs(M);
		for (int i = 0; i < absM - 1; i++)
		{
			temp_re = re * x - im * y;
			temp_im = re * y + im * x;

			re = temp_re; im = temp_im;
		}
		const double Px = (M >= 0) ? double(absM) * re : double(absM) * im;
		const double Py = (M >= 0) ? -double(absM) * im : double(absM) * re;

		if (absM)
		{
			temp_re = re * x - im * y;
			temp_im = re * y + im * x;

			re = temp_re; im = temp_im;
		}
		const double P = (M >= 0) ? re : im;

		double Q = 0.f;
		if ((L - absM) % 2)
			for (int n = 0; n <= (L - absM - 1) / 2; n++)
				Q += Kn[n] * pow(z, 2.0 * n + 1.0);
		else
			for (int n = 0; n <= (L - absM) / 2; n++)
				Q += Kn[n] * pow(z, 2.0 * n);

		double Qz = 0.f;
		if ((L - absM) % 2)
			for (int n = 0; n <= (L - absM - 1) / 2; n++)
				Qz += (2.0 * n + 1.0) * Kn[n] * pow(z, 2.0 * n);
		else
			for (int n = 1; n <= (L - absM) / 2; n++)
				Qz += (2.0 * n) * Kn[n] * pow(z, 2.0 * n - 1.0);

		double drx = Px * Q;
		double dry = Py * Q;
		double drz = P * Qz;
		double r = P * Q;

		Vector3d s2(x, y, z);
		Vector3d grad(drx, dry, drz);

		Vector3d grad_t = grad - (grad ^ s2) * s2;
		Vector3d normal = (r > 0.0) ? (r * s2 - grad_t).normal() : -(r * s2 - grad_t).normal();

		return { float(normal.x), float(normal.y), float(normal.z) };
	}

public:
	// Constructor, initializes the window. Creates the surface descriptor calls 
	// the initializer and sets the lights. Pushes imgui sliders.
	FourierWindow(): demoWindow(&FourierDescriptor, info, "fourier_screenshot")
	{
		// Set initial window datas
		setScale(350.f);
		data.window = this;
		data.rot_free = Quaternion(cosf(0.25f), 0.f, sinf(0.25f), 0.f);
		data.d_rot_free = Quaternion::Rotation({ sinf(0.5f), 0.f, cosf(0.5f) }, -0.005f);

		// Initialize Kn and set your values
		compute_kns();
		Kn = myKn;
		L = myL;
		M = myM;

		// Initialize your surface
		SURFACE_DESC desc = {};
		desc.type = SURFACE_DESC::SPHERICAL_SURFACE;
		desc.coloring = SURFACE_DESC::OUTPUT_FUNCTION_COLORING;
		desc.normal_computation = SURFACE_DESC::OUTPUT_FUNCTION_NORMALS;
		desc.default_initial_lights = false;
		desc.spherical_func = spherical_harmonics;
		desc.output_color_func = output_function_coloring;
		desc.output_normal_func = normal_spherical_harmonic;
		desc.enable_updates = true;
		desc.icosphere_depth = 6u;
		spherical.initialize(&desc);
		spherical.updateLight(0, { 880.f,340.f }, Color::White, { 30.f, 10.f, 20.f });

#ifdef _INCLUDE_IMGUI
		// Add imgui sliders and default settings.
		imgui.pushSliderInt(&imguiL, { 0, maxL }, "L value");
		imgui.pushSliderInt(&imguiM, { -imguiL, imguiL }, "M value");
		imgui.initial_size = { 315,100 };
#endif
	}

	// If any of the values have been modified it recomputes the spherical harmonic.
	// Does some basic event management and draws the next frame.
	void event_and_draw() override
	{
		// Update shape if variables change
		if (imguiL != myL || imguiM != myM)
		{
			// Keep boundaries right
			while (imguiM > imguiL) imguiM--;
			while (imguiM < -imguiL) imguiM++;
			if (imguiL > maxL) imguiL = maxL;
			if (imguiL < 0) imguiL = 0;

#ifdef _INCLUDE_IMGUI
			// Recreate M slider
			imgui.eraseSliderInt(1);
			imgui.pushSliderInt(&imguiM, { -imguiL, imguiL }, "M value");
#endif
			// Store new values
			myL = imguiL;
			myM = imguiM;

			// Recompute Kn values
			compute_kns();

			// Set global values
			L = myL;
			M = myM;
			Kn = myKn;

			// Recompute shape
			spherical.updateShape();
		}

		// Do some event management.
		defaultEventManager(data);
		Quaternion obs = (Quaternion(1.f, 0.f, 1.f, 0.f) * Quaternion(1.f, -1.f, 0.f, 0.f)).normal();
		graphics().setPerspective(obs, {}, data.scale);
		spherical.updateRotation(data.rot_free);

		// Clear buffers and set target.
		graphics().setRenderTarget();
		graphics().clearBuffer();
		// Draw.
		spherical.Draw();
		// Push.
		graphics().pushFrame();
		return;
	}
};

// To be called before event_and_draw(). Events shared by all demo windows, like 
// new window events, screenshots and some keyboard updates are managed here.
bool demoWindow::new_window_event()
{
	// Check for screenshots.
	if (capture_scheduled)
	{
		capture_scheduled = false;
		screenshot.save(screenshot_name);
		screenshot.reset(0, 0);
	}

	// Check for keyboard updates.
	if (hasFocus())
	{
		if (Keyboard::isKeyPressed(0x7A /*VK_F11*/))
			screen_mode = 2;
		if (Keyboard::isKeyPressed(0x1B /*VK_ESCAPE*/))
			screen_mode = 1;

		if (
			!Keyboard::isKeyPressed('S') &&
			!Keyboard::isKeyPressed('M') &&
			Keyboard::isKeyPressed(0x11 /*VK_CONTROL*/)
			) ctrl_pressed = true;

		else if (!Keyboard::isKeyPressed(0x11 /*VK_CONTROL*/))
			ctrl_pressed = false;

		if (ctrl_pressed && Keyboard::isKeyPressed('S'))
		{
			ctrl_pressed = false;
			scheduleFrameCapture(&screenshot);
			capture_scheduled = true;
			Keyboard::popChar();
		}
#ifdef _INCLUDE_IMGUI
		else if (ctrl_pressed && Keyboard::isKeyPressed('M'))
		{
			ctrl_pressed = false;
			if (imgui.visible)
				imgui.visible = false;
			else
				imgui.visible = true;
			Keyboard::popChar();
		}
#endif
	}

	// Check for screen mode uptdates.
	if (screen_mode)
	{
		switch (screen_mode)
		{
		case 1:
			setFullScreen(false);
			break;

		case 2:
			setFullScreen(true);
			break;

		case 3:
			scheduleFrameCapture(&screenshot);
			capture_scheduled = true;
			break;
#ifdef _INCLUDE_IMGUI
		case 4:
			if (imgui.visible)
				imgui.visible = false;
			else
				imgui.visible = true;
			break;
#endif

		default:
			break;

		}
		screen_mode = 0;
	}

	// Now do the new window event.
	if (add_new_window == NONE)
		return false;

	switch (add_new_window)
	{
	case LORENTZ:
		new LorenzWindow;
		break;

	case HOPF_FIBRATION:
		new HopfFibrationWallpaper;
		break;

	case GAME_OF_LIFE:
		new GameOfLifeWindow;
		break;

	case BOUNCING_BALLS:
		new BouncingBallsWindow;
		break;

	case OCEAN_PLANET:
		new OceanPlanetWindow;
		break;

	case RUBIKS_CUBE:
		new RubiksWindow;
		break;

	case SIERPINSKI_TETRA:
		new SierpinskiWindow;
		break;

	case FOURIER:
		new FourierWindow;
		break;

	default:
		break;
	}

	add_new_window = NONE;
	return true;
}

// This function runs the library demo!! All the relevant information 
// is provided inside the function itself, run it and enjoy!
// Source code can be found at 'chaotic/source/chaotic_demo.cpp'.
void chaotic_demo()
{
	// Create a Lorentz attractor demoWindow to start with.
	new LorenzWindow;

	// Run loop until there is no window left.
	while (demoWindow::active_windows.size() != 0u)
	{
		// Call process events that will run the message pump for 
		// all active Windows, and store the output value.
		if (unsigned id = Window::processEvents())
			// If the value is non-zero a window has to die, find it, kill it.
			for (unsigned i = 0; i < demoWindow::active_windows.size(); i++)
				if (demoWindow::active_windows[i]->getID() == id) { delete demoWindow::active_windows[i]; break; }

		// Check for new window events and other universal checks.
		for (demoWindow* w : demoWindow::active_windows)
			if (w->new_window_event())
				break;

		// Do step and draw each windows.
		for (demoWindow* w : demoWindow::active_windows)
			w->event_and_draw();
	}
}
#endif