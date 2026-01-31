#ifdef THEENGINE_VERTEX
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
layout(location = 3) in vec4 aColor;

uniform mat4 worldMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

out vec2 vUV;
out vec4 vColor;

void main()
{
    vec4 worldPos = vec4(aPosition, 1.0) * worldMatrix;
    vec4 viewPos = worldPos * viewMatrix;
    gl_Position = viewPos * projectionMatrix;
    vUV = aUV;
    vColor = aColor;
}
#endif

#ifdef THEENGINE_FRAGMENT
in vec2 vUV;
in vec4 vColor;

uniform sampler2D tex;
uniform vec4 tintColor;

out vec4 FragColor;

void main()
{
    vec4 texColor = texture(tex, vUV);
    vec3 blended = mix(texColor.rgb, tintColor.rgb, tintColor.a);
    FragColor = vec4(blended, texColor.a) * vColor;
}
#endif
