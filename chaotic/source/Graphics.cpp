#include "Bindable/BindableBase.h"
#include "iGManager.h"

#include "WinHeader.h"

#ifdef _DEPLOYMENT
#include "embedded_resources.h"
#endif

// Uncomment to use the Graphics Debugger.
//#define GRAPHICS_DEBUGGING

/*
-----------------------------------------------------------------------------------------------------------
 Global Device Functions
-----------------------------------------------------------------------------------------------------------
*/

// This struct contains the data for the global device.
struct DEVICE_DATA
{
	ComPtr<ID3D11Device>		pDevice;
	ComPtr<ID3D11DeviceContext>	pContext;
};

// Helper to delete the global device at the end of the process.
GlobalDevice GlobalDevice::helper;
GlobalDevice::~GlobalDevice()
{
	if (globalDeviceData)
		delete (DEVICE_DATA*)globalDeviceData;
}

// Sets the global devica according to the GPU preference, must be set before
// creating any window instance, otherwise it will be ignored.
// If none is set it will automatically be created by the first window creation.

void GlobalDevice::set_global_device(GPU_PREFERENCE preference)
{
	if (globalDeviceData)
		return;

	globalDeviceData = new DEVICE_DATA;
	DEVICE_DATA& data = *(DEVICE_DATA*)globalDeviceData;

	//  Create Factory

	UINT factoryFlags = 0u;
#ifdef _DEBUG
	factoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ComPtr<IDXGIFactory> dxgiFactory;
	GRAPHICS_HR_CHECK(CreateDXGIFactory2(factoryFlags, __uuidof(IDXGIFactory), &dxgiFactory));

	// Create device based on preference

	UINT createFlags = 0u;
#ifdef _DEBUG
	createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

#ifndef GRAPHICS_DEBUGGING
	//  Find adapter by GPU preference
	ComPtr<IDXGIAdapter> bestAdapter = nullptr;

	ComPtr<IDXGIFactory6> Factory6;
	GRAPHICS_HR_CHECK(dxgiFactory.As(&Factory6));

	switch (preference)
	{
	case GPU_HIGH_PERFORMANCE:
		GRAPHICS_HR_CHECK(Factory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&bestAdapter)));
		break;
	case GPU_MINIMUM_POWER:
		GRAPHICS_HR_CHECK(Factory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_MINIMUM_POWER, IID_PPV_ARGS(&bestAdapter)));
		break;
	case GPU_UNSPECIFIED:
		GRAPHICS_HR_CHECK(Factory6->EnumAdapterByGpuPreference(0, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&bestAdapter)));
		break;
	}

	//  Create D3D11 device and context with the chosen adapter
	GRAPHICS_HR_CHECK(D3D11CreateDevice(
		bestAdapter.Get(),
		D3D_DRIVER_TYPE_UNKNOWN,
		nullptr,
		createFlags,
		nullptr,
		0u,
		D3D11_SDK_VERSION,
		data.pDevice.GetAddressOf(),
		nullptr,
		data.pContext.GetAddressOf()
	));
#else
	// Use the debuggers choice
	GRAPHICS_HR_CHECK(D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createFlags,
		nullptr,
		0u,
		D3D11_SDK_VERSION,
		data.pDevice.GetAddressOf(),
		nullptr,
		data.pContext.GetAddressOf()
	));
#endif
}

// Internal function that returns the ID3D11Device* masked as a void*.

void* GlobalDevice::get_device_ptr()
{
	DEVICE_DATA& data = *(DEVICE_DATA*)globalDeviceData;

	return data.pDevice.Get();
}

// Internal function that returns the ID3D11DeviceContext* masked as void*

void* GlobalDevice::get_context_ptr()
{
	DEVICE_DATA& data = *(DEVICE_DATA*)globalDeviceData;

	return data.pContext.Get();
}

// Mini helpers to make the code more readable
#define _device	 ((ID3D11Device*)GlobalDevice::get_device_ptr())
#define _context ((ID3D11DeviceContext*)GlobalDevice::get_context_ptr())

/*
-----------------------------------------------------------------------------------------------------------
 OIT Implementation Functions
-----------------------------------------------------------------------------------------------------------
*/

