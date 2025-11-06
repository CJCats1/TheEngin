# SoftGuy System Usage Guide

## Overview
The SoftGuy system provides an easy-to-use interface for creating soft bodies that combine `FrameComponent` with `SpringGuy` components. It's designed to make soft body creation simple and intuitive for users.

## Key Features
- **Easy Creation**: Simple factory methods for common shapes
- **Frame Integration**: Combines FrameComponent with SpringGuy physics
- **Collision Support**: Automatic collision detection with FirmGuy objects
- **Configurable**: Customizable stiffness, damping, and visual properties
- **Multiple Shapes**: Circle, Rectangle, Triangle, Line, and Custom shapes

## Quick Start

### Basic Usage
```cpp
// Create a simple soft circle
auto* softCircle = SoftGuyComponent::createCircle(
    entityManager, "MySoftCircle", 
    Vec2(100.0f, 200.0f), 50.0f, 8
);

// Create a soft rectangle
auto* softRect = SoftGuyComponent::createRectangle(
    entityManager, "MySoftRect",
    Vec2(200.0f, 200.0f), Vec2(100.0f, 80.0f), 4, 3
);
```

### With Custom Configuration
```cpp
SoftGuyConfig config;
config.stiffness = 1500.0f;        // How rigid the soft body is
config.damping = 100.0f;           // Energy loss
config.maxForce = 1000.0f;         // Breaking force
config.nodeColor = Vec4(1.0f, 0.5f, 0.0f, 1.0f);  // Orange nodes
config.beamColor = Vec4(1.0f, 0.3f, 0.0f, 0.8f);   // Orange beams
config.showBeams = true;
config.showNodes = true;

auto* softBody = SoftGuyComponent::createCircle(
    entityManager, "CustomSoftCircle", 
    Vec2(0.0f, 0.0f), 60.0f, 12, config
);
```

## Factory Methods

### 1. createCircle()
Creates a soft circular body.
```cpp
SoftGuyComponent* createCircle(
    EntityManager& em, 
    const std::string& name, 
    Vec2 position, 
    float radius, 
    int segments = 8, 
    const SoftGuyConfig& config = SoftGuyConfig()
);
```

### 2. createRectangle()
Creates a soft rectangular body.
```cpp
SoftGuyComponent* createRectangle(
    EntityManager& em, 
    const std::string& name, 
    Vec2 position, 
    Vec2 size, 
    int segmentsX = 4, 
    int segmentsY = 4, 
    const SoftGuyConfig& config = SoftGuyConfig()
);
```

### 3. createTriangle()
Creates a soft triangular body.
```cpp
SoftGuyComponent* createTriangle(
    EntityManager& em, 
    const std::string& name, 
    Vec2 position, 
    float size, 
    const SoftGuyConfig& config = SoftGuyConfig()
);
```

### 4. createLine()
Creates a soft line/rope body.
```cpp
SoftGuyComponent* createLine(
    EntityManager& em, 
    const std::string& name, 
    Vec2 start, 
    Vec2 end, 
    int segments = 3, 
    const SoftGuyConfig& config = SoftGuyConfig()
);
```

### 5. createCustom()
Creates a custom soft body from node positions and connections.
```cpp
SoftGuyComponent* createCustom(
    EntityManager& em, 
    const std::string& name, 
    Vec2 position, 
    const std::vector<Vec2>& nodePositions, 
    const std::vector<std::pair<int, int>>& connections, 
    const SoftGuyConfig& config = SoftGuyConfig()
);
```

## Configuration Options

### SoftGuyConfig Structure
```cpp
struct SoftGuyConfig {
    float stiffness = 1000.0f;      // Spring stiffness
    float damping = 80.0f;          // Energy damping
    float maxForce = 1000.0f;       // Breaking force threshold
    float nodeMass = 1.0f;          // Mass of each node
    float nodeRadius = 14.0f;       // Visual radius of nodes
    Vec4 nodeColor = Vec4(0.0f, 1.0f, 0.0f, 1.0f);  // Node color
    Vec4 beamColor = Vec4(0.0f, 0.8f, 0.0f, 0.8f);  // Beam color
    bool showBeams = true;          // Show beam connections
    bool showNodes = true;          // Show nodes
};
```

## SoftGuy Management

### Position and Movement
```cpp
// Set/get position
softGuy->setPosition(Vec2(100.0f, 200.0f));
Vec2 pos = softGuy->getPosition();

// Set/get velocity
softGuy->setVelocity(Vec2(50.0f, -100.0f));
Vec2 vel = softGuy->getVelocity();

// Set/get rotation
softGuy->setRotation(1.57f);  // 90 degrees
float rot = softGuy->getRotation();

// Set/get angular velocity
softGuy->setAngularVelocity(2.0f);
float angVel = softGuy->getAngularVelocity();
```

