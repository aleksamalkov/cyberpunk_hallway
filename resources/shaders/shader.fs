#version 330 core
out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} fs_in;

uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;

struct Light {
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

struct Material {
    float shininess;
};

#define NUM_LIGHTS 2
uniform Light lights[NUM_LIGHTS];
uniform Material material;
uniform vec3 viewPos;

// TODO: remove this when post-processing is added
uniform float gamma;

vec3 BlinnPhong(Light light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fs_in.FragPos);

    vec3 ambient = light.ambient * vec3(texture(texture_diffuse1, fs_in.TexCoords));

    vec3 diffuse = light.diffuse * max(dot(normal, lightDir), 0.0) * vec3(texture(texture_diffuse1, fs_in.TexCoords));

    vec3 halfwayDir = normalize(lightDir + viewDir);
    vec3 specular = light.specular * pow(max(dot(normal, halfwayDir), 0.0), material.shininess)
            * texture(texture_specular1, fs_in.TexCoords).xxx;

    float distance = length(light.position - fs_in.FragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    return (ambient + diffuse + specular) * attenuation;
}

void main()
{
    vec3 normal = normalize(fs_in.Normal);
    vec3 viewDir = normalize(viewPos - fs_in.FragPos);
    vec3 color = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < NUM_LIGHTS; i++) {
        color += BlinnPhong(lights[i], normal, viewDir);
    }
    FragColor = vec4(color, 1.0);
    // TODO: remove this when post-processing is added
    FragColor.rgb = pow(FragColor.rgb, vec3(1.0/gamma));
}