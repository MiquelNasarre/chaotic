# Porting Layout

Chaotic is being split into a portable core and renderer backends.

## Targets

- `chaotic_core`: portable code that can compile on macOS without Win32 or Direct3D.
  It currently owns math types, image handling, keyboard/mouse state containers, timers,
  and the base user-error pipeline.
- `chaotic_d3d11`: the existing Win32/Direct3D 11 implementation. It owns windows,
  graphics devices, bindables, drawables, ImGui Win32/DX11 glue, embedded shader blobs,
  and the current demo implementation.
- `chaotic_metal`: the native macOS Metal backend. It currently owns Cocoa window
  creation, Metal device/queue setup, `CAMetalLayer` presentation, event polling,
  and the first implementation of the neutral `chaotic::render` interfaces.
  It also builds `chaotic_metal_demo`, a small executable that renders a curve
  and a line-mesh scatter through the new path.

## Current Boundary

The public core facade is:

```cpp
#include "chaotic_core.h"
```

The current Direct3D facade is:

```cpp
#include "chaotic_d3d11.h"
```

The current Metal facade is:

```cpp
#include "chaotic_metal.h"
```

`chaotic_d3d11` still reuses the historical headers under `chaotic/include` and
`chaotic/chaotic_headers`. `chaotic_metal` deliberately exposes a small C++ API first:
it can create a native Metal window, present frames, create vertex buffers, compile MSL
shader libraries, create simple render pipelines, issue non-indexed draw calls, and draw
the first portable `chaotic::metal::Curve` and `chaotic::metal::Scatter` objects through
`chaotic::render::Scene`. Dynamic buffers can now be updated through the neutral
`render::Buffer` interface, and command encoders can bind buffers to vertex or fragment
shader stages.

The old `Window`/`Graphics`/`Drawable` surface is not implemented on top of Metal yet.
`Curve` and `Scatter` now have portable geometry stages in `chaotic::render`; the D3D11
drawables still use the historical `Bindable` GPU path, but their vertex/color data now comes
from shared geometry code. The Metal drawables create buffers, shader libraries, and pipelines
through the backend-neutral interfaces. They currently support depth-tested rendering plus
opaque, alpha, and additive single-render-target blending; weighted OIT is still left to a
future pass.

## macOS Smoke Run

```sh
cmake -S . -B build -DCHAOTIC_BUILD_D3D11_BACKEND=OFF -DCHAOTIC_BUILD_METAL_BACKEND=ON
cmake --build build -j
build/chaotic/backend_metal/chaotic_metal_demo --frames 1
```
