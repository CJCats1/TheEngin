# Engine Refactoring Plan - Addressing Bloat

## Current Issues Identified

### 1. **Excessive Scene Proliferation** (High Priority)
- **13+ different scenes** in the codebase
- Many appear to be test/demo scenes that could be consolidated
- Hardcoded scene switching with massive switch statements
- Each scene duplicates initialization code

**Scenes Found:**
- TestScene
- BridgeScene  
- SpiderSolitaireScene
- PhysicsTetrisScene
- JellyTetrisReduxScene
- PartitionScene (already split across 6 files!)
- ThreeDTestScene
- FlipFluidSimulationScene
- SPHFluidSimulationScene
- CloudScene
- PowderScene (3295 lines!)
- ThreeDBridgeGameScene
- MarbleMazeScene

### 2. **Monolithic Scene Files** (High Priority)
- `PowderScene.cpp`: **3,295 lines** - way too large
- `PartitionScene` already split into multiple files (showing signs of bloat)
- Large files are hard to maintain, test, and understand

### 3. **Code Duplication** (Medium Priority)
- **Repeated initialization patterns** in every scene:
  - Camera setup (EntityManager, Camera2D/3D creation)
  - EntityManager creation
  - LineRenderer setup
  - GraphicsDevice pointer storage
- Similar update/render patterns across scenes

### 4. **Duplicate Libraries** (Low Priority - Easy Fix)
- Both `imgui-docking` and `imgui-master` in `DX3D/Include/Libraries/`
- Only one is needed (likely imgui-docking based on usage)

### 5. **Graphics Pipeline Proliferation** (Medium Priority)
- **9+ different pipeline states** in GraphicsEngine:
  - Default, Text, 3D, Toon, ShadowMap, ShadowMapDebug, BackgroundDots, Line, Skybox
- Some may be unused or could be consolidated

### 6. **Scene-Specific Components** (Medium Priority)
- Many components seem tied to specific scenes:
  - `TetrisPhysicsComponent` - only used in Tetris scenes
  - `CarPhysicsComponent` - only used in car scenes
  - `SoftGuyComponent`, `SpringGuyComponent` - specific to test scenes
- These could be moved to scene-specific folders or made more generic

### 7. **Manual Scene Management** (High Priority)
- Hardcoded switch statements in `Game.cpp` for scene switching
- SceneType enum needs updating for every new scene
- No plugin/registration system for scenes

---

## Recommended Refactoring Steps

### Phase 1: Quick Wins (1-2 days)

#### 1.1 Remove Duplicate ImGui Library
```bash
# Keep imgui-docking, remove imgui-master
# Update any includes if needed
```

#### 1.2 Create Scene Base Helper Class
Create `BaseScene` or `SceneHelper` to eliminate repeated initialization:
```cpp
class SceneHelper {
public:
    static Camera2D* createDefaultCamera2D(EntityManager& em, float zoom = 1.0f);
    static Camera3D* createDefaultCamera3D(EntityManager& em);
    static LineRenderer* createLineRenderer(EntityManager& em, GraphicsDevice& device, GraphicsEngine& engine);
    // Common initialization patterns
};
```

#### 1.3 Audit and Remove Unused Scenes
- Identify which scenes are actively used
- Archive or remove test/demo scenes that aren't needed
- Consider consolidating similar scenes (e.g., PhysicsTetrisScene + JellyTetrisReduxScene)

### Phase 2: Scene System Refactoring (3-5 days)

#### 2.1 Implement Scene Registration System
Replace hardcoded switch statements with a registration system:

```cpp
class SceneRegistry {
public:
    using SceneFactory = std::function<std::unique_ptr<Scene>()>;
    
    static void registerScene(const std::string& name, SceneFactory factory);
    static std::unique_ptr<Scene> createScene(const std::string& name);
    static std::vector<std::string> getAvailableScenes();
    
private:
    static std::unordered_map<std::string, SceneFactory> s_factories;
};

// Usage in scenes:
REGISTER_SCENE("PowderScene", []() { return std::make_unique<PowderScene>(); });
```

#### 2.2 Split Large Scene Files
Break down `PowderScene.cpp` (3295 lines) into logical modules:

```
PowderScene/
  ├── PowderScene.h
  ├── PowderScene.cpp (main scene logic, ~200 lines)
  ├── ParticleSystem.cpp (~800 lines)
  ├── AirSystem.cpp (~600 lines)
  ├── ParticleRenderer.cpp (~400 lines)
  ├── ParticleProperties.cpp (~200 lines)
  └── PowderSceneUI.cpp (~300 lines)
```

