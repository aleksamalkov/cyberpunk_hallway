#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

#define NUM_LIGHTS 2

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 ViewPos;
    vec3 LightPos[NUM_LIGHTS];
} vs_out;

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 viewPos;
uniform Light lights[NUM_LIGHTS];

void main()
{
    vec3 T = normalize(vec3(model * vec4(aTangent, 0.0)));
    vec3 N = normalize(vec3(model * vec4(aNormal, 0.0)));
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 TBN = transpose(mat3(T, B, N));

    vs_out.TexCoords = aTexCoords;
    vs_out.FragPos = TBN * vec3(model * vec4(aPos, 1.0));
    vs_out.ViewPos = TBN * viewPos;

    for (int i = 0; i < NUM_LIGHTS; i++) {
        vs_out.LightPos[i] = TBN * lights[i].position;
    }

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}