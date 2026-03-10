#ifdef THEENGINE_VERTEX
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aColor;

uniform mat4 worldMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec4 vColor;

void main()
{
    vec4 worldPos = vec4(aPosition, 1.0) * worldMatrix;
    vec4 viewPos = worldPos * viewMatrix;
    gl_Position = viewPos * projectionMatrix;
    vColor = aColor;
}
#endif

#ifdef THEENGINE_FRAGMENT
in vec4 vColor;

uniform vec4 tintColor;

out vec4 FragColor;

void main()
{
    FragColor = vColor * tintColor;
}
#endif
