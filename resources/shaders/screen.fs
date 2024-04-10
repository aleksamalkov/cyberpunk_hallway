#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform float exposure;
uniform float gamma;

void main()
{
    vec3 bloomColor = vec3(texture(texture_diffuse1, fs_in.TexCoords));
    vec3 hdrColor = vec3(texture(texture_specular1, fs_in.TexCoords));
    vec3 color = hdrColor + bloomColor;
    vec3 mapped = vec3(1.0) - exp(-color * exposure);
    FragColor = vec4(pow(mapped, vec3(1.0/gamma)), 1.0);
}