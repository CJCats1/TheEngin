# Building a New App with TheEngine as a DLL

This document describes how to create a standalone Visual Studio solution that uses TheEngine as a DLL and runs a scene (e.g. PartitionScene) in its own executable. Use it when spinning up another app from an engine scene so you don’t have to re-discover all the steps.

---

## 1. Overview

- **Goal:** New `.sln` + app project that references TheEngine, builds an exe, and runs a scene.
- **Engine:** Built as a DLL (`TheEngine.dll`). The app links `TheEngine.lib` and loads the DLL at runtime.
- **Catch:** Many engine types (Mat4, Quadtree, geom, ShadowMap, etc.) are **not** exported from the DLL. The app must compile those engine sources into the exe to resolve linker symbols.

**Assumed layout:**

- Repo root: `TheEngine\TheEngine\` (contains `TheEngine\` subfolder with Source/Include/Assets).
- New app solution: e.g. `ThePartitionVisualization\` (sibling or elsewhere), with `ThePartitionVisualization.sln` and app `.vcxproj`.

---

## 2. Solution and Project Structure

Create a folder for the new app, e.g. `ThePartitionVisualization\`, with:

- `ThePartitionVisualization.sln` – solution that references the engine project and the app project.
- `ThePartitionVisualization.vcxproj` – app project (exe).
- `main.cpp` – entry point that creates `Game` and sets your scene.
- Scene sources – e.g. `Scenes\PartitionScene.cpp` (and split files if any).
- Optional local includes – e.g. `Include\TheEngine\Game\Scenes\PartitionScene.h` if you need a copy or overrides.

---

## 3. Creating the Solution (`.sln`)

1. Create a new solution in Visual Studio (or add a blank solution), add the **existing** TheEngine project:
   - Add → Existing Project → `..\TheEngine\TheEngine\TheEngine.vcxproj`.
2. Add the new app project (see below) and set it as the startup project.
3. Ensure solution configs (e.g. Debug|x64, Release|x64) map to the same configs for both projects.

Example structure (paths relative to solution dir):

```text
Solution: ThePartitionVisualization.sln
├── TheEngine          → ..\TheEngine\TheEngine\TheEngine.vcxproj
└── ThePartitionVisualization → ThePartitionVisualization.vcxproj
```

---

## 4. Creating the App Project (`.vcxproj`)

### 4.1 Basic settings

- **Configuration type:** Application (exe).
- **Platform:** x64 (match the engine).
- **C++ standard:** C++20.
- **Character set:** Unicode.
- **Vcpkg:** If you don’t use a vcpkg manifest for this app, set in the project:

  ```xml
  <VcpkgEnableManifest>false</VcpkgEnableManifest>
  ```

### 4.2 Paths and properties

Define the engine “root” (the folder that contains `Source`, `Include`, `Assets`):

```xml
<TheEngineRoot>$(SolutionDir)..\TheEngine\TheEngine\TheEngine</TheEngineRoot>
```

- **Include path:** So that `#include <TheEngine/...>` works. Include at least:
  - `$(ProjectDir)Include`
  - `$(TheEngineRoot)\Include`
  - `$(TheEngineRoot)\Source`
  - Any imgui/glfw/etc. and vcpkg include paths the engine uses (e.g. imgui-docking, implot, glfw, vcpkg `x64-windows\include`).

- **Output / intermediate:** e.g. `$(SolutionDir)Bin\$(Platform)\$(Configuration)\` and `$(SolutionDir)Intermediate\YourApp\$(Platform)\$(Configuration)\`.

### 4.3 Linker

- **Subsystem:** Console (or Windows if you prefer).
- **Additional dependencies:**  
  `TheEngine.lib;d3d11.lib;d3dcompiler.lib;d2d1.lib;dwrite.lib;%(AdditionalDependencies)`
- **Additional library directories:**  
  `$(SolutionDir)..\TheEngine\TheEngine\Bin\$(Platform)\$(Configuration);$(SolutionDir)Bin\$(Platform)\$(Configuration);%(AdditionalLibraryDirectories)`

So the app links against the import library of the engine DLL.

### 4.4 Post-build: copy DLLs to app output

Copy the engine DLL (and any runtime DLLs the engine needs) into the app’s output directory:

```xml
<PostBuildEvent>
  <Command>xcopy /Y "$(SolutionDir)..\TheEngine\TheEngine\Bin\$(Platform)\$(Configuration)\TheEngine.dll" "$(OutDir)"
