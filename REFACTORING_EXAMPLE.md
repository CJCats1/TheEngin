# Example: Refactoring a Scene to Use SceneHelper

This document shows how to refactor existing scenes to use the new `SceneHelper` class, eliminating code duplication.

## Before: PowderScene::load() (Original)

```cpp
void PowderScene::load(GraphicsEngine& engine)
{
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    m_entityManager = std::make_unique<EntityManager>();

    // Load texture
    m_nodeTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), L"DX3D/Assets/Textures/node.png");

    // Create camera
    createCamera(engine);

    // Create line renderer for grid debug
    auto& lineEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->enableScreenSpace(false);

    if (auto* linePipeline = engine.getLinePipeline())
    {
        m_lineRenderer->setLinePipeline(linePipeline);
    }

    // Initialize grid
    initializeGrid();
    
    // Initialize particle properties
    initializeParticleProperties();
    
    // Initialize air system
    initializeAirSystem();
}

void PowderScene::createCamera(GraphicsEngine& engine)
{
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(1.0f);
}
```

## After: Using SceneHelper

```cpp
#include <DX3D/Game/SceneHelper.h>

void PowderScene::load(GraphicsEngine& engine)
{
    // Use helper for common initialization
    SceneHelper::initializeScene(engine, m_graphicsDevice, m_entityManager);
    auto& device = *m_graphicsDevice;

    // Load texture
    m_nodeTexture = Texture2D::LoadTexture2D(device.getD3DDevice(), L"DX3D/Assets/Textures/node.png");

    // Create camera using helper
    SceneHelper::createDefaultCamera2D(*m_entityManager, 1.0f, Vec2(0.0f, 0.0f));

    // Create line renderer using helper
    m_lineRenderer = SceneHelper::createLineRenderer(
        *m_entityManager, 
        device, 
        engine, 
        false,  // screenSpace
        true    // visible
    );

    // Initialize scene-specific systems
    initializeGrid();
    initializeParticleProperties();
    initializeAirSystem();
}

// createCamera() method can be removed entirely!
```

## Benefits

1. **Reduced code**: ~15 lines → ~8 lines in `load()`
2. **Eliminated duplication**: Camera and LineRenderer setup code removed
3. **Consistency**: All scenes use the same initialization pattern
4. **Maintainability**: Changes to common setup only need to be made in one place

## Migration Checklist

For each scene, you can now:

- [ ] Add `#include <DX3D/Game/SceneHelper.h>`
- [ ] Replace device/entityManager initialization with `SceneHelper::initializeScene()`
- [ ] Replace camera creation with `SceneHelper::createDefaultCamera2D()` or `createDefaultCamera3D()`
- [ ] Replace LineRenderer creation with `SceneHelper::createLineRenderer()`
- [ ] Remove `createCamera()` method if it only does default setup
- [ ] Test the scene to ensure it still works correctly

## Example: PhysicsTetrisScene

### Before
```cpp
void PhysicsTetrisScene::load(GraphicsEngine& engine) {
    auto& device = engine.getGraphicsDevice();
    m_graphicsDevice = &device;
    m_entityManager = std::make_unique<EntityManager>();

    auto& lineRendererEntity = m_entityManager->createEntity("LineRenderer");
    m_lineRenderer = &lineRendererEntity.addComponent<LineRenderer>(device);
    m_lineRenderer->setVisible(true);
    m_lineRenderer->setPosition(0.0f, 0.0f); 

    // Setup camera
    auto& cameraEntity = m_entityManager->createEntity("MainCamera");
    float screenWidth = GraphicsEngine::getWindowWidth();
    float screenHeight = GraphicsEngine::getWindowHeight();
    auto& camera = cameraEntity.addComponent<Camera2D>(screenWidth, screenHeight);
    camera.setPosition(0.0f, 0.0f);
    camera.setZoom(DEFAULT_CAMERA_ZOOM);
    
    // ... rest of initialization
}
```

### After
```cpp
void PhysicsTetrisScene::load(GraphicsEngine& engine) {
    SceneHelper::initializeScene(engine, m_graphicsDevice, m_entityManager);
    
    m_lineRenderer = SceneHelper::createLineRenderer(
        *m_entityManager, 
        *m_graphicsDevice, 
        engine
    );
    m_lineRenderer->setPosition(0.0f, 0.0f); // Custom setting

    SceneHelper::createDefaultCamera2D(*m_entityManager, DEFAULT_CAMERA_ZOOM);
    
    // ... rest of initialization
}
```

## Next Steps

After refactoring all scenes to use SceneHelper:

1. **Measure impact**: Count lines of code removed
2. **Identify more patterns**: Look for other repeated code across scenes
3. **Create more helpers**: Extract other common patterns (e.g., texture loading, entity creation)




