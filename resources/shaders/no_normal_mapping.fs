#version 330 core
out vec4 FragColor;

#define NUM_LIGHTS 2

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 ViewPos;
    vec3 LightPos[NUM_LIGHTS];
} fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_emission1;

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform float shininess;
uniform Light lights[NUM_LIGHTS];

vec3 BlinnPhong(Light light, vec3 normal, vec3 viewDir, vec2 texCoords, vec3 lightPos)
{
    vec3 lightDir = normalize(lightPos  - fs_in.FragPos);

    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, texCoords));

    vec3 diffuse = light.diffuse * max(dot(normal, lightDir), 0.0) * vec3(texture(texture_diffuse1, texCoords));

    vec3 halfwayDir = normalize(lightDir + viewDir);
    vec3 specular = light.specular * pow(max(dot(normal, halfwayDir), 0.0), shininess)
            * texture(texture_specular1, texCoords).xxx;

    float distance = length(lightPos - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return (ambient + diffuse + specular) * attenuation;
}

void main()
{
    vec3 viewDir = normalize(fs_in.ViewPos - fs_in.FragPos);
    vec2 texCoords = fs_in.TexCoords;

    vec3 normal = vec3(0.0, 0.0, 1.0);

    vec3 color = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < NUM_LIGHTS; i++) {
        color += BlinnPhong(lights[i], normal, viewDir, texCoords, fs_in.LightPos[i]);
    }

    float alpha = texture(texture_diffuse1, texCoords).a;
    FragColor = vec4(color, alpha);
}