if exist "$(SolutionDir)..\TheEngine\TheEngine\Bin\$(Platform)\$(Configuration)\glfw3.dll" xcopy /Y "$(SolutionDir)..\TheEngine\TheEngine\Bin\$(Platform)\$(Configuration)\glfw3.dll" "$(OutDir)"
if not exist "$(OutDir)glfw3.dll" if exist "$(SolutionDir)..\TheEngine\TheEngine\vcpkg_installed\x64-windows\debug\bin\glfw3.dll" xcopy /Y "$(SolutionDir)..\TheEngine\TheEngine\vcpkg_installed\x64-windows\debug\bin\glfw3.dll" "$(OutDir)"
if not exist "$(OutDir)glfw3.dll" if exist "$(SolutionDir)..\TheEngine\TheEngine\vcpkg_installed\x64-windows\bin\glfw3.dll" xcopy /Y "$(SolutionDir)..\TheEngine\TheEngine\vcpkg_installed\x64-windows\bin\glfw3.dll" "$(OutDir)"</Command>
</PostBuildEvent>
```

Adjust paths if your engine or vcpkg live elsewhere.

### 4.5 Working directory (critical for assets)

The engine loads assets (e.g. shaders) with paths like `TheEngine/Assets/Shaders/Basic.hlsl` **relative to the process current directory**. So the app must run with its working directory set to the folder that **contains** the `TheEngine` directory (e.g. repo root `TheEngine\TheEngine`), not inside the engine folder.

In the app project:

```xml
<LocalDebuggerWorkingDirectory>$(SolutionDir)..\TheEngine\TheEngine</LocalDebuggerWorkingDirectory>
```

Set this for both Debug and Release. If you run the exe by double-clicking, it will use the exe’s directory as CWD; then either run from a directory that contains `TheEngine`, or copy `TheEngine\Assets` into the output and adjust paths.

---

## 5. Scene and Code Adaptations

### 5.1 Base `Scene` API

The engine’s `Scene` base class declares:

```cpp
virtual void render(GraphicsEngine& engine, IRenderSwapChain& swapChain) = 0;
```

- Use **`IRenderSwapChain&`**, not `SwapChain&`.
- Include: `<TheEngine/Graphics/Abstraction/RenderSwapChain.h>`.
- In the implementation, `ctx.clearAndSetBackBuffer(swapChain, color)` is valid (interface takes `const IRenderSwapChain&`).

### 5.2 Include paths that differ from older code

- **Spatial structures** may live under **DataStructures**, not Components:
  - `TheEngine/DataStructures/Quadtree.h`
  - `TheEngine/DataStructures/AABBTree.h`
  - `TheEngine/DataStructures/KDTree.h`
  - `TheEngine/DataStructures/Octree.h`
- If the scene still uses **ButtonComponent** / **PanelComponent** and those don’t exist in the engine, remove or stub that UI and use ImGui (or whatever the engine provides).

### 5.3 Engine exposes interfaces, not D3D types

- `GraphicsEngine::getGraphicsDevice()` returns **`IRenderDevice&`**.
- `getContext()` returns **`IRenderContext&`**.
- Scene code that needs D3D-specific APIs (e.g. `getD3DDevice()`, `getGraphicsResourceDesc()`, raw `ID3D11DeviceContext*`) must:
  - Include the concrete headers: `<TheEngine/Graphics/GraphicsDevice.h>`, `<TheEngine/Graphics/DeviceContext.h>` (or equivalent).
  - **Cast** at call sites, e.g.  
    `static_cast<GraphicsDevice&>(engine.getGraphicsDevice())`,  
    `static_cast<GraphicsDevice&>(lineRenderer->getDevice())`.
- For texture loading, use the overload that takes **`IRenderDevice&`**:  
  `Texture2D::LoadTexture2D(device, path)` so you don’t need `getD3DDevice()` for that.
- For raw D3D context from `IRenderContext&`:  
  `static_cast<ID3D11DeviceContext*>(ctx.getNativeContextHandle())`  
  (no `getD3DDeviceContext()` on the interface).

### 5.4 Shadow maps and `NativeGraphicsHandle`

- `setShadowMaps` expects `const NativeGraphicsHandle*` and `NativeGraphicsHandle` (e.g. `void*`).
- Cast SRV array and sampler, e.g.  
  `reinterpret_cast<const NativeGraphicsHandle*>(shadowSRVs.data())`,  
  `static_cast<NativeGraphicsHandle>(m_shadowSampler.Get())`,  
  and use `static_cast<ui32>(shadowSRVs.size())` for the count.

### 5.5 Missing `Mesh3DComponent` in the engine

If the engine doesn’t ship `Mesh3DComponent` (e.g. it lived in another module), you can add a minimal implementation in the app:

- Header: e.g. `Include\TheEngine\Components\Mesh3DComponent.h` with a class that holds `std::shared_ptr<Mesh>`, position, scale, visibility, material, and `draw(IRenderContext&)`.
- Implementation: set world matrix and material on the context, then call `mesh->draw(ctx)`.
- Add the implementation `.cpp` to the app project and use `#include <TheEngine/Components/Mesh3DComponent.h>` so your scene compiles.

