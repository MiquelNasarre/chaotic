#pragma once

/* LICENSE AND COPYRIGHT
-------------------------------------------------------------------------------------------------------
 * Chaotic — a 3D renderer for mathematical applications
 * Copyright (c) 2025-2026 Miguel Nasarre Budiño
 * Licensed under the MIT License. See LICENSE file.
-------------------------------------------------------------------------------------------------------
 */

// Default library dependencies.
#define _INCLUDE_DEFAULT_ERROR
#define _INCLUDE_CONSTANTS
#include "chaotic.h"

// Some std dependencies for easier user experience.
#include <vector>
#include <string>
using std::vector;
using std::string;

// Toggle to enable or disable demo during build, for memory saving.
#define _ENABLE_CHAOTIC_DEMO

/* API DEFAULT HELPERS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains a set of classes, structures and functions that will help you
start designing with this API with some simple default settings. It includes default
imGui managers, default windows, and default event management.

The use of these helpers allow you to achieve quite remarkable apps with minimal 
lines of code. And serve as a good introduction to the capabilities of the library. 
But by no means represent the flexibility that this library brings.

That is why if you are planning to build some more complex 3D App I strongly recommend
slowly moving away from this header and creating your own classes to run your app. 

Abstraction has a strong hierarchy in this API. At a surface level you will only see
the Window, Graphics, Mouse, Keyboard and Drawable tools you have to create the apps.
Those allows for quite a lot of freedom in the way you plot and design your figures, 
having multiple default drawables that allow for all different kinds of plots. 

At an intermediate level, you have access to the bindable classes and this allows for 
the creation of your own drawable classes and shaders, to visualize exactly what you 
want to see and how you want to see it. 

And at a low level you can play with DirectX11 and Win32 yourself!! With the creation
of new bindables or the addition of new features to the Graphics and Window classes.

I strongly reccommend checking the library headers and source files for a better 
understanding and some inspiration on how to start building with the API.
But if you are just here to plot something cool feel free to use this header :)
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

#ifdef _ENABLE_CHAOTIC_DEMO
// This function runs the library demo!! All the relevant information 
// is provided inside the function itself, run it and enjoy!
// Source code can be found at 'chaotic/source/chaotic_demo.cpp'.
extern void chaotic_demo();
#endif

// Both surfaces and polyhedrons allow for ilumination, and both of them require
// three parameters for every lightsource update, the position, the intensities
// and the color. Therefore this struct is a compact way of representing a light
// for both of those drawables.
struct LightSource
{
	Vector3f position = {};		// 3D vector representing the light position.
	Vector2f intensities = {};	// 2D vector representing direct and diffused light intesities.
	Color color = {};			// Color of the light in (R,G,B,A) unsigned char.
};

// Something that really brings a program to life is the capacity to use the 
// mouse and keyboard to move stuff around. For that kind of interaction you 
// need some feedback from where the mouse is, what buttons are pressed, and 
// what keyboard keys are pressed and how.
// For all this the API provides the mouse and keyboard class, but those might
// feel a bit confusing at first, that is why this struct exists. When you feed 
// it to the default event manager it will get updated with data free to use for 
// your code and your app interactions.
struct EventData
{
	// MANDATORY: Pointer to the window you want to work with, to extract its 
	// current perspective and dimensions. Does not modify it.
	Window* window = nullptr;

	// To rotate objects around the API uses quaternions, these two are defined
	// by a rotation given by the mouse position projected to the sphere. If you
	// use them to update your objects rotation you will feel them very smooth.
	// You can use them in your drawables by calling updateRotation(rot_free).
	// If you are looking for other kind of motions feel free to implement them
	// yourself :)
	Quaternion d_rot_free = 1.f;	// Quaternion composed to the rotation.
	Quaternion rot_free = 1.f;		// Total accumulated rotation.

	// Controls the sensitivity of the free rotation.
	float sensitivity = 1.f;
	// Stores how the mouse wheel has changed since last frame.
	float d_mouse_wheel = 0.f;

	// Stores the last mouse screen position in pixels.
	Vector2i last_mouse = {};
	// Stores the new mouse screen position in pixels.
	Vector2i new_mouse = {};

	// Stores the last mouse screen position scaled to R2 coordinates.
	Vector2f R2_last_mouse = {};
	// Stores the new mouse screen position scaled to R2 coordinates.
	Vector2f R2_new_mouse = {};

	// Stores the last mouse screen position projected onto the sphere.
	Vector3f S2_last_mouse = {};
	// Stores the new mouse screen position projected onto the sphere.
	Vector3f S2_new_mouse = {};

	// Whether the mouse is dragging an object.
	bool dragging = false;

	// Scale that takes the current window scale and updates it with the
	// mouse wheel if not dragging. You can use it by setting your default
	// window scale to it, or by calling setPerspective()/setScale().
	float scale = 250.f;
};

// Default event manager function, takes in a reference to an EventData object
// with a valid window pointer and updates the data inside the struct accordingly.
// It can be called every frame inside the Window::processEvents() loop.
// If the window pointer is not valid it will early return.
static void defaultEventManager(EventData& data)
{
	// If the window is not provided there is not much we can do.
	USER_CHECK(data.window,
		"Called defaultEventManager on an EventData with an invalid window pointer.\n"
		"A valid window pointer must exist for the default event manager to work."
	);

	// We get the scene perspective data from the window.
	Quaternion observer = data.window->graphics().getObserver();
	Vector2i dim = data.window->getDimensions() / 2;
	data.scale = data.window->graphics().getScale();

	// If no focus, accumulate rotation and leave.
	if (!data.window->hasFocus())
	{
		data.rot_free *= data.d_rot_free;
		return;
	}

	// Get the mouse wheel spin change
	data.d_mouse_wheel = (float)Mouse::getWheel();

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
	Vector3f p0 = { data.R2_last_mouse.x, data.R2_last_mouse.y, -data.sensitivity };
	Vector3f p1 = { data.R2_new_mouse.x,  data.R2_new_mouse.y, -data.sensitivity };

	data.S2_last_mouse = (observer.inv() * Quaternion(p0) * observer).getVector().normal();
	data.S2_new_mouse = (observer.inv() * Quaternion(p1) * observer).getVector().normal();

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
	else data.scale *= powf(1.1f, data.d_mouse_wheel / 120.f);

	// Accumulate the rotation.
	data.rot_free *= data.d_rot_free;
}

#ifdef _INCLUDE_IMGUI
#include "imgui.h"
// ImGui is a natural addition to this library due to its very powerful and simple widget
// capabilities. If you have ImGui included this class defines a simple default iGManager
// from which to add imGui widgets to your app. It has multiple functions to add sliders, 
// selectors and a light editing function. Read the class comments for more information 
// on how to use it.
class defaultImGui : public iGManager	// API ImGui implementation class
{
public:
	// Constructor expects a window to bind and will use the default iGMnaager constructor.
	// If you later want to bind it to another window you can use the iGManager functions.
	defaultImGui(Window& _w) : iGManager(_w) {}

	// You can add simple imgui float sliders to the widgets. Just send the address of the
	// float you want to slide, the range, and the name of the slider. And it will be used
	// during the render call. Make sure the float life is as long as this objects.
	void pushSlider(float* address, Vector2f range, const char* name)
	{
		if (!address)
			return;

		sliders.push_back(address);
		slider_ranges.push_back(range);
		slider_names.push_back(name);
	}

	// Erases the slider with the specified idx from the widgets if it exists.
	void eraseSlider(unsigned idx)
	{
		if (sliders.size() <= idx)
			return;

		sliders.erase(sliders.begin() + idx);
		slider_ranges.erase(slider_ranges.begin() + idx);
		slider_names.erase(slider_names.begin() + idx);
	}

	// You can add simple imgui integer sliders to the widgets. Just send the address of the
	// integer you want to slide, the range, and the name of the slider. And it will be used
	// during the render call. Make sure the integer life is as long as this objects.
	void pushSliderInt(int* address, Vector2i range, const char* name)
	{
		if (!address)
			return;

		sliders_int.push_back(address);
		slider_ranges_int.push_back(range);
		slider_names_int.push_back(name);
	}

	// Erases the slider with the specified idx from the widgets if it exists.
	void eraseSliderInt(unsigned idx)
	{
		if (sliders_int.size() <= idx)
			return;

		sliders_int.erase(sliders_int.begin() + idx);
		slider_ranges_int.erase(slider_ranges_int.begin() + idx);
		slider_names_int.erase(slider_names_int.begin() + idx);
	}

	// You can add different menu bar selections to the widgets. For that you will need to 
	// provide the name of the selector, the range of integers it will select from (including 
	// start and end), the integer address where it will write its selections, and the names 
	// of all the possible selections arraged with the indexation { 0, ..., range.y - range.x }.
	// When you click a selection in the widgets it will automatically assign that value to the 
	// integer address provided. Make sure the address lifetime is as long as this objects.
	void pushSelector(const char* selector_name, Vector2i range, int* intenger_address, const char** integer_names)
	{
		if (!intenger_address || range.x > range.y || !integer_names)
			return;

		sel_integers.push_back(intenger_address);
		sel_ranges.push_back(range);
		selector_names.push_back(selector_name);

		vector<string> names;

		for (int i = 0u; i <= range.y - range.x; i++)
			names.push_back(integer_names[i]);

		int_names.push_back(names);
	}

	// Erases the selector with the specified idx from the widgets if it exists.
	void eraseSelector(unsigned idx)
	{
		if (sel_integers.size() <= idx)
			return;

		sel_integers.erase(sel_integers.begin() + idx);
		sel_ranges.erase(sel_ranges.begin() + idx);
		int_names.erase(int_names.begin() + idx);
		selector_names.erase(selector_names.begin() + idx);
	}

	// Light source editor. Send in a light source address and during rendering it will pop 
	// a light editor widget. The widget will stay there until the edition is finished by 
	// the user, modifying the light source as specified. When finished it will release the 
	// pointer from its storage. Make sure the lifetime of the pointer is as long as the editor.
	void editLightSource(LightSource* address)
	{
		if (!address || light)
			return;

		storage = *address;
		light = address;
	}

	// If it is holding a lightsource pointer it releases it and stops running the editor.
	void popLightSource()
	{
		light = nullptr;
	}

	// Color editor. Send in a color address and during rendering it will pop  a color editor 
	// widget. The widget will stay there until the edition is finished by the user, modifying 
	// the color as specified. When finished it will release the pointer from its storage. 
	// Make sure the lifetime of the pointer is as long as the editor.
	void editColor(Color* address)
	{
		if (!address || color)
			return;

		color_storage = *address;
		color = address;
	}

	// If it is holding a lightsource pointer it releases it and stops running the editor.
	void popColor()
	{
		color = nullptr;
	}

	// Stores an ImGui user function to be added to the basic render call.
	void inject(void(*your_imgui)()) { injected = your_imgui; }

	// Rendering function override. This function will be called automatically by the bind 
	// window during push frame calls and will render and run all the imGui widgets as built 
	// by the user. You do not have to call this function yourself.
	void render() override 
	{
		// Return if invisible.
		if (!visible)
			return;

		// We now render the main widgets window.
		if (ImGui::Begin(title, NULL, ImGuiWindowFlags_MenuBar))
		{
			// If it is the first call of this imgui context it will set some defaults.
			ImGui::SetWindowPos(ImVec2(2, 2), ImGuiCond_Once);
			ImGui::SetWindowCollapsed(true, ImGuiCond_Once);
			ImGui::SetWindowSize(ImVec2((float)initial_size.x, (float)initial_size.y), ImGuiCond_Once);

			// Render the selectors if they exist.
			if (sel_integers.size())
				if (ImGui::BeginMenuBar())
				{
					for (unsigned s = 0u; s < sel_integers.size(); s++)
						if (ImGui::BeginMenu(selector_names[s].c_str()))
						{
							for (int i = sel_ranges[s].x; i <= sel_ranges[s].y; i++)
								if (ImGui::MenuItem(int_names[s][i - sel_ranges[s].x].c_str()))
									*sel_integers[s] = i;

							ImGui::EndMenu();
						}

					ImGui::EndMenuBar();
				}

			// Render the integer sliders if they exist.
			for (unsigned s = 0u; s < sliders_int.size(); s++)
				ImGui::SliderInt(slider_names_int[s].c_str(), sliders_int[s], slider_ranges_int[s].x, slider_ranges_int[s].y);

			// Render the sliders if they exits.
			for (unsigned s = 0u; s < sliders.size(); s++)
				ImGui::SliderFloat(slider_names[s].c_str(), sliders[s], slider_ranges[s].x, slider_ranges[s].y);

			
		}
		ImGui::End();

		// If we are holding a light in the pointer run the light editor for that light.
		if (light)
		{
			if (ImGui::Begin(" Light editor", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
			{
				ImGui::Text("Color:");

				// First do one of the default ImGui color pickers.
				_float4color col = light->color.getColor4();
				// The reason why we can send &col as a float*, is because _float4 and Vector*f 
				// structs are tightly packed and can be interpreted as float*.
				ImGui::ColorPicker4("", (float*)&col);
				light->color = Color(col);

				// We add a double slider for the intensities.
				ImGui::Spacing();
				ImGui::Text("Intensities:");
				ImGui::SliderFloat2("", (float*)&light->intensities, 0.f, 1000.f, "%.3f", ImGuiSliderFlags_Logarithmic);

				// We add a triple slider for the positions.
				ImGui::Spacing();
				ImGui::Text("Position:");
				ImGui::SliderFloat3(" ", (float*)&light->position, -100.f, 100.f, "%.3f", ImGuiSliderFlags_Logarithmic);

				// Cancel buttor returns to the buffer and exits edition.
				ImGui::SetCursorPos(ImVec2(290, 285));
				if (ImGui::Button("Cancel", ImVec2(80, 45))) 
				{
					*light = storage;
					light = nullptr;
				}
				// Clear button sets the light to black and exits edition.
				ImGui::SetCursorPos(ImVec2(290, 340));
				if (ImGui::Button("Clear", ImVec2(80, 45))) 
				{
					*light = {};
					light = nullptr;
				}
				// Apply button, exits edition.
				ImGui::SetCursorPos(ImVec2(290, 395));
				if (ImGui::Button("Apply", ImVec2(80, 45)))
					light = nullptr;
			}
			ImGui::End();
		}

		// If we are holding a color in the pointer run the color editor for that color.
		if (color)
		{
			if (ImGui::Begin(" Color editor", NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
			{
				ImGui::SetWindowSize(ImVec2(350, 340), ImGuiCond_Once);

				ImGui::Text("Color:");

				// First do one of the default ImGui color pickers.
				_float4color col = color->getColor4();
				// The reason why we can send &col as a float*, is because _float4 and Vector*f 
				// structs are tightly packed and can be interpreted as float*.
				ImGui::ColorPicker4("", (float*)&col);
				*color = Color(col);

				// Cancel buttor returns to the buffer and exits edition.
				ImGui::SetCursorPos(ImVec2(250, 220));
				if (ImGui::Button("Cancel", ImVec2(80, 45)))
				{
					*color = color_storage;
					color = nullptr;
				}
				// Apply button, exits edition.
				ImGui::SetCursorPos(ImVec2(250, 272));
				if (ImGui::Button("Apply", ImVec2(80, 45)))
					color = nullptr;
			}
			ImGui::End();
		}

		// If a user extension has been provided do it here.
		if (injected != nullptr)
			injected();
	}

	// Toggle to control whether the imGui is rendered ot not.
	bool visible = true;

	char title[64] = "Settings";		  // Stores the imgui window title
	Vector2i initial_size = { 315, 120 }; // Stores the imgui window initial size
protected:
	vector<int*> sliders_int;			// Stores the addresses of the floats it slides.
	vector<Vector2i> slider_ranges_int;	// Stores the ranges of the sliders displayed.
	vector<string> slider_names_int;	// Stores the names of the sliders displayed.

	vector<float*> sliders;			// Stores the addresses of the floats it slides.
	vector<Vector2f> slider_ranges;	// Stores the ranges of the sliders displayed.
	vector<string> slider_names;	// Stores the names of the sliders displayed.

	vector<int*> sel_integers;			// Stores the addresses of the enumerator integers.
	vector<Vector2i> sel_ranges;		// Stores the ranges of the selectors.
	vector<vector<string>> int_names;	// Stores the names of the selector options.
	vector<string> selector_names;		// Stores the names of the selectors.

	LightSource storage = {};		// Stores the previous light state before editing.
	LightSource* light = nullptr;	// Pointer to the currently editing lightSource.
	Color color_storage = {};		// Stores the previous color before editing.
	Color* color = nullptr;			// Pointer to the currently editing color.

	void(*injected)() = nullptr;	// Stores your injected imgui code.
};
#endif

// Windows are the main objects of this library. You need them to be able to issue
// draw calls and to hadle any events. This class is an itroduction of how a window
// child class could be built, containing an imGui manager inside, a list of drawables
// and an object draw call that draws all its drawables to the window.
// Also by defaut this window stores some perspective and window managemet resources
// taking care of updating his perspective and mode during draw calls. Window diversity
// is very much possible with this library, so if you intend to build something different
// feel free to create your own windows. This is just a nice begginer idea :)
class defaultWindow : public Window
{
public:
	// Calls the window constructor with the specified dimensions and window name.
	// If imGui is included it initializes its defaultImGui manager and it pushes
	// sliders for the perspective and a full screen selector.
	defaultWindow(Vector2i winDim, const char* name = "Chaotic Window") : Window() 
#ifdef _INCLUDE_IMGUI
		,imGui(*this) 
	{
		setTitle(name);
		setDimensions(winDim);

		imGui.pushSlider(&theta, { -2.f * MATH_PI, 2.f * MATH_PI }, "Theta");
		imGui.pushSlider(  &phi, {            0.f,       MATH_PI },   "Phi");
		imGui.pushSlider(&scale, {            1.f,        2000.f }, "Scale");

		const char* names[2] = { "Normal View (esc)", "Full Screen (F11)" };
		imGui.pushSelector("Screen Mode", { 1,2 }, &screen_mode, names);

	}
	defaultImGui imGui;
#else
	{
		setTitle(name);
	}
#endif
	// Deletes the owned drawables.
	~defaultWindow()
	{
		for (unsigned i = 0u; i < drawables.size(); i++)
			if (ownerships[i])
				delete drawables[i];
	}

	// Pushes a drawable to the internal drawable storage. This drawable will 
	// be drawn during draw calls in the window. If onw, the window will claim 
	// ownership of the drawable and destroy it upon window desctruction, 
	// otherwise the memory management is up to the user.
	Drawable* pushDrawable(Drawable* drawable, bool own = false)
	{
		if (!drawable)
			return nullptr;

		drawables.push_back(drawable);
		ownerships.push_back(own);

		return drawable;
	}

	// Templated function to return the same Drawable type that it took as input.
	// Functions the same as the original pushDrawable.
	template<typename DrawableType>
	DrawableType* pushDrawable(DrawableType* drawable, bool own = false)
	{
		return static_cast<DrawableType*>(pushDrawable(static_cast<Drawable*>(drawable), own));
	}

	// Erases the specified drawable from its internal list and returns its pointer. 
	// If delete_it it will delete the drawable and return nullptr. Else, if the 
	// drawable was owned, erasing transfers ownership back to the user.
	Drawable* eraseDrawable(unsigned idx, bool delete_it = false)
	{
		if (drawables.size() <= idx)
			return nullptr;

		Drawable* drawable = drawables[idx];

		drawables.erase(drawables.begin() + idx);
		ownerships.erase(ownerships.begin() + idx);

		if (delete_it)
		{
			delete drawable;
			return nullptr;
		}
		else return drawable;
	}

	// Returns the pointer of the specified drawable if it exists.
	Drawable* getDrawable(unsigned idx)
	{
		if (drawables.size() < idx)
			return nullptr;

		return drawables[idx];
	}

	// Does some basic event management, clears the buffer with the specified
	// color and draws by order all the drawables in its list, then pushes the 
	// frame to the window.
	void drawFrame(Color background = Color::Black)
	{
		// Set this window as the render target and clear the buffer.
		setRenderTarget();
		clearBuffer(background);

		// Draw all drawables by order.
		for (Drawable* d : drawables)
			d->Draw();

		// Push the frame to the window.
		pushFrame();

		// We process events after drawing so that what ImGui says 
		// does not get overwritten by the user. That causes our events
		// to be processed next frame but for a starter setting it does 
		// not make a difference and you won't notice it at default 60fps.
		
		// First check for screen mode updates.
		if (Keyboard::isKeyPressed(0x7A /*VK_F11*/))
			screen_mode = 2;
		if (Keyboard::isKeyPressed(0x1B /*VK_ESCAPE*/))
			screen_mode = 1;

		if (screen_mode)
		{
			setFullScreen(screen_mode == 2 ? true : false);
			screen_mode = 0;
		}

		// Then update the perspective with the up vector and the theta, phi values.
		Quaternion rotUp = (up.normal().y > -0.9999f) ? 
			Quaternion{ 1.f + up.y, -up.z, 0.f, up.x } : 
			Quaternion{ 0.f, 0.f, 0.f, 1.f };

		Quaternion rotTheta = { cosf(theta / 2.f), 0.f, sinf(theta / 2.f), 0.f };
		Quaternion rotPhi = { sinf(phi / 2.f) + cosf(phi / 2.f), sinf(phi / 2.f) - cosf(phi / 2.f), 0.f, 0.f };

		Quaternion observer = (rotPhi * rotTheta * rotUp).normal();
		setPerspective(observer, center, scale);
	}

public:
	// Stores the direction considered upwards by the window. It will adjust
	// the perspective accordingly. Due to the math rendering nature of this
	// library the z axis is considered up by default in this class.
	Vector3f up = { 0.f,0.f,1.f };
	// Stores the up axis rotation of the perspective. (left/right view)
	float theta = 0.f;
	// Stores the horizontal axis rotation of the perspective. (up/down view)
	float phi = MATH_PI / 2.f;
	// Stores tue center of the scene. Center of coordinates by default.
	Vector3f center = { 0.f,0.f,0.f };
	// Stores the scale of the scene. Scale 1 being 1distance = 1px, default
	// scale is 250 meaning 1distance = 250px.
	float scale = 250.f;
	// Event data to be used by the default event manager for this particular
	// window. For multiple window settings you can use this internal data on
	// to call the default event manager on the window that has focus.
	EventData data = { this };
protected:
	// Whether the screen mode has to be updated. (1 normal, 2 full)
	int screen_mode = 0; 

	// Descriptor to initialize the window
	WINDOW_DESC desc = {};

	// Vectors to store the drawables and their ownership status.
	vector<Drawable*> drawables;
	vector<bool> ownerships;
};
