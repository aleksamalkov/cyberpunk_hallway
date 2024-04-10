#version 330 core

out vec4 FragColor;

in VS_OUT {
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture_diffuse1;

void main()
{
    vec3 color = texture(texture_diffuse1, fs_in.TexCoords).rgb;
    // check whether fragment output is higher than threshold, if so output as brightness color
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.0)
        FragColor = vec4(color, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}