// This struct contains all the necessary GPU pointers for the OIT pass
// and is initialized inside Graphics internals when OIT is enabled.
struct OITransparencyInternals
{
	// OIT render targets: accumulation + revealage
	ComPtr<ID3D11Texture2D>         pOITAccumTex;	// Accumulation texture
	ComPtr<ID3D11RenderTargetView>  pOITAccumRTV;	// To set the texture as Target
	ComPtr<ID3D11ShaderResourceView>pOITAccumSRV;	// To set the texture as Resource

	ComPtr<ID3D11Texture2D>         pOITRevealTex;	// Revealage texture
	ComPtr<ID3D11RenderTargetView>  pOITRevealRTV;	// To set the texture as Target
	ComPtr<ID3D11ShaderResourceView>pOITRevealSRV;	// To set the texture as Resource

	// OIT pipeline states
	ComPtr<ID3D11BlendState>        pOITBlendState;        // accumulation pass (2 MRTs)
	ComPtr<ID3D11BlendState>        pOITResolveBlendState; // fullscreen composite
	ComPtr<ID3D11DepthStencilState> pOITDepthReadOnly;     // depth test on, writes off

	// Bindables used for the final draw blending call
	Bindable* resolveBindables[7] = { nullptr };
};

/*
-----------------------------------------------------------------------------------------------------------
 Graphics class Internal Functions
-----------------------------------------------------------------------------------------------------------
*/

// This structure contains all the data needed for the graphics
// obnect to render that is external for the user.
struct GraphicsInternals
{
	HWND HWnd = nullptr;

	ComPtr<IDXGISwapChain>			pSwap = {};
	ComPtr<ID3D11RenderTargetView>	pTarget = {};
	ComPtr<ID3D11DepthStencilView>	pDSV = {};
	
	ConstantBuffer* Perspective = nullptr;
	DepthStencil* defaultDephtStencil = nullptr;
	Blender* defaultBlender = nullptr;

	bool oitEnabled = false;
	OITransparencyInternals* OIT = nullptr;

	Image* captureImage = nullptr;
	bool capture_ui_visible = false;
	ComPtr<ID3D11Texture2D> pCaptureStaging = {};
};

// Initializes the class data and calls the creation of the graphics instance.
// Initializes all the necessary GPU data to be able to render the graphics objects.

Graphics::Graphics(void* hWnd)
{
	GlobalDevice::set_global_device();

	GraphicsData = new GraphicsInternals;
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	data.HWnd = (HWND)hWnd;
	DXGI_SWAP_CHAIN_DESC sd = {};
	sd.BufferDesc.Width = 0u;
	sd.BufferDesc.Height = 0u;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0u;
	sd.BufferDesc.RefreshRate.Denominator = 0u;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.SampleDesc.Count = 1u;
	sd.SampleDesc.Quality = 0u;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 1u;
	sd.OutputWindow = (HWND)hWnd;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;

	ComPtr<IDXGIFactory> dxgiFactory;
	GRAPHICS_HR_CHECK(CreateDXGIFactory(__uuidof(IDXGIFactory), &dxgiFactory));

	//  Create swap chain
	GRAPHICS_HR_CHECK(dxgiFactory->CreateSwapChain(_device, &sd, data.pSwap.GetAddressOf()));

	//	Gain access to render target through shinnanigins
	ComPtr<ID3D11Resource> pBackBuffer;
	GRAPHICS_HR_CHECK(data.pSwap->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer));
	GRAPHICS_HR_CHECK(_device->CreateRenderTargetView(pBackBuffer.Get(), nullptr, data.pTarget.GetAddressOf()));

	//	Create Perspective constant buffer
	data.Perspective = new ConstantBuffer(&cbuff, VERTEX_CONSTANT_BUFFER, 0u);

	// Create default bindables
	data.defaultBlender = new Blender(BLEND_MODE_OPAQUE);
	data.defaultDephtStencil = new DepthStencil(DEPTH_STENCIL_MODE_DEFAULT);

	// If someone was the render target reset to it.
	if (currentRenderTarget) currentRenderTarget->setRenderTarget();
	// Else you are the render target.
	else setRenderTarget();
}

