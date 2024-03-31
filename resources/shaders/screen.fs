#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture1;
uniform float exposure;
uniform float gamma;

void main()
{
    vec3 color = vec3(texture(texture1, fs_in.TexCoords));
//     vec3 mapped = vec3(1.0) - exp(-color * exposure);
    vec3 mapped = color;
    FragColor = vec4(pow(mapped, vec3(1.0/gamma)), 1.0);
}