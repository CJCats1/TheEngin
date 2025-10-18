# Line Rendering Pipeline

This directory contains dedicated shaders and pipeline for optimized line rendering.

## Files

- `Line.hlsl` - Combined vertex and pixel shader for line rendering
- `Line.vs.hlsl` - Separate vertex shader (alternative)
- `Line.ps.hlsl` - Separate pixel shader (alternative)

## Features

### Line Shader Features
- **Per-vertex colors**: Each line vertex can have its own color
- **Global tint support**: Apply tint to all lines
- **No texture sampling**: Pure color output for maximum performance
- **Optimized for lines**: Minimal overhead compared to general-purpose shaders

### Performance Benefits
- **Reduced shader complexity**: No texture lookups or complex lighting
- **Faster vertex processing**: Streamlined vertex shader
- **Minimal pixel shader**: Simple color pass-through
- **Dedicated pipeline**: No unnecessary state changes

## Usage

The line pipeline is automatically created by GraphicsEngine and can be accessed via:
```cpp
auto* linePipeline = graphicsEngine.getLinePipeline();
lineRenderer.setLinePipeline(linePipeline);
```

## Future Enhancements

Potential improvements to the line shader:
- **Anti-aliasing**: Smoother line edges
- **Distance-based alpha**: Fade effects based on distance
- **Custom line styles**: Dashed, dotted, or patterned lines
- **Thickness-based effects**: Variable line thickness
- **Gradient lines**: Color interpolation along line length

## Technical Details

### Vertex Shader
- Transforms vertices through view and projection matrices
- Applies global tint to vertex colors
- Minimal processing for maximum performance

### Pixel Shader
- Simple color pass-through
- No texture sampling
- No complex lighting calculations
- Optimized for pure color output

### Pipeline State
- Dedicated pipeline state for line rendering
- Optimized blend state for line rendering
- Minimal state changes when switching to line rendering