// Destroys the class data and frees the pointers to the graphics instance.

Graphics::~Graphics()
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	// If you are the current render target, you are no longer
	if (currentRenderTarget == this)
		currentRenderTarget = nullptr;

	// Disable OIT if enabled.
	if (data.oitEnabled)
		disableTransparency();

	// Delete the perspective Constant Buffer
	delete data.Perspective;

	// Delete default bindables
	delete data.defaultDephtStencil;
	delete data.defaultBlender;

	// Delete the data
	delete &data;
}

// To be called by drawable objects during their draw calls, issues an indexed 
// draw call drawing the object to the render target., If the object is transparent 
// redirects it to the accumulation targets for later composing. At the end returns 
// to default Blender and DepthStencil states.

void Graphics::drawIndexed(unsigned IndexCount, bool isOIT)
{
	USER_CHECK(currentRenderTarget,
		"Trying to issue a draw call when no render target has been assigned. \n"
		"Create a window object or call setRenderTarget on your desired window object before calling Drawable::Draw()"
	);

	GraphicsInternals& data = *((GraphicsInternals*)currentRenderTarget->GraphicsData);

	if (isOIT)
	{
		USER_CHECK(data.oitEnabled,
			"Trying to draw a Transparency Drawable on a Window that does not support it.\n"
			"If you want to draw transparent objects on a window you first have to call Graphics::enableTransparency()"
		);

		OITransparencyInternals& oit = *data.OIT;

		// 1) Bind OIT MRTs + depth
		ID3D11RenderTargetView* rtvs[2] =
		{
			oit.pOITAccumRTV.Get(),
			oit.pOITRevealRTV.Get()
		};
		GRAPHICS_INFO_CHECK(_context->OMSetRenderTargets(2u, rtvs, data.pDSV.Get()));

		// 2) Depth read-only + OIT blend state
		GRAPHICS_INFO_CHECK(_context->OMSetDepthStencilState(oit.pOITDepthReadOnly.Get(), 0u));
		GRAPHICS_INFO_CHECK(_context->OMSetBlendState(oit.pOITBlendState.Get(), nullptr, 0xFFFFFFFFu));

		// 3) Draw into OIT buffers
		GRAPHICS_INFO_CHECK(_context->DrawIndexed(IndexCount, 0u, 0u));

		// 4) Restore backbuffer as RT and default depth/blend state
		GRAPHICS_INFO_CHECK(_context->OMSetRenderTargets(1u, data.pTarget.GetAddressOf(), data.pDSV.Get()));
	}

	else GRAPHICS_INFO_CHECK(_context->DrawIndexed(IndexCount, 0u, 0u));

	// Back to default Depth Stencil State and Blender.
	data.defaultDephtStencil->Bind();
	data.defaultBlender->Bind();
}

// To be called by its window. When window dimensions are updated it reshapes its
// buffers to match the new window dimensions, as specified by the vector.

