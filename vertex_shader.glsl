#version 330 

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 incol;
layout (location = 3) in vec3 Normal;
layout (location = 2) in vec2 vTex;

uniform mat4 MVP;

out vec3 outcol;
out vec3 outNormal;
out vec2 outTex;
out vec3 FragPos;

void main()
{
    FragPos = aPos;
    outcol=incol;
    gl_Position = MVP*vec4(aPos.x, aPos.y, aPos.z, 1.0);
    outNormal = Normal;
    outTex = vTex;
}