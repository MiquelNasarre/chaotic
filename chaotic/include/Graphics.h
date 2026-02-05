#pragma once
#include "Header.h"

/* GRAPHICS OBJECT CLASS
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
This header contains the Graphics object and its functions. All windows contain their own 
graphics object which controls the GPU pointers regarding that window.

Also due to the nature of the API the class also contains the POV of the window, including 
a direction of view and a center. All drawables have acces to this buffer and the shaders 
are built accordingly. The direction is given by a quaternion and the center is given by a 
3D vector. They can be updated via class functions.

The graphics also defines two default bindables, these being the depth stencil state to be 
always DEPTH_STENCIL_MODE_DEFAULT, and the blender to be always BLEND_MODE_OPAQUE. Therefore 
if you design a drawable that does not require different settings you don't have to include 
those bindables. Every other bindable must be in every single drawable. Check bindable base
to see all the different bindables contained in this library.
-------------------------------------------------------------------------------------------------------
-------------------------------------------------------------------------------------------------------
*/

// Class to manage the global device shared by all windows and 
// graphics instances along the process.
class GlobalDevice
{
	friend class Bindable;
	friend class Graphics;
	friend class iGManager;
private:
	// Helper to delete the global device at the end of the process.
	static GlobalDevice helper;
	GlobalDevice() = default;
	~GlobalDevice();

	static inline void* globalDeviceData = nullptr; // Stores the device data masked as void*
	static inline bool skip_error = false;			// Whether to skip debug tools error
public:
	// Different GPU preferences following the IDXGIFactory6::EnumAdapterByGpuPreference
	// GPU distinction layout. 
	enum GPU_PREFERENCE
	{
		GPU_HIGH_PERFORMANCE,
		GPU_MINIMUM_POWER,
		GPU_UNSPECIFIED,
	};

	// Sets the global devica according to the GPU preference, must be set before
	// creating any window instance, otherwise it will be ignored.
	// If none is set it will automatically be created by the first window creation.
	static void set_global_device(GPU_PREFERENCE preference = GPU_HIGH_PERFORMANCE);

	// To be called by the user if it is using debug binaries to avoid D3D11 device 
	// no debug tools error message, must be called at the beggining of the code.
	static void skip_debug_tools_error() { skip_error = true; }

private:
	// Internal function that returns the ID3D11Device* masked as a void*.
	static void* get_device_ptr();

	// Internal function that returns the ID3D11DeviceContext* masked as void*
	static void* get_context_ptr();
};

// Contains the graphics information of the window, storing the GPU access pointers
// as well as the POV of the window, draw calls are funneled through this object.
class Graphics
{
	friend class Drawable;
	friend class Window;
	friend class MSGHandlePipeline;
public:
	// Before issuing any draw calls to the window, for multiple window settings 
	// this function has to be called to bind the window as the render target.
	void setRenderTarget();

	// Swaps the current frame and shows the new frame to the window.
	void pushFrame();

	// Clears the buffer with the specified color. If all buffers is false it will only clear
	// the screen color. the depth buffer and the transparency buffers will stay the same.
	void clearBuffer(Color color = Color::Black, bool all_buffers = true);

	// Clears the depth buffer so that all objects painted are moved to the back.
	// The last frame pixels are still on the render target.
	void clearDepthBuffer();

	// If OITransparency is enabled clears the two buffers related to OIT plotting.
	// IF it is not enabled it does nothing.
	void clearTransparencyBuffers();

	// Sets the perspective on the window, by changing the observer direction, 
	// the center of the POV and the scale of the object looked at.
	void setPerspective(Quaternion obs, Vector3f center, float scale);

	// Sets the observer quaternion that defines the POV on the window.
	void setObserver(Quaternion obs);

	// Sets the center of the window perspective.
	void setCenter(Vector3f center);

	// Sets the scale of the objects, defined as pixels per unit distance.
	void setScale(float scale);

	// Schedules a frame capture to be done during the next pushFrame() call. It expects 
	// a valid pointer to an Image where the capture will be stored. The image dimensions 
	// will be adjusted automatically. Pointer must be valid during next push call.
	// If ui_visible is set to false the capture will be taken before rendering imGui.
	void scheduleFrameCapture(Image* image, bool ui_visible = true);

	// To draw transparent objects this setting needs to be toggled on, it causes extra 
	// conputation due to other buffers being used for rendering, so only turn on if needed.
	// It uses the McGuire/Bavoli OIT approach. For more information you can check the 
	// original paper at: https://jcgt.org/published/0002/02/09/
	void enableTransparency();

	// Deletes the extra buffers and disables the extra steps when pushing frames.
	void disableTransparency();

	// Returns whether OITransparency is enabled on this Graphics object.
	bool isTransparencyEnabled() const;

	// Returns the current observer quaternion.
	inline Quaternion getObserver() const { return cbuff.observer; }

	// Returns the current Center POV.
	inline Vector3f getCenter() const { return Vector3f(cbuff.center); }

	// Returns the current scals.
	inline float getScale() const { return Scale; }

private:
	// Initializes the class data and calls the creation of the graphics instance.
	// Initializes all the necessary GPU data to be able to render the graphics objects.
	Graphics(void* hWnd);

	// Destroys the class data and frees the pointers to the graphics instance.
	~Graphics();

	// To be called by drawable objects during their draw calls, issues an indexed 
	// draw call drawing the object to the render target., If the object is transparent 
	// redirects it to the accumulation targets for later composing. At the end returns 
	// to default Blender and DepthStencil states.
	static void drawIndexed(unsigned IndexCount, bool isOIT);

	// To be called by its window. When window dimensions are updated it reshapes its
	// buffers to match the new window dimensions, as specified by the vector.
	void setWindowDimensions(const Vector2i Dim);

	// No copies of a graphics instance are allowed.
	Graphics(Graphics&&)					= delete;
	Graphics& operator=(Graphics&&)			= delete;
	Graphics(const Graphics&)				= delete;
	Graphics& operator=(const Graphics&)	= delete;

private:
	void* GraphicsData = nullptr; // Stores the graphics object internal data.

	static inline Graphics* currentRenderTarget = nullptr; // Stores the pointer to the current graphics target.

	struct // Constant buffer of the current graphics perspective, accessable to all vertex shaders.
	{
		Quaternion observer = 1.f;	// Stores the current observer direction.
		_float4vector center = {};	// Stores the current center of the POV.
		_float4vector scaling = {};	// Scaling values for the shader
	}cbuff;		

	Vector2i WindowDim;			// Stores the current window dimensions.
	float Scale = 250.f;		// Stores the current view scale.
};