void Graphics::setWindowDimensions(const Vector2i Dim)
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	WindowDim = Dim;

	if (!Dim.x || !Dim.y)
		return;

	// Release references to old buffers.
	data.pTarget.Reset();
	data.pCaptureStaging.Reset(); // Will be recreated on next capture call

	// Preserve the existing buffer count and format.
	// Automatically choose the width and height to match the client rect for HWNDs.

	GRAPHICS_HR_CHECK(data.pSwap->ResizeBuffers(0u, (UINT)Dim.x, (UINT)Dim.y, DXGI_FORMAT_UNKNOWN, 0u));

	// Get buffer and create a render-target-view.

	ComPtr<ID3D11Resource> pBackBuffer;
	GRAPHICS_HR_CHECK(data.pSwap->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer));
	GRAPHICS_HR_CHECK(_device->CreateRenderTargetView(pBackBuffer.Get(), NULL, data.pTarget.GetAddressOf()));

	//	Create depth stencil texture

	ComPtr<ID3D11Texture2D> pDepthStencil;
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = (UINT)Dim.x;
	descDepth.Height = (UINT)Dim.y;
	descDepth.MipLevels = 1u;
	descDepth.ArraySize = 1u;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1u;
	descDepth.SampleDesc.Quality = 0u;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	GRAPHICS_HR_CHECK(_device->CreateTexture2D(&descDepth, NULL, &pDepthStencil));

	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0u;
	GRAPHICS_HR_CHECK(_device->CreateDepthStencilView(pDepthStencil.Get(), &descDSV, data.pDSV.GetAddressOf()));

	//	Update perspective to match scaling

	cbuff.scaling = { Scale / Dim.x, Scale / Dim.y, Scale, 0.f };

	data.Perspective->update(&cbuff);

	clearDepthBuffer();

	// If OIT is enabled resize its buffers as well
	if (data.oitEnabled)
	{
		OITransparencyInternals& oit = *data.OIT;

		// Release old RTs
		oit.pOITAccumTex.Reset();
		oit.pOITAccumRTV.Reset();
		oit.pOITAccumSRV.Reset();
		oit.pOITRevealTex.Reset();
		oit.pOITRevealRTV.Reset();
		oit.pOITRevealSRV.Reset();

		D3D11_TEXTURE2D_DESC accumDesc = {};
		accumDesc.Width = (UINT)Dim.x;
		accumDesc.Height = (UINT)Dim.y;
		accumDesc.MipLevels = 1u;
		accumDesc.ArraySize = 1u;
		accumDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		accumDesc.SampleDesc.Count = 1u;
		accumDesc.SampleDesc.Quality = 0u;
		accumDesc.Usage = D3D11_USAGE_DEFAULT;
		accumDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		GRAPHICS_HR_CHECK(_device->CreateTexture2D(&accumDesc, nullptr, oit.pOITAccumTex.GetAddressOf()));
		GRAPHICS_HR_CHECK(_device->CreateRenderTargetView(oit.pOITAccumTex.Get(), nullptr, oit.pOITAccumRTV.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC accumSRVDesc = {};
		accumSRVDesc.Format = accumDesc.Format;
		accumSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		accumSRVDesc.Texture2D.MostDetailedMip = 0;
		accumSRVDesc.Texture2D.MipLevels = 1;

		GRAPHICS_HR_CHECK(_device->CreateShaderResourceView(oit.pOITAccumTex.Get(), &accumSRVDesc, oit.pOITAccumSRV.GetAddressOf()));

		D3D11_TEXTURE2D_DESC revealDesc = accumDesc;
		revealDesc.Format = DXGI_FORMAT_R8_UNORM;

		GRAPHICS_HR_CHECK(_device->CreateTexture2D(&revealDesc, nullptr, oit.pOITRevealTex.GetAddressOf()));
		GRAPHICS_HR_CHECK(_device->CreateRenderTargetView(oit.pOITRevealTex.Get(), nullptr, oit.pOITRevealRTV.GetAddressOf()));

		D3D11_SHADER_RESOURCE_VIEW_DESC revealSRVDesc = {};
		revealSRVDesc.Format = revealDesc.Format;
		revealSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		revealSRVDesc.Texture2D.MostDetailedMip = 0;
		revealSRVDesc.Texture2D.MipLevels = 1;

		GRAPHICS_HR_CHECK(_device->CreateShaderResourceView(oit.pOITRevealTex.Get(), &revealSRVDesc, oit.pOITRevealSRV.GetAddressOf()));
	}

	// If I was the render target reset me.
	if (currentRenderTarget == this)
		currentRenderTarget->setRenderTarget();
}

/*
-----------------------------------------------------------------------------------------------------------
 Graphics class Interface Functions
-----------------------------------------------------------------------------------------------------------
*/

// Before issuing any draw calls to the window, for multiple window settings 
// this function has to be called to bind the window as the render target.

void Graphics::setRenderTarget()
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	// Identify as the render target
	currentRenderTarget = this;

	// Bind the render target
	GRAPHICS_INFO_CHECK(_context->OMSetRenderTargets(1u, data.pTarget.GetAddressOf(), data.pDSV.Get()));

	// Bind the viewport
	CD3D11_VIEWPORT vp;
	vp.Width = (float)WindowDim.x;
	vp.Height = (float)WindowDim.y;
	vp.MinDepth = 0.f;
	vp.MaxDepth = 1.f;
	vp.TopLeftX = 0.f;
	vp.TopLeftY = 0.f;
	GRAPHICS_INFO_CHECK(_context->RSSetViewports(1u, &vp));

	// Bind the perspective
	data.Perspective->Bind();
}

