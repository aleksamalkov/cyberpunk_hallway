#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

#define NUM_LIGHTS 2

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vs_out.TexCoords = aTexCoords;
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}