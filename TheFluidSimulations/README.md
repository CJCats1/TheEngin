# TheFluidSimulations

Standalone app that uses **TheEngine as a DLL** and runs the FLIP, SPH, and Powder scenes from the Archive.

## Structure

- **TheFluidSimulations.sln** – Solution with TheFluidSimulations (exe) and TheEngine (DLL). TheFluidSimulations is the default startup project.
- **TheFluidSimulations.vcxproj** – App project: links `TheEngine.lib`, compiles selected engine sources for unresolved symbols, and copies `TheEngine.dll` (and `glfw3.dll` if present) to the output folder.
- **main.cpp** – Entry point: creates `Game`, sets the scene from command line (default FLIP), and runs.
- **Scenes/** – FLIP, SPH, and Powder scene implementations (adapted from `Archive/Scenes/`).
- **Include/TheEngine/Game/Scenes/** – Scene headers.
- **Include/TheEngine/Components/** – Local **FirmGuyComponent** and **FirmGuySystem** stubs used by FLIP/SPH (not in the engine DLL).

## Building

1. Open `TheFluidSimulations.sln` in Visual Studio.
2. Build the solution (F6 or Build → Build Solution). TheEngine builds first, then TheFluidSimulations.
3. Set **TheFluidSimulations** as startup project if needed (right‑click → Set as Startup Project).

## Running

- **Working directory:** The debugger working directory is set to `$(SolutionDir)..` (repo root) so asset paths like `TheEngine/Assets/Textures/...` resolve.
- **Scene selection:** Run with no args for FLIP (default), or:
  - `TheFluidSimulations.exe 1` – FLIP fluid
  - `TheFluidSimulations.exe 2` – SPH fluid
  - `TheFluidSimulations.exe 3` – Powder / sand

## Reference

See **BUILDING-AN-APP-WITH-THEENGINE.md** in the repo root for the full guide on using TheEngine as a DLL (include paths, linker settings, engine sources to compile in, working directory, and scene adaptations).