// Swaps the current frame and shows the new frame to the window.

void Graphics::pushFrame()
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	// If you are not the render target, you must be.
	Graphics* target = currentRenderTarget;
	if (target != this)
		setRenderTarget();

	// If OIT is enabled before swapping it needs to merge the RTV with the accum 
	// and revealage buffer on a final draw call.
	if (data.oitEnabled)
	{
		OITransparencyInternals& oit = *data.OIT;

		// Bind backbuffer as RT, no depth
		GRAPHICS_INFO_CHECK(_context->OMSetRenderTargets(1u, data.pTarget.GetAddressOf(), nullptr));

		// Bind resolve blend state
		GRAPHICS_INFO_CHECK(_context->OMSetBlendState(oit.pOITResolveBlendState.Get(), nullptr, 0xFFFFFFFFu));

		// Bind accum + reveal SRVs to PS
		ID3D11ShaderResourceView* srvs[2] = { oit.pOITAccumSRV.Get(), oit.pOITRevealSRV.Get() };
		_context->PSSetShaderResources(0u, 2u, srvs);

		// Set bindables for last merging call
		for (Bindable* b : oit.resolveBindables)
			b->Bind();

		// Draw fullscreen merge
		GRAPHICS_INFO_CHECK(_context->Draw(4u, 0u));

		// Unbind SRVs to avoid Render Target binding conflicts next frame
		ID3D11ShaderResourceView* nullSrvs[2] = { nullptr, nullptr };
		GRAPHICS_INFO_CHECK(_context->PSSetShaderResources(0u, 2u, nullSrvs));

		// Restore backbuffer as RT
		GRAPHICS_INFO_CHECK(_context->OMSetRenderTargets(1u, data.pTarget.GetAddressOf(), data.pDSV.Get()));

		// Restore default blend/depth state
		data.defaultDephtStencil->Bind();
		data.defaultBlender->Bind();
	}

	// Copies the render buffer into the capture image.
	auto capture = [&]()
		{
			// Access the back buffer
			ComPtr<ID3D11Resource> pBackBuffer;
			GRAPHICS_HR_CHECK(data.pSwap->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer));

			// View it as a texture
			ComPtr<ID3D11Texture2D> pBackBufferTex;
			GRAPHICS_HR_CHECK(pBackBuffer.As(&pBackBufferTex));

			// If the copy buffer is not created, create it.
			if (!data.pCaptureStaging)
			{
				D3D11_TEXTURE2D_DESC stDesc = {};

				stDesc.BindFlags = 0;
				stDesc.MiscFlags = 0;
				stDesc.Width = WindowDim.x;
				stDesc.Height = WindowDim.y;
				stDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
				stDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
				stDesc.Usage = D3D11_USAGE_STAGING;
				stDesc.MipLevels = 1;
				stDesc.ArraySize = 1;
				stDesc.SampleDesc.Count = 1;
				stDesc.SampleDesc.Quality = 0;

				GRAPHICS_HR_CHECK(_device->CreateTexture2D(&stDesc, nullptr, &data.pCaptureStaging));
			}

			// Copy data from the back buffer to the copy buffer.
			_context->CopyResource(data.pCaptureStaging.Get(), pBackBufferTex.Get());

			// Map staging resource to CPU pointer.
			D3D11_MAPPED_SUBRESOURCE msr = {};
			GRAPHICS_HR_CHECK(_context->Map(data.pCaptureStaging.Get(), 0, D3D11_MAP_READ, 0, &msr));

			// Create the image with the desired dimensions.
			if (data.captureImage->width() != WindowDim.x || data.captureImage->height() != WindowDim.y)
				data.captureImage->reset(WindowDim.x, WindowDim.y);

			// Copy image pixels
			const unsigned rowBytes = WindowDim.x * sizeof(Color);
			for (int y = 0; y < WindowDim.y; y++)
				memcpy((byte*)data.captureImage->pixels() + y * rowBytes, (byte*)msr.pData + y * msr.RowPitch, rowBytes);

			// Unmap resource
			_context->Unmap(data.pCaptureStaging.Get(), 0);

			// Reset image buffer.
			data.captureImage = nullptr;
		};

	// If a frame capture is scheduled with no UI capture here.
	if (data.captureImage && !data.capture_ui_visible)
		capture();