### Physics Control
```cpp
// Make static (immovable)
softGuy->setStatic(true);

// Apply forces
softGuy->addForce(Vec2(100.0f, 0.0f));  // Push right
softGuy->addTorque(50.0f);              // Rotate clockwise

// Check if broken
if (softGuy->isBroken()) {
    // Handle broken soft body
}
```

### State Management
```cpp
// Reset to initial state
softGuy->reset();

// Destroy the soft body
softGuy->destroy();

// Show/hide
softGuy->setVisible(false);
```

## Integration with Physics Systems

### In Your Scene's fixedUpdate()
```cpp
void MyScene::fixedUpdate(float dt) {
    // Update SoftGuy physics
    SoftGuySystem::update(*entityManager, dt);
    
    // Update FirmGuy physics (includes SoftGuy collision detection)
    FirmGuySystem::update(*entityManager, dt);
}
```

### Collision Detection
SoftGuy objects automatically collide with FirmGuy objects:
- **Circle FirmGuy** vs SoftGuy nodes
- **Rectangle FirmGuy** vs SoftGuy nodes
- Proper impulse-based collision response

## Example: Creating a Soft Body Physics Scene

```cpp
void MyScene::createSoftBodyDemo() {
    // Create different types of soft bodies
    
    // 1. Bouncy ball
    SoftGuyConfig ballConfig;
    ballConfig.stiffness = 1500.0f;
    ballConfig.damping = 100.0f;
    ballConfig.nodeColor = Vec4(1.0f, 0.5f, 0.0f, 1.0f); // Orange
    
    auto* ball = SoftGuyComponent::createCircle(
        *entityManager, "BouncyBall", 
        Vec2(-200.0f, 300.0f), 40.0f, 8, ballConfig
    );
    
    // 2. Soft box
    SoftGuyConfig boxConfig;
    boxConfig.stiffness = 2000.0f;
    boxConfig.damping = 120.0f;
    boxConfig.nodeColor = Vec4(0.0f, 0.8f, 1.0f, 1.0f); // Cyan
    
    auto* box = SoftGuyComponent::createRectangle(
        *entityManager, "SoftBox",
        Vec2(200.0f, 300.0f), Vec2(80.0f, 60.0f), 3, 2, boxConfig
    );
    
    // 3. Soft rope
    SoftGuyConfig ropeConfig;
    ropeConfig.stiffness = 800.0f;
    ropeConfig.damping = 50.0f;
    ropeConfig.nodeColor = Vec4(0.5f, 0.5f, 0.5f, 1.0f); // Gray
    
    auto* rope = SoftGuyComponent::createLine(
        *entityManager, "SoftRope",
        Vec2(-100.0f, 200.0f), Vec2(100.0f, 200.0f), 6, ropeConfig
    );
    
    // 4. Create some rigid bodies to interact with
    auto& rigidBall = entityManager->createEntity("RigidBall");
    auto& rigidPhysics = rigidBall.addComponent<FirmGuyComponent>();
    rigidPhysics.setCircle(20.0f);
    rigidPhysics.setPosition(Vec2(0.0f, 500.0f));
    rigidPhysics.setVelocity(Vec2(0.0f, -200.0f));
    rigidPhysics.setMass(2.0f);
    rigidPhysics.setRestitution(0.8f);
}
```

## Performance Tips

1. **Segment Count**: Use fewer segments for better performance
   - Circle: 6-12 segments
   - Rectangle: 3x3 to 5x5 grid
   - Line: 3-8 segments

2. **Stiffness**: Higher stiffness = more rigid, lower = more flexible
   - Soft: 500-1000
   - Medium: 1000-2000  
   - Rigid: 2000-5000

3. **Damping**: Controls energy loss
   - Low damping: 20-50 (bouncy)
   - Medium damping: 50-100 (normal)
   - High damping: 100-200 (dampened)

## Common Use Cases

- **Soft Balls**: Bouncy, deformable spheres
- **Soft Boxes**: Flexible containers
- **Ropes/Chains**: Flexible line segments
- **Cloth Simulation**: Grid-based soft surfaces
- **Jelly Objects**: Soft, jiggly shapes
- **Soft Constraints**: Flexible connections between objects

The SoftGuy system makes it incredibly easy to create realistic soft body physics with just a few lines of code!
