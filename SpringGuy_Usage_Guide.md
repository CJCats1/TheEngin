# SpringGuy System Usage Guide

## Overview
The SpringGuy system provides spring-based physics simulation with nodes and beams, similar to the existing PhysicsComponent system but designed to work alongside the FirmGuy physics system.

## Components

### SpringGuyNodeComponent
Represents a physics node in the spring system.

**Key Methods:**
- `SpringGuyNodeComponent(Vec2 position, bool positionFixed = false)` - Constructor
- `setPosition(const Vec2& pos)` / `getPosition()` - Position management
- `setVelocity(const Vec2& vel)` / `getVelocity()` - Velocity management
- `setPositionFixed(bool fixed)` / `isPositionFixed()` - Static node control
- `addExternalForce(const Vec2& force)` - Apply external forces
- `clearExternalForces()` - Clear accumulated forces

### SpringGuyBeamComponent
Represents a spring beam connecting two nodes.

**Key Methods:**
- `SpringGuyBeamComponent(Entity* node1Entity, Entity* node2Entity)` - Constructor
- `setStiffness(float stiffness)` / `getStiffness()` - Spring stiffness
- `setDamping(float damping)` / `getDamping()` - Damping coefficient
- `setMaxForce(float maxForce)` / `getMaxForce()` - Maximum force before breaking
- `setEnabled(bool enabled)` / `getEnabled()` - Enable/disable beam
- `isBroken()` / `setBroken(bool broken)` - Breaking state

### SpringGuySystem
Manages the physics simulation for all SpringGuy components.

**Key Methods:**
- `updateNodes(EntityManager& entityManager, float dt)` - Update node physics
- `updateBeams(EntityManager& entityManager, float dt)` - Update beam physics
- `resetPhysics(EntityManager& entityManager)` - Reset all physics state

## Integration with FirmGuy System

The SpringGuy system automatically integrates with the FirmGuy system for collision detection:

- **FirmGuy vs SpringGuy**: FirmGuy objects can collide with SpringGuy nodes
- **Collision Types**: Circle-FirmGuy vs SpringGuy nodes, Rectangle-FirmGuy vs SpringGuy nodes
- **Physics Response**: Proper impulse-based collision response between systems

## Usage Example

```cpp
// Create SpringGuy nodes
std::vector<Entity*> nodes;
for (int i = 0; i < 5; ++i) {
    auto& nodeEntity = entityManager.createEntity("SpringGuyNode_" + std::to_string(i));
    auto& node = nodeEntity.addComponent<SpringGuyNodeComponent>(
        Vec2(i * 100.0f, 200.0f), 
        i == 0 || i == 4  // Fix end nodes
    );
    nodes.push_back(&nodeEntity);
}

// Create beams between nodes
for (int i = 0; i < 4; ++i) {
    auto& beamEntity = entityManager.createEntity("SpringGuyBeam_" + std::to_string(i));
    auto& beam = beamEntity.addComponent<SpringGuyBeamComponent>(
        nodes[i], nodes[i + 1]
    );
    beam.setStiffness(1000.0f);
    beam.setDamping(80.0f);
}

// Update systems in your scene
void MyScene::fixedUpdate(float dt) {
    // Update SpringGuy physics
    SpringGuySystem::updateNodes(*entityManager, dt);
    SpringGuySystem::updateBeams(*entityManager, dt);
    
    // Update FirmGuy physics (includes SpringGuy collision detection)
    FirmGuySystem::update(*entityManager, dt);
}
```

## Configuration

### Spring Parameters
- **Stiffness**: How rigid the spring is (default: 1000.0f)
- **Damping**: Energy loss in the spring (default: 80.0f)
- **Max Force**: Force threshold for breaking (default: 1000.0f)

### Node Parameters
- **Mass**: Affects how nodes respond to forces (default: 1.0f for SpringGuy nodes)
- **Position Fixed**: Makes nodes static (like anchors)

## Collision Detection

The system automatically handles collisions between:
- FirmGuy circles and SpringGuy nodes
- FirmGuy rectangles and SpringGuy nodes
- Proper impulse-based response with restitution and friction

## Visual Integration

SpringGuy components work with SpriteComponent for rendering:
- Nodes can have sprites for visualization
- Beams automatically update their sprites based on node positions
- Stress visualization through beam thickness and color

## Performance Notes

- SpringGuy system uses the same efficient collision detection as FirmGuy
- Sub-stepping and solver iterations ensure stable simulation
- Automatic sprite synchronization for visual components
