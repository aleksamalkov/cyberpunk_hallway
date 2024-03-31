#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture1;
uniform float gamma;

void main()
{
    FragColor = texture(texture1, fs_in.TexCoords);
    FragColor.rgb = pow(FragColor.rgb, vec3(1.0/gamma));
}