#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture_diffuse1;

float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{
    vec2 tex_offset = 1.0 / textureSize(texture_diffuse1, 0); // gets size of single texel
    vec3 result = texture(texture_diffuse1, fs_in.TexCoords).rgb * weight[0]; // current fragment's contribution
    for(int i = 1; i < 5; ++i)
    {
        result += texture(texture_diffuse1, fs_in.TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        result += texture(texture_diffuse1, fs_in.TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
    }
    FragColor = vec4(result, 1.0);
}