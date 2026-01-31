# Updating TheEngine in Your Projects

This guide explains how to keep your game projects in sync with engine updates while keeping **your** code (scenes, components, entities, `main.cpp`) untouched.

## What Is “Engine” vs “Your Content”

| Belongs to engine (safe to overwrite on update) | Your content (never overwritten by updater) |
|-------------------------------------------------|---------------------------------------------|
| `TheEngine/` (Include, Source, Assets)          | `Game/` (main.cpp, your scenes)             |
| `vcpkg.json` (optional)                         | Your custom Scenes, Components, Entities   |
| `BACKEND_SWITCHING.md`, `ENGINE_UPDATING.md`    | Solution/project customizations you keep    |

- **Engine**: Core runtime, graphics backends, physics, windowing, ImGui, math, etc. Living in `TheEngine/` is the only code that should be replaced when you “update the engine.”
- **Your content**: Entry point and scene selection in `Game/main.cpp`, plus any custom `Scene` subclasses, components, and entity setup. The engine never hardcodes which scene runs; your `main.cpp` calls `game.setScene(...)` before `game.run()`.

## Recommended Project Layout

```
YourProject/
├── Game/
│   ├── main.cpp          ← Your entry point; creates Game, setScene(your_scene), run()
│   └── Scenes/           ← Optional: your Scene classes
├── TheEngine/            ← Engine (updated by submodule or script)
│   ├── Include/
│   ├── Source/
│   └── Assets/
├── TheEngine.sln
├── TheEngine.vcxproj
└── vcpkg.json
```

Your `main.cpp` should:

1. Create `TheEngine::Game` with your `GameDesc`.
2. Call `game.setScene(std::make_unique<YourScene>())` (or your default scene).
3. Call `game.run()`.

The engine does **not** set a default scene; that keeps the engine reusable across projects.

## How to Update the Engine in Another Project

### Option 1: Git submodule (recommended if you use Git)

1. In your game repo, add TheEngine as a submodule (if not already):
   ```bash
   git submodule add <url-of-TheEngine-repo> TheEngine
   ```
2. To pull the latest engine changes:
   ```bash
   cd TheEngine
   git fetch && git checkout main   # or your engine branch
   cd ..
   git add TheEngine
   git commit -m "Update TheEngine"
   ```

Your `Game/` and `main.cpp` stay as-is; only the `TheEngine/` submodule is updated.

### Option 2: Updater script (copy from this repo)

Use the script when the engine is not a submodule (e.g. you copied the repo once and want to refresh it).

1. From **this** engine repo (the one you keep updated), run:
   ```powershell
   .\scripts\update_engine_in_project.ps1 -TargetPath "C:\Path\To\Your\Game\Project"
   ```
2. The script copies:
   - `TheEngine/` → target’s `TheEngine/` (overwrites)
   - Optionally: `vcpkg.json`, `BACKEND_SWITCHING.md`, `ENGINE_UPDATING.md`
3. It does **not** overwrite:
   - `Game/` (your main.cpp and scenes)

So you can safely re-run the script whenever you’ve updated the main engine repo and want to push those changes into another project.

### Option 3: Manual copy

- Copy the entire `TheEngine/` folder from the updated engine repo into your project, overwriting the existing `TheEngine/`.
- Do **not** replace your `Game/` folder with the engine’s sample `Game/` unless you intend to reset your entry point and scenes.

## After Updating

1. Rebuild the project (engine sources and possibly vcpkg may have changed).
2. If the public API changed (e.g. `Scene`, `Game`, `GraphicsEngine`), fix compile errors in **your** code (Game, Scenes, Components); the updater does not modify your files.
3. See `BACKEND_SWITCHING.md` if you need to switch or verify the render backend (OpenGL / DirectX11).

## Summary

- **Engine** = `TheEngine/` (and optionally vcpkg/docs). Update this when you want the latest backend and features.
- **Your content** = `Game/main.cpp` and your scenes/components/entities. You keep these; the updater and submodule flow leave them untouched.
- Use a **submodule** for a clean Git-based workflow, or the **updater script** to copy the engine into a project that isn’t using a submodule.
