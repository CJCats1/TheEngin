// OpenGL background dots shader
// Vertex and fragment shaders are combined in this file and selected
// via THEENGINE_VERTEX / THEENGINE_FRAGMENT defines at compile time.

#ifdef THEENGINE_VERTEX

void main()
{
    // Fullscreen triangle based on gl_VertexID, no vertex buffer required.
    vec2 pos;
    if (gl_VertexID == 0)
        pos = vec2(-1.0, -1.0);
    else if (gl_VertexID == 1)
        pos = vec2(-1.0, 3.0);
    else
        pos = vec2(3.0, -1.0);

    gl_Position = vec4(pos, 0.0, 1.0);
}

#endif // THEENGINE_VERTEX


#ifdef THEENGINE_FRAGMENT

out vec4 FragColor;

// Signed-distance to a circle of radius r.
float sdCircle(vec2 p, float r)
{
    return length(p) - r;
}

void main()
{
    // Pixel coordinates in window space.
    vec2 pix = gl_FragCoord.xy;

    // Match defaults from GraphicsEngine::renderBackgroundDots.
    float dotSpacing = 40.0;
    float dotRadius  = 1.2;
    vec4  baseColor  = vec4(0.27, 0.39, 0.55, 1.0);
    vec4  dotColor   = vec4(0.20, 0.32, 0.46, 0.6);

    // Nearest grid cell center.
    vec2 cell  = floor(pix / dotSpacing) * dotSpacing + dotSpacing * 0.5;
    vec2 local = pix - cell;

    float d    = sdCircle(local, dotRadius);
    float edge = fwidth(d) * 1.5;
    float mask = 1.0 - clamp(smoothstep(0.0, edge, d), 0.0, 1.0);

    vec3 color = mix(baseColor.rgb, dotColor.rgb, mask * dotColor.a);
    FragColor  = vec4(color, 1.0);
}

#endif // THEENGINE_FRAGMENT

