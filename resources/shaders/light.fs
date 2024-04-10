#version 330 core
out vec4 FragColor;

#define NUM_LIGHTS 2

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;
uniform sampler2D texture_height1;
uniform float shininess;

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform Light light;

void main()
{
    vec3 color = texture(texture_diffuse1, fs_in.TexCoords).rgb;
    color *= (light.diffuse + light.ambient + light.specular) / 2.0;
    FragColor = vec4(color, 1.0);
}