#ifdef _INCLUDE_IMGUI
	// If the window has an imGui instance call the render function before swapping.
	Window* pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(data.HWnd, GWLP_USERDATA));
	if (pWnd && *pWnd->imGuiPtrAdress())
		if (iGManager* imgui = (iGManager*)(*pWnd->imGuiPtrAdress()))
		{
			imgui->newFrame();
			imgui->render();
			imgui->drawFrame();
		}

#endif

	// If a frame capture is scheduled with UI capture here.
	if (data.captureImage && data.capture_ui_visible)
		capture();

	// Present the new frame to the window.
	HRESULT hr;
	if (FAILED(hr = data.pSwap->Present(1u, 0u))) 
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED)
			GRAPHICS_HR_DEVICE_REMOVED_ERROR(_device->GetDeviceRemovedReason());
		else
			GRAPHICS_HR_ERROR(hr);
	}

	// Reset to old render target if was another.
	if (target && target != this)
		target->setRenderTarget();
}

// Clears the buffer with the specified color. If all buffers is false it will only clear
// the screen color. the depth buffer and the transparency buffers will stay the same.

void Graphics::clearBuffer(Color color, bool all_buffers)
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	_float4color col = color.getColor4();
	GRAPHICS_INFO_CHECK(_context->ClearRenderTargetView(data.pTarget.Get(), &col.r));

	if (all_buffers)
		clearDepthBuffer(), clearTransparencyBuffers();
}

// Clears the depth buffer so that all objects painted are moved to the back.
// The last frame pixels are still on the render target.

void Graphics::clearDepthBuffer()
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	GRAPHICS_INFO_CHECK(_context->ClearDepthStencilView(data.pDSV.Get(), D3D11_CLEAR_DEPTH, 1.f, 0u));
}

// If OITransparency is enabled clears the two buffers related to OIT plotting.
// IF it is not enabled it does nothing.

void Graphics::clearTransparencyBuffers()
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	if (data.oitEnabled)
	{
		OITransparencyInternals& oit = *data.OIT;

		const float clearAccum[4] = { 0.f, 0.f, 0.f, 0.f };
		const float clearReveal[4] = { 1.f, 1.f, 1.f, 1.f };

		GRAPHICS_INFO_CHECK(_context->ClearRenderTargetView(oit.pOITAccumRTV.Get(), clearAccum));
		GRAPHICS_INFO_CHECK(_context->ClearRenderTargetView(oit.pOITRevealRTV.Get(), clearReveal));
	}
}

// Updates the perspective on the window, by changing the observer quaternion, 
// the center of the POV and the scale of the object looked at.

void Graphics::setPerspective(Quaternion obs, Vector3f center, float scale)
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	USER_CHECK(obs, "The observer must be a quaternion diferent than zero.");

	Scale = scale;

	cbuff.observer = obs.normalize();
	cbuff.center = center.getVector4();
	cbuff.scaling = { scale / WindowDim.x, scale / WindowDim.y, scale, 0.f };

	data.Perspective->update(&cbuff);
}

// Sets the observer quaternion that defines the POV on the window.

void Graphics::setObserver(Quaternion obs)
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	USER_CHECK(obs, "The observer must be a quaternion diferent than zero.");

	cbuff.observer = obs.normalize();
	data.Perspective->update(&cbuff);
}

// Sets the center of the window perspective.

void Graphics::setCenter(Vector3f center)
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	cbuff.center = center.getVector4();
	data.Perspective->update(&cbuff);
}

// Sets the scale of the objects, defined as pixels per unit distance.

void Graphics::setScale(float scale)
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	Scale = scale;
	cbuff.scaling = { scale / WindowDim.x, scale / WindowDim.y, scale, 0.f };

	data.Perspective->update(&cbuff);
}

