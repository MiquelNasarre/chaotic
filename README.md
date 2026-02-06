# Chaotic

Chaotic is a **fully self-contained, bloat-free 3D rendering library for C++**, designed for interactive
mathematical plotting. It provides `Window` objects as your canvas, and flexible `Drawable` classes to 
design your plots, giving you full control over how to visualize your functions.

This is a library built for **mathematicians, scientists, engineers and programmers** who want to use C++ 
to create cool and interactive plots in real time. Often, plotting software in this language is not very 
well-known and is quite limited, which ends up constraining the kind of plots we want to create. Chaotic 
intends to break away from all that. 

In the background, it takes care of all *Win32* and *DirectX11* details. At the API level, it gives you flexible 
tools to build plots **from your own functions**, while always trying to minimize overhead and code complexity.

Chaotic uses [ImGui](https://github.com/ocornut/imgui.git) for the widgets on screen, integrating 
it naturally into the workflow of the library. This allows fine-tuning and enhanced visualization, naturally 
enabling variable sliders, selectors, color pickers, and all other tools provided by ImGui.

Some of the features included in the library are:

* **Multi-window** setups with minimal code complexity.
* Built-in `Drawable` classes: `Surface`, `Polyhedron`, `Background`, `Curve`, `Scatter` and `Light`.
* **ImGui** naturally merged into the library classes via the `iGManager` class.
* Weighted blended **Order-Independent Transparency (OIT)**.
* Support for **custom drawables and shaders** without writing *Win32* and *DirectX11* code.
* Dedicated `Image` and `Color` classes, for **image file handling**, and **plot coloring**.
* Built-in **math library**, for vector manipulation, **quaternion rotations**, and matrix transformations.
* Multiple `Surface` types: `EXPLICIT`, `SPHERICAL`, `PARAMETRIC` and `IMPLICIT`.
* Simple `Keyboard` and `Mouse` support classes for **defining user interaction**.
* **Texturing** support for `Surface`, `Polyhedron` and `Background` objects.
* **Wallpaper windows**, making your plots appear directly on your desktop background!
* Support for **OBJ triangle meshes**. 

Chaotic is not built to be a general-purpose 3D renderer or a game engine, instead it's a library specialized for 
plots. For that reason most drawables use exclusively orthographic projection.

## Requirements

This library is built using *Win32* and *DirectX11* tools, therefore it is only available for *Windows* systems 
at the moment. It targets *ISO C++17* and later. It's built with Visual Studio, precompiled binary files are provided 
in the [release](https://github.com/MiquelNasarre/chaotic/releases).

To use the library, you will need the precompiled library files `chaotic.lib`/`chaotic_d.lib` or your own 
compilation, as well as at least the main library header, `chaotic.h`. Some other headers are also provided 
as we will discuss later.

If you want to use **ImGui**, which is enabled by default, you will need to link the `imgui.h` header
file (also provided). 

For immediate implementation:

* Download [latest release](https://github.com/MiquelNasarre/chaotic/releases).
* Add the include path or copy headers into your project (`imgui.h` included).
* Link `chaotic.lib` for release and optionally `chaotic_d.lib` for debug.
* `#include "chaotic_defaults.h"` and run `chaotic_demo()`!

## Demo

Calling the `chaotic_demo()` function, found inside `chaotic_defaults.h` will automatically run the library demo. 
It consists of a multi-window setting, starting from a single introductory one, and letting the user navigate 
through different kinds of plots. It showcases the library's capabilities while covering some interesting mathematical 
topics. I hope you have fun with it!

---

<img width="2004" height="1214" alt="chaotic_demo_screenshot" src="https://github.com/user-attachments/assets/45ee6937-ccec-49e5-8fad-1b84f846df1c" />

---

This image shows some of the demo windows. The code makes use of the `chaotic.h` and `chaotic_defaults.h` 
functions and classes. Its source code can be found at `chaotic/source/chaotic_demo.cpp`.

## Getting Started

The public API is split into four headers:

* `chaotic.h`: This is the main header of the library. It contains all the high-level API classes and functions.
  This header is self-sufficient. It lets you use the library at full capacity with no customization.
* `chaotic_defaults.h`: Contains some helper classes and functions; `defaultWindow`, `defaultImGui`,
  `defaultEventManager()` and `chaotic_demo()`. It’s intended to make the library easier to adopt for new users.
* `chaotic_customs.h`: If the drawables provided by the library are not sufficient or do not fit the
  implementation you are thinking of, this header gives you the tools to create your own drawable classes
  and shaders, all while staying inside the library abstraction, without the need to write *Win32* or *DirectX11* code.
* `chaotic_internals.h`: Contains all the internal libraries used to create the *Win32* and *DirectX11* dependent functions
  and some additional error classes used internally.

As mentioned, to get started, `chaotic_defaults.h` provides some basic implementations of the library classes. 
For example, the following code plots a simple surface that is interactive with the mouse, with default lighting on a single window.

``` cpp
#include "chaotic_defaults.h"

void __stdcall WinMain() // Or main() depending on your sub-system
{
    // All drawables have their own descriptor with multiple
    // settings for initialization. In this case we define a Surface.
    SURFACE_DESC desc = {};
    desc.type = SURFACE_DESC::EXPLICIT_SURFACE; // Default; other options available.
    desc.explicit_func = [](float x, float y) { return cosf(10.f * (x * x + y * y)) / 5.f; };

    // Create the Surface object with the provided descriptor.
    Surface surf(&desc);

    // Creates a default window with the given dimensions. The window appears 
    // immediately and will persist until the object is deleted. Simple.
    defaultWindow window({ 720,480 });

    // Pushes the drawable into the defaultWindow list.
    // (chaotic_defaults.h simplicity addition).
    window.pushDrawable(&surf);

    // Loop until your window ID is returned. This happens when
    // the close button is pressed on your window.
    while (Window::processEvents() != window.getID())
    {
        // Default event manager uses the Keyboard and Mouse functions to update the data.
        defaultEventManager(window.data);
        surf.updateRotation(window.data.rot_free);
        window.scale = window.data.scale;

        // Wrapper function of defaultWindow that takes care of the rendering.
        window.drawFrame();
	}
}
```

---

<img width="1000" height="512" alt="chaotic_window_screenshot" src="https://github.com/user-attachments/assets/86a57524-9455-47f2-b32d-5937846f73f0" />

---

This is just a very simple example, but given the library flexibility your imagination is the limit! 
To learn how to use the different drawables and functions consider reading the main header file `chaotic.h`, 
all classes and functions have explanatory comments and are built to be intuitively used. 

## Main Idea

The classes found in `chaotic.h` can be split into four different types: 

* `Window`: Standalone class that defines all the windows in the library.
* `Drawable` classes: These are all the objects that can be drawn to a `Window`.
* **User Interface**: This includes `Keyboard`, `Mouse` and `iGManager`.
* **Tools**: All other classes; math library, `Image`, `Color`, `Timer` and `ChaoticError`.

The simplest way to understand the library is: We create windows, we create drawables to be drawn to those windows, 
and we use the UI classes to define interactions. Different tools are used in every step. 

The only required function call is `Window::processEvents()`, it is essential for bookkeeping, sends events
to the `Keyboard` and `Mouse`, runs the *Windows* message pump and controls the framerate, so it must be called 
inside your loop, even if you don't rely on it for closing your windows.

The main philosophy of this library is user flexibility, with the intent to give as much freedom as possible in the 
way plots are created. This implies the following:

* `Window` and `Drawable` objects can be created and destroyed at any time and are independent from each other.
* The close button does not enforce a window to close, instead it sends a message through `Window::processEvents()`
  with the corresponding `Window` ID. Windows appear and disappear via the constructor and destructor.
* Draw calls will always draw to whichever `Window` is the current render target, and can always be issued as long
  as there is a target.
* The same object can be drawn multiple times on the same `Window`, while being modified between draw calls.
* The same object can be drawn on different `Window` objects, with different views and states.
* Clearing buffers between draw calls is not enforced, and can be called at any point during runtime or even never.

All this flexibility can be quite overwhelming for a new user though, that is why `chaotic_defaults.h` exists. 
You can take the code example of the previous section as a template for single window settings. For a multi-window 
setting I recommend checking the `demoWindow` and `chaotic_demo()` code, found inside `chaotic_demo.cpp`, 
for minimal correct implementation.

## ImGui

Widgets are essential for real interaction with plots; the ability to modify variables with a slider, select options 
from a list, modify the coloring in real time, etc. **ImGui** naturally provides widgets with all those functionalities 
and much more!

This version of Chaotic runs with [ImGui v1.92.5](https://github.com/ocornut/imgui/releases/tag/v1.92.5). There is a 
large community of developers that uses this tool and lots of resources online on how to use it. You can check its 
repository at: <https://github.com/ocornut/imgui.git>.

**ImGui** is implemented in this library via the virtual class `iGManager`, you can create your own inherited classes 
and override the `render()` function with your **ImGui** code. Each instance of the class creates its own **ImGui** 
context, and they can be bound and unbound from `Window` objects, allowing for simple multi-window implementation.

The following code creates one of those classes, binds it to a window and renders the **ImGui** demo.

``` cpp
#include "chaotic.h"
#include "imgui.h"

// Create the iGManager inherited class
class MyImGui : public iGManager
{
public:
    // Define the constructor to bind directly to a window.
    MyImGui(Window& w) : iGManager(w) {}

    // Override virtual render() function. Call ImGui demo.
    void render() override { ImGui::ShowDemoWindow(); }
};

void __stdcall WinMain() // Or main() depending on your sub-system
{
    // Create a window descriptor and initialize your window.
    WINDOW_DESC desc = { "ImGui Demo Window" };
    desc.window_dim = Vector2i{ 1080, 800 };
    Window window(&desc);

    // Create the ImGui object and bind it to your window.
    MyImGui imgui(window);

    // Run a usual process events loop.
    while (Window::processEvents() != window.getID())
    {
        // Set render target and clear buffers.
        window.setRenderTarget(), window.clearBuffer();
        // Push frame. Since the iGManager is bound to the window, render()
        // is called automatically, and ImGui will take care of the rest.
        window.pushFrame();
    }
}
```

This code should give a good idea of how to implement **ImGui** into this library. Nevertheless, I recommend 
checking the `iGManager` class declaration to get familiar with the details.

Alternatively you can use the `defaultImGui`, which is included by default inside the `defaultWindow`, this does 
not give full ImGui customization capabilities but allows you to add sliders and selectors without worrying about 
the ImGui code itself. For example it is the one used inside `chaotic_demo()`.

## Custom Drawables

The default drawables provided in this library allow for a lot of flexibility in the way you define your plots, but 
sometimes what you want to plot does not exactly fit inside the default `Drawable` classes, or maybe it requires too 
much compute to run on the CPU and you would like to define it directly inside the shaders. 

Chaotic is well aware of these situations, in my case I wanted to plot an infinite Mandelbrot set, and despite being 
able to do it in theory with the `Image` and `Background` classes, the amount of compute required for detail far 
surpassed what my CPU is able to compute in real time. Therefore I created a custom drawable called `Mandelbrot`, which 
contained the set formulation inside the pixel shader itself, allowing for really cool plots in real time.

---

<img width="1919" height="1033" alt="mandelbrot_screenshot" src="https://github.com/user-attachments/assets/961a891a-7e45-4912-9108-f6bb4e2c38ce" />

---

The library facilitates this kind of customization through the `chaotic_customs.h` header, this provides access to 
the classes needed to create drawables, which are the bindables, and also to the embedded default shaders for release 
mode.

The bindables are the different kinds of pointers to the GPU that are held and are bound during a draw call, like the 
vertex and index buffers, the shaders, the textures, etc. It is important to become familiar with the default graphics 
pipeline to create customized drawables. But in general drawables are just holders to GPU pointers, that bind those 
pointers during a draw call.

You can read some of the default Drawables to understand how they work and how to create one, I suggest 
`Scatter`, `Light` and `Curve`, since they are the simplest ones and focus mostly on defining and updating the 
bindables.

## Image and Texturing

If you are planning to use textures, take screenshots, or design backgrounds the `Image` class will be essential 
for those. This class relies on the `Color` struct, which is a `B8G8R8A8` class with some basic operations. 
Pixels inside an image are stored as a packed array of colors, you can access them and modify them as you please. 

You can create images of any size and save them to your computer. This is done via the `load()/save()` functions,
these functions currently only support uncompressed bitmap images, since no additional image dependencies are used.

There are many software options to change image formats, but the best tool I have found so far and I strongly 
recommend for this and any other image related issues is [ImageMagick](https://imagemagick.org/), simply input 
into a console prompt:

```cmd
> magick initial_image.png -compress none image.bmp
```

and you have any image of yours converted directly into an uncompressed bitmap, which is remarkably convenient, 
and I strongly suggest checking them out.

Once you have easy access to your images you can use them for texturing `Surface`, `Polyhedron` and `Background` 
objects, and you can also use them to make frame captures in real time. 

For dynamic backgrounds and spherical surfaces, texture cubes are used instead, these are images that represent a cube
wrapped around a sphere and are really easy for computer graphics to map to spherical coordinates. Since those 
projections are not especially common there is a quality-of-life helper called `ToCube` inside the `Image` header that 
allows for conversion of common 360° image formats to texture cubes, these being equirectangular images and fisheye 
images.

As an example of the capabilities, the following code takes an equirectangular image from your executable path, wraps 
it around a sphere, takes a screenshot and saves it.

``` cpp
#include "chaotic_defaults.h"

void __stdcall WinMain() // Or main() depending on your sub-system
{
	// Create empty images.
	Image image, screenshot, *texture_cube;

	// Try to load your equirectangular projection.
	if (!image.load("equirect.bmp"))
		USER_ERROR("Could not load equirect.bmp");

	// Turn this image into a texture cube.
	constexpr unsigned cube_width = 1500u;
	texture_cube = ToCube::from_equirect(image, cube_width);

	// Create a spherical surface with the texture for coloring.
	SURFACE_DESC desc = {};
	desc.type = SURFACE_DESC::SPHERICAL_SURFACE;
	desc.spherical_func = [](float, float, float) { return 2.f; };
	desc.enable_illuminated = false;
	desc.coloring = SURFACE_DESC::TEXTURED_COLORING;
	desc.texture_image = texture_cube;

	// Initialize the surface.
	Surface surf(&desc);

	// Create a default window and push the surface.
	defaultWindow window(Vector2i{ 1080,720 }, "Spherical Image");
	window.pushDrawable(&surf);

	// Loop until your window ID is returned.
	while (Window::processEvents() != window.getID())
	{
		// Default event management.
		defaultEventManager(window.data);
		surf.updateRotation(window.data.rot_free);
		window.scale = window.data.scale;

		// If letter S is pressed take a screenshot next frame.
		while (char c = Keyboard::popChar())
			if (c == 's' || c == 'S')
				window.scheduleFrameCapture(&screenshot, false);

		// Draw next frame.
		window.drawFrame();
	}
	// Save screenshot and delete texture cube.
	screenshot.save("spherical_screenshot.bmp");
	delete texture_cube;
}
```

To run this code I used the image on the left, obtained from <https://polyhaven.com/es/a/rogland_clear_night>, 
and got the screenshot on the right:

---

<img width="1536" height="512" alt="composite" src="https://github.com/user-attachments/assets/3f0cda07-e0f0-425a-b916-8a26669fa348" />

---

## Transparencies

`Surface`, `Polyhedron`, `Curve` and `Scatter` objects all support transparency plots. Given the nature of the 
library it is important to define proper transparent objects, and it is also important that those transparencies
are order-independent, given that objects can intersect with each other and even with themselves.

The implementation in this library follows the approach described by Morgan McGuire and Louis Bavoil 
in their article [Weighted Blended Order-Independent Transparency](https://jcgt.org/published/0002/02/09/).

The core idea behind the article is to accumulate transparent objects on a separate target weighted by their alpha
value and their relative distance to the observer. Right before presenting the frame, the separate target is combined 
with the actual render target to produce the resolved image.

This idea produces credible transparent objects and is well behaved, but also requires a lot of extra compute, as it 
needs two additional render targets. Therefore, this option is disabled by default on all windows, and the extra buffers 
will only be created as per the user's request.

To draw a transparent object, on its descriptor you must set `enable_transparency` to true and color it with an alpha
value smaller than 255. To draw transparent objects to a window first you must call the method `Window::enableTransparency()` 
on that specific window.

The following screenshot is taken from the Hopf Fibration Wallpaper window on the `chaotic_demo()` function, with a 
small alpha value and 1000 fibers per circle.

<img width="1920" height="1080" alt="hopf_wallpaper_screenshot" src="https://github.com/user-attachments/assets/c3156cd2-d403-48d0-acb2-5551f6c0959e" />

## Wallpaper Windows

As mentioned previously, and showcased by the *Hopf Fibration Wallpaper* inside the `chaotic_demo()`, one of the 
features of this library is to create windows that bind directly to the desktop background, essentially acting 
as controllable wallpaper plots.

The creation of wallpaper windows is extremely simple. They are still regular windows but in the descriptor you 
can change the `window_mode` to `WINDOW_DESC::WINDOW_MODE_WALLPAPER` and that automatically triggers the window
to bind to the desktop background. Some internal window functions are specially defined for wallpapers, for example
`Window::setWallpaperMonitor()` that allows you to choose between different monitor settings.

As an example, the following code creates a Wallpaper window and uses a timer to make it persist for 20 seconds while 
drawing a simple moving curve in your desktop background:

``` cpp
#include "chaotic.h"
#include <cmath> // for sinf()/cosf().

void __stdcall WinMain() // Or main() depending on your sub-system
{
    // We create the Wallpaper window descriptor. Setting the mode 
    // to wallpaper automatically creates it in our desktop background.
    WINDOW_DESC wal_desc = {"Our Wallpaper!"};
    wal_desc.window_mode = WINDOW_DESC::WINDOW_MODE_WALLPAPER; 
    wal_desc.wallpaper_persist = false; // Default

    // Now we create a Window as wallpaper.
    Window wallpaper(&wal_desc);

    // Let's describe a parametric Curve. A spiral around the sphere.
    CURVE_DESC curve_desc = {};
    curve_desc.curve_function = [](float t) { return 3.f * Vector3f(cosf(t), cosf(t / 2.f * MATH_PI), sinf(t)); };
    curve_desc.enable_updates = true; // We do not define the initial range because we will update it anyway.

    // Create the Curve object with the provided descriptor.
    Curve curve(&curve_desc);

    // We will use a timer to update the function. (starts at 0s on creation).
    Timer timer;

    // Loop until time reaches 20 seconds. We do this because wallpapers do not get focus,
    // therefore we need an external way of interaction. Usually a separate window or console.
    for (float t = timer.check(); (t = timer.check()) < 20.f;)
    {
        // Important to do this for book-keeping and framerate control.
        Window::processEvents();

        // Recompute the curve with new range.
        curve.updateRange(Vector2f{ t, t + 2.f * MATH_PI});

        // Set the wallpaper window as the render target and clear the buffer.
        wallpaper.setRenderTarget(), wallpaper.clearBuffer();

        // Draw the curve onto the current target.
        curve.Draw();
        
        // Push new frame.
        wallpaper.pushFrame();
    }
}
```

There are two important notes with wallpaper windows though:

* Wallpaper windows do not get focus, and there is no clearly defined way of closing them, so additional
  measures need to be taken to ensure the ability to interact with the window. A console or an additional
  window seem like the most appropriate choices.

  In the `chaotic_demo()` code for example, the `Hopf Fibration Wallpaper` is contained inside the helper window, 
  and any interactions are defined through it. When the helper window is closed, the destructor is called, therefore 
  the wallpaper disappears too.

* The method used to create a window behind your desktop icons is not official and relies on undocumented 
  *Windows* behavior. Therefore it is not guaranteed to work in all system configurations.

  If the constructor cannot get access to the window that controls your desktop background, it will fall back
  to creating a regular window instead, and pop up an information message box.

## Limitations

Chaotic is optimized for interactive mathematical plotting, so it makes a few trade-offs. 
The following list covers some of the limitations you might find:

* Single platform library:

  Since this library is built using *Win32* and *DirectX11*, it is only available for *Windows*.
  A multi-platform graphics API like *Vulkan* is a possible future direction, but not planned yet.

* All drawables besides the dynamic `Background` follow an orthographic / plot-style projection:
  
  Most drawables use orthographic projection to keep scale/axes consistent for plotting. No perspective
  view is implemented by default. Nevertheless, it still supports observer direction and center, and object
  rotations given by quaternions. A global toggle for perspective view may be implemented in the future.

* Functions that rely on *DirectX11* are not thread-safe:
  
  The library initializes a global D3D11 device and device context. All objects use the same context for
  different functions, so the internals of the API are not thread-safe.

  This includes all `Drawable` and `Window` functions. Other running tasks like general code and library
  tools can be used in a multi-thread setting with no issue.

## Error Handling

To facilitate debugging, Chaotic by default performs multiple checks during function calls, and generates errors
if conditions are not met. All errors should be considered fatal. There are two distinct types of errors handled 
by the library:

* User Errors: If a pointer that was supposed to hold an array is passed `nullptr` or a `Draw()` function is
  called before the creation of any window, etc. These are considered user errors, when they are detected, a
  message string explaining the error is generated.

* System Errors: When *Win32* and *DirectX11* functions return failed results or error messages are found, some
  diagnostic tools are run to find information about the error and a string is generated.

All errors share a base class called `ChaoticError`, you can find it in `chaotic.h`. And all error findings
are funneled through the `CHAOTIC_FATAL` macro, which by default pops a message box displaying the error 
information, when the message is closed the process is terminated. The following image shows some example errors.

---

<img width="1126" height="328" alt="error_messages" src="https://github.com/user-attachments/assets/89952d34-79e0-4f0b-afbd-f6b276334d1b" />

---

**ImGui** assertions are also funneled through this centralized error path, the added code can be found at 
the beginning of `imgui.h`, and the source code at `source/Error/ChaoticError.cpp`.

Checks are performed in both debug and release mode, due to their almost negligible overhead and their really 
valuable diagnostics. However this can be disabled by the user, both `CHAOTIC_FATAL` and `CHAOTIC_CHECK` can 
be customized or set to zero at library compile time, with no additional modifications needed and checks will 
be skipped.

The `USER_ERROR` and `USER_CHECK` macros are available to the user for their own internal debugging, like the 
use case on the image and texture section code above. They are used for all user errors across the library, see 
their definitions for more detail.

## Contact

For library and code related issues/bugs the issues tab can be used. For suggestions, code extensions, 
and other inquiries you can contact me via email at:

* [miguel.nasarre.budino@gmail.com](mailto:miguel.nasarre.budino@gmail.com)

This library is an important project for me, I put a lot of effort into making it accessible and fun! 
And I hope it can reach other people that might find it useful. :)

## License

Chaotic is released under the MIT License.
See the LICENSE file for details.

- Dear ImGui: Dear ImGui is licensed under the MIT License.

