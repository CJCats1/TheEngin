## Switching Render Backends

You can switch between DirectX11 and OpenGL in two ways.

### 1) Default backend (code)
Edit `DX3D/Include/DX3D/Core/Common.h`:

```
RenderBackendType backendType{ RenderBackendType::OpenGL };
```

Set it to `OpenGL` or `DirectX11`.

### 2) Runtime override (environment variable)
Set `DX3D_BACKEND` before launch:

- `DX3D_BACKEND=opengl`
- `DX3D_BACKEND=dx11`

On Windows (PowerShell):

```
$env:DX3D_BACKEND="opengl"
.\Bin\x64\Debug\TheEngine.exe
```

### Notes
- The environment variable overrides the code default.
- You can confirm the active backend in the ImGui panel (`Backend: ...`).