// Schedules a frame capture to be done during the next pushFrame() call. It expects 
// a valid pointer to an Image where the capture will be stored. The image dimensions 
// will be adjusted automatically. Pointer must be valid during next push call.
// If ui_visible is set to false the capture will be taken before rendering imGui.

void Graphics::scheduleFrameCapture(Image* image, bool ui_visible)
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	USER_CHECK(!data.captureImage,
		"ScheduleFrameCapture should not be called twice on the same frame.\n"
		"The capture is not stored on the Image until the next pushFrame() call is issued."
	);

	USER_CHECK(image, "Expected to find a valid image pointer on scheduleFrameCapture but found nullptr.");

	data.captureImage = image;
	data.capture_ui_visible = ui_visible;
}

// To draw transparent objects this setting needs to be toggled on, it causes extra 
// conputation due to other buffers being used for rendering, so only turn on if needed.
// It uses the McGuire/Bavoli OIT approach. For more information you can check the 
// original paper at: https://jcgt.org/published/0002/02/09/

void Graphics::enableTransparency()
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	if (data.oitEnabled)
		return;

	data.oitEnabled = true;
	data.OIT = new OITransparencyInternals;

	OITransparencyInternals& oit = *data.OIT;

	// --- 1) Create OIT Render Targets (if we already have valid WindowDim) ---

	if (WindowDim.x > 0 && WindowDim.y > 0)
	{
		// Prepare D3D11 descriptors
		D3D11_TEXTURE2D_DESC texDesc = {};
		texDesc.Width = (UINT)WindowDim.x;
		texDesc.Height = (UINT)WindowDim.y;
		texDesc.MipLevels = 1u;
		texDesc.ArraySize = 1u;
		texDesc.SampleDesc.Count = 1u;
		texDesc.SampleDesc.Quality = 0u;
		texDesc.Usage = D3D11_USAGE_DEFAULT;
		texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.MipLevels = 1;

		// Create Accumulative: Texture, Render Target View and Shader Resource View (RGBA16)
		texDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		SRVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

		GRAPHICS_HR_CHECK(_device->CreateTexture2D(&texDesc, nullptr, oit.pOITAccumTex.GetAddressOf()));
		GRAPHICS_HR_CHECK(_device->CreateRenderTargetView(oit.pOITAccumTex.Get(), nullptr, oit.pOITAccumRTV.GetAddressOf()));
		GRAPHICS_HR_CHECK(_device->CreateShaderResourceView(oit.pOITAccumTex.Get(), &SRVDesc, oit.pOITAccumSRV.GetAddressOf()));

		// Create Revealage: Texture, Render Target View and Shader Resource View (R8)
		texDesc.Format = DXGI_FORMAT_R8_UNORM;
		SRVDesc.Format = DXGI_FORMAT_R8_UNORM;

		GRAPHICS_HR_CHECK(_device->CreateTexture2D(&texDesc, nullptr, oit.pOITRevealTex.GetAddressOf()));
		GRAPHICS_HR_CHECK(_device->CreateRenderTargetView(oit.pOITRevealTex.Get(), nullptr, oit.pOITRevealRTV.GetAddressOf()));
		GRAPHICS_HR_CHECK(_device->CreateShaderResourceView(oit.pOITRevealTex.Get(), &SRVDesc, oit.pOITRevealSRV.GetAddressOf()));
	}

	// --- 2) Create OIT accumulation blend state (2 MRTs) ---

	{
		D3D11_BLEND_DESC bd = {};
		bd.AlphaToCoverageEnable = FALSE;
		bd.IndependentBlendEnable = TRUE;

		// RT0: accum
		auto& r0 = bd.RenderTarget[0];
		r0.BlendEnable = TRUE;
		r0.SrcBlend = D3D11_BLEND_ONE;
		r0.DestBlend = D3D11_BLEND_ONE;
		r0.BlendOp = D3D11_BLEND_OP_ADD;
		r0.SrcBlendAlpha = D3D11_BLEND_ONE;
		r0.DestBlendAlpha = D3D11_BLEND_ONE;
		r0.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		r0.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		// RT1: reveal
		auto& r1 = bd.RenderTarget[1];
		r1.BlendEnable = TRUE;
		r1.SrcBlend = D3D11_BLEND_ZERO;
		r1.DestBlend = D3D11_BLEND_INV_SRC_COLOR; // product of (1 - alpha)
		r1.BlendOp = D3D11_BLEND_OP_ADD;
		r1.SrcBlendAlpha = D3D11_BLEND_ZERO;
		r1.DestBlendAlpha = D3D11_BLEND_ONE;
		r1.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		r1.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		GRAPHICS_HR_CHECK(_device->CreateBlendState(&bd, oit.pOITBlendState.GetAddressOf()));
	}

	// --- 3) Create depth read-only state for OIT draw calls ---

	{
		D3D11_DEPTH_STENCIL_DESC dsd = {};
		dsd.DepthEnable = TRUE;
		dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO; // no writes
		dsd.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
		dsd.StencilEnable = FALSE;

		GRAPHICS_HR_CHECK(_device->CreateDepthStencilState(&dsd, oit.pOITDepthReadOnly.GetAddressOf()));
	}

	// --- 4) Create resolve blend state for fullscreen composite ---

	{
		D3D11_BLEND_DESC rbd = {};
		rbd.AlphaToCoverageEnable = FALSE;
		rbd.IndependentBlendEnable = FALSE;

		auto& rr = rbd.RenderTarget[0];
		rr.BlendEnable = TRUE;
		rr.SrcBlend = D3D11_BLEND_SRC_ALPHA;
		rr.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		rr.BlendOp = D3D11_BLEND_OP_ADD;
		rr.SrcBlendAlpha = D3D11_BLEND_ONE;
		rr.DestBlendAlpha = D3D11_BLEND_ZERO;
		rr.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		rr.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

		GRAPHICS_HR_CHECK(_device->CreateBlendState(&rbd, oit.pOITResolveBlendState.GetAddressOf()));
	}

	// --- 5) Create fullscreen quad/triangle resources and resolve shaders ---

	{
		Vector2f V[4] = { { -1.f, +1.f }, { -1.f, -1.f }, { +1.f, +1.f }, { +1.f, -1.f } };
		INPUT_ELEMENT_DESC ied = { "Position", _2_FLOAT };

		oit.resolveBindables[0] = new VertexBuffer(V, 4);
#ifndef _DEPLOYMENT
		oit.resolveBindables[1] = new VertexShader(PROJECT_DIR L"shaders/OITresolveVS.cso");
		oit.resolveBindables[2] = new PixelShader(PROJECT_DIR L"shaders/OITresolvePS.cso");
#else
		oit.resolveBindables[1] = new VertexShader(getBlobFromId(BLOB_ID::BLOB_OIT_RESOLVE_VS), getBlobSizeFromId(BLOB_ID::BLOB_OIT_RESOLVE_VS));
		oit.resolveBindables[2] = new PixelShader(getBlobFromId(BLOB_ID::BLOB_OIT_RESOLVE_PS), getBlobSizeFromId(BLOB_ID::BLOB_OIT_RESOLVE_PS));
#endif
		oit.resolveBindables[3] = new Sampler(SAMPLE_FILTER_POINT, SAMPLE_ADDRESS_WRAP);
		oit.resolveBindables[4] = new InputLayout(&ied, 1u, (VertexShader*)oit.resolveBindables[1]);
		oit.resolveBindables[5] = new Topology(TRIANGLE_STRIP);
		oit.resolveBindables[6] = new Rasterizer(false);
	}

}

// Deletes the extra buffers and disables the extra steps when pushing frames.

void Graphics::disableTransparency()
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	if (data.oitEnabled)
	{
		for (Bindable* b : data.OIT->resolveBindables)
			delete b;

		delete data.OIT;
		data.OIT = nullptr;
		data.oitEnabled = false;
	}
}

// Returns whether OITransparency is enabled on this Graphics object.

bool Graphics::isTransparencyEnabled() const
{
	GraphicsInternals& data = *((GraphicsInternals*)GraphicsData);

	return data.oitEnabled;
}
