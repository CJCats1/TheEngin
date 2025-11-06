# TestScene Spawn Panel Guide

## Overview
The TestScene now includes an ImGui panel that allows you to spawn different physics objects for testing and experimentation.

## Features

### SoftGuy Objects
- **Spawn Soft Circle**: Creates a deformable circular soft body
- **Spawn Soft Rectangle**: Creates a deformable rectangular soft body  
- **Spawn Soft Triangle**: Creates a deformable triangular soft body
- **Spawn Soft Line**: Creates a deformable line/rope soft body

### FirmGuy Objects
- **Spawn Firm Circle**: Creates a rigid circular physics body
- **Spawn Firm Rectangle**: Creates a rigid rectangular physics body

### Physics Controls
- **Gravity Slider**: Adjust gravity from -5000 to 0 (default: -2000)
- **Reset All Physics**: Resets all SoftGuy objects to their starting positions
- **Entity Count Display**: Shows the current number of SoftGuy and FirmGuy objects

## Usage

1. **Open the Spawn Panel**: The ImGui panel appears automatically when running TestScene
2. **Click Spawn Buttons**: Click any spawn button to create objects at the current mouse position
3. **Adjust Physics**: Use the gravity slider to change the physics environment
4. **Reset When Needed**: Use the reset button to clear all spawned objects

## Object Properties

### SoftGuy Objects
- **Soft Circles**: Orange nodes, 50px radius, 8 segments
- **Soft Rectangles**: Cyan nodes, 100x80px, 4x3 grid
- **Soft Triangles**: Magenta nodes, 60px size
- **Soft Lines**: Gray nodes, 100px length, 5 segments

### FirmGuy Objects
- **Firm Circles**: 25px radius, 2.0 mass, 0.7 restitution
- **Firm Rectangles**: 30x30px, 2.5 mass, 0.5 restitution

## Interaction
- SoftGuy objects can collide with FirmGuy objects
- All objects are affected by gravity
- Objects can interact with each other through the physics system
- Use WASD to move the camera, Q/E to zoom

## Tips
- Spawn objects at different heights to see gravity effects
- Create multiple objects to see collision interactions
- Adjust gravity to see different physics behaviors
- Use the entity count to track how many objects you've created