Apply similar splitting to other large scenes.

### Phase 3: Component Organization (2-3 days)

#### 3.1 Organize Components by Category
Create subdirectories:
```
Components/
  ├── Core/          (PhysicsComponent, TransformComponent, etc.)
  ├── Graphics/      (SpriteComponent, Mesh3DComponent, etc.)
  ├── Physics/       (Physics3DComponent, CarPhysicsComponent, etc.)
  ├── UI/            (ButtonComponent, PanelComponent, etc.)
  └── SceneSpecific/ (TetrisPhysicsComponent, etc. - consider deprecating)
```

#### 3.2 Move Scene-Specific Components
Move components that are only used in one scene to that scene's directory:
```
Scenes/
  ├── PhysicsTetrisScene/
  │   ├── PhysicsTetrisScene.h
  │   ├── PhysicsTetrisScene.cpp
  │   └── TetrisPhysicsComponent.h  (moved here)
```

### Phase 4: Graphics Pipeline Cleanup (1-2 days)

#### 4.1 Audit Pipeline Usage
- Check which pipelines are actually used
- Remove unused pipelines
- Consider lazy loading for rarely-used pipelines

#### 4.2 Pipeline Manager
Create a `PipelineManager` to handle pipeline creation/retrieval:
```cpp
class PipelineManager {
public:
    GraphicsPipelineState* getPipeline(PipelineType type);
    GraphicsPipelineState* getOrCreatePipeline(PipelineType type, PipelineDesc desc);
    // Centralized pipeline management
};
```

### Phase 5: Long-term Architecture (Ongoing)

#### 5.1 Plugin System for Scenes
Allow scenes to be loaded as plugins/DLLs, enabling:
- Dynamic scene loading
- Easier scene distribution
- Better separation of concerns

#### 5.2 Scene Templates
Create scene templates for common patterns:
- `PhysicsScene` base class
- `FluidSimulationScene` base class
- `GameScene` base class

#### 5.3 Resource Management
- Centralize texture/model loading
- Implement resource caching
- Reduce duplicate asset loading across scenes

---

## Immediate Action Items (Start Here)

### Priority 1: Reduce Scene Count
1. **Audit scenes**: Which ones are actually needed?
2. **Consolidate similar scenes**: 
   - PhysicsTetrisScene + JellyTetrisReduxScene → TetrisScene (with modes)
   - ThreeDTestScene + ThreeDBridgeGameScene → consolidate or remove
3. **Archive unused scenes**: Move to `Archive/Scenes/` instead of deleting

### Priority 2: Break Down Large Files
1. **Split PowderScene.cpp** into modules (see Phase 2.2)
2. **Review other large scene files** and split if >1000 lines

### Priority 3: Eliminate Code Duplication
1. **Create SceneHelper class** (see Phase 1.2)
2. **Refactor all scenes** to use helper methods
3. **Extract common patterns** into base classes

### Priority 4: Clean Up Libraries
1. **Remove duplicate ImGui** (keep imgui-docking)
2. **Update includes** if needed

---

## Metrics to Track

- **Total scene count**: Target < 8 scenes
- **Average scene file size**: Target < 500 lines per file
- **Code duplication**: Measure with tools like CPD (Copy-Paste Detector)
- **Compile time**: Should improve as code is better organized
- **Build size**: Should decrease as unused code is removed

---

## Risk Mitigation

1. **Version Control**: Ensure all changes are committed frequently
2. **Incremental Changes**: Don't refactor everything at once
3. **Testing**: Test each scene after refactoring
4. **Backup**: Keep a branch with the original code

---

## Estimated Timeline

- **Phase 1 (Quick Wins)**: 1-2 days
- **Phase 2 (Scene System)**: 3-5 days  
- **Phase 3 (Components)**: 2-3 days
- **Phase 4 (Pipelines)**: 1-2 days
- **Phase 5 (Long-term)**: Ongoing

**Total for Phases 1-4**: ~7-12 days of focused work

---

## Questions to Answer Before Starting

1. Which scenes are actively used vs. test/demo scenes?
2. Are there any scenes you want to keep but rarely use? (Archive them)
3. What's the target architecture? (Game engine vs. tech demo collection)
4. Are there plans to distribute scenes separately? (Affects plugin system priority)