---

## 6. Unresolved Externals: Compile Engine Sources Into the App

TheEngine DLL does not export many symbols (no `THEENGINE_API` on Mat4, Quadtree, geom, ShadowMap, GraphicsDevice, etc.). So when the app links only `TheEngine.lib`, you get “unresolved external symbol” for those. Fix: **compile the corresponding engine `.cpp` files into the app** so their symbols are in the exe.

Add these (or equivalent) to the app project, using `$(TheEngineRoot)` so paths match your layout:

**Math and geometry**

- `$(TheEngineRoot)\Source\TheEngine\Math\Geometry.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Math\Types.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Math\MathBackend.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Math\DefaultMathBackend.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Math\GLMMathBackend.cpp`  
  (omit if you don’t use GLM / `THEENGINE_USE_GLM`)

**Data structures**

- `$(TheEngineRoot)\Source\TheEngine\DataStructures\Quadtree.cpp`
- `$(TheEngineRoot)\Source\TheEngine\DataStructures\KDTree.cpp`
- `$(TheEngineRoot)\Source\TheEngine\DataStructures\AABBTree.cpp`
- `$(TheEngineRoot)\Source\TheEngine\DataStructures\Octree.cpp`

**Graphics (needed if the scene uses ShadowMap / GraphicsDevice / pipelines)**

- `$(TheEngineRoot)\Source\TheEngine\Graphics\ShadowMap.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Graphics\GraphicsDevice.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Graphics\DeviceContext.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Graphics\GraphicsPipelineState.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Graphics\ShaderBinary.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Graphics\SwapChain.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Graphics\VertexBuffer.cpp`
- `$(TheEngineRoot)\Source\TheEngine\Graphics\VertexShaderSignature.cpp`

If you hit more unresolved symbols, add the engine `.cpp` that defines them the same way.

---

## 7. Main Entry Point

Typical `main.cpp`:

```cpp
#include <TheEngine/All.h>
#include <TheEngine/Graphics/GraphicsEngine.h>
#include <TheEngine/Game/Scenes/PartitionScene.h>
#include <windows.h>

int main()
{
    try
    {
        TheEngine::Game game({ {1280, 720}, TheEngine::Logger::LogLevel::Info });
        game.setScene(std::make_unique<TheEngine::PartitionScene>());
        game.run();
    }
    catch (const std::runtime_error& e)
    {
        MessageBoxA(nullptr, e.what(), "Runtime Error", MB_OK | MB_ICONERROR);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
```

Adjust scene type and window/config as needed.

---

## 8. Checklist (quick reference)

- [ ] New solution references TheEngine project and app project; app is startup project.
- [ ] App project: Application, x64, C++20, `VcpkgEnableManifest=false` if no manifest.
- [ ] `TheEngineRoot` and include paths so `#include <TheEngine/...>` resolve.
- [ ] Linker: `TheEngine.lib` and engine lib dir; D3D/DirectWrite libs as needed.
- [ ] Post-build: copy `TheEngine.dll` and `glfw3.dll` (or other runtime DLLs) to `$(OutDir)`.
- [ ] `LocalDebuggerWorkingDirectory` = directory that **contains** `TheEngine` (so `TheEngine/Assets/Shaders/...` works).
- [ ] Scene uses `IRenderSwapChain&` for `render()`; no `SwapChain&`.
- [ ] Scene uses `IRenderDevice&` / `IRenderContext&` where possible; cast to `GraphicsDevice&` / D3D context only where needed; texture load via `IRenderDevice&`.
- [ ] setShadowMaps: cast to `NativeGraphicsHandle*` / `NativeGraphicsHandle` and `ui32` count.
- [ ] If Mesh3DComponent (or other component) is missing in the engine, add a local implementation and include it in the app.
- [ ] Add engine `.cpp` files (math, data structures, graphics) listed above to the app project to fix unresolved externals.
- [ ] Build TheEngine first, then the app; run from Visual Studio so the working directory is set correctly.

---

## 9. Running Without Visual Studio

If you start the exe by double-clicking or from another app, the working directory is usually the exe’s folder. Then `TheEngine/Assets/Shaders/...` won’t be found unless you:

- Run from a directory that contains the `TheEngine` folder (e.g. `TheEngine\TheEngine`), or
- Copy `TheEngine\Assets` (and any other needed assets) into the exe output and/or change the engine to support an asset root path.

For development, running under Visual Studio (F5) with `LocalDebuggerWorkingDirectory` set as above is the simplest.
