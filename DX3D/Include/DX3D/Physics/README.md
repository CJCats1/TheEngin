# AABB Physics System

This physics system is based on the excellent collision detection algorithms from [noonat.github.io/intersect](https://noonat.github.io/intersect/#aabb-vs-aabb).

## Features

- **AABB vs Point** - Check if a point is inside an AABB
- **AABB vs Segment** - Raycast/line segment intersection with AABB
- **AABB vs AABB** - Static collision detection between two AABBs
- **AABB vs Swept AABB** - Moving object collision detection (prevents tunneling)

## Quick Start

```cpp
#include <DX3D/Physics/AABBPhysics.h>
#include <DX3D/Physics/PhysicsSystem.h>

using namespace dx3d;
using namespace dx3d::physics;

// Create a moving box
AABB movingBox(Vec2(100.0f, 100.0f), Vec2(25.0f, 25.0f)); // center, half-size

// Create static obstacles
std::vector<AABB> obstacles;
obstacles.push_back(AABB(Vec2(200.0f, 100.0f), Vec2(50.0f, 50.0f)));
obstacles.push_back(AABB(Vec2(150.0f, 200.0f), Vec2(30.0f, 30.0f)));

// Move the box and resolve collisions
Vec2 movement(50.0f, 0.0f); // Move right
Vec2 finalPos = PhysicsSystem::moveAABB(movingBox, movement, obstacles);

// Or use sweep test for more control
Sweep sweep = movingBox.sweepInto(obstacles, movement);
if (sweep.hit) {
    // Collision occurred at sweep.time (0.0 to 1.0)
    // sweep.pos is the final position
    // sweep.hit contains collision details (normal, delta, etc.)
}
```

## Converting Between Engine Types

The physics system uses `physics::AABB` which is compatible with `dx3d::AABB`:

```cpp
// Convert engine AABB to physics AABB
dx3d::AABB engineAABB(Vec2(100, 100), Vec2(50, 50));
physics::AABB physicsAABB = PhysicsSystem::toPhysicsAABB(engineAABB);

// Convert back
dx3d::AABB back = PhysicsSystem::toEngineAABB(physicsAABB);
```

## Hit Information

When a collision occurs, the `Hit` object contains:
- `pos` - Point of contact
- `normal` - Surface normal at contact point
- `delta` - Overlap vector (can be added to position to resolve collision)
- `time` - For sweep tests, fraction along the path (0.0 to 1.0)

## Sweep Tests

Sweep tests are recommended for moving objects as they prevent tunneling and handle multiple collisions correctly:

```cpp
AABB player(Vec2(100, 100), Vec2(10, 10));
Vec2 velocity(200.0f, 0.0f); // pixels per second
Vec2 delta = velocity * dt; // movement this frame

Sweep sweep = player.sweepInto(obstacles, delta);
player.pos = sweep.pos; // Update position
```

