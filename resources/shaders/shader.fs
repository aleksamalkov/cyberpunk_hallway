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
uniform sampler2D texture_normal1;
uniform sampler2D texture_height1;
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
uniform float heightScale;
uniform float minLayers;
uniform float maxLayers;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
    float numLayers = mix(maxLayers, minLayers, max(dot(vec3(0.0, 0.0, 1.0), viewDir), 0.0));
    float layerDepth = 1.0 / numLayers;
    vec2 deltaTexCoords = viewDir.xy * heightScale / numLayers;

    float currentLayerDepth = 0.0;
    vec2  currentTexCoords = texCoords;
    float currentDepthMapValue = texture(texture_height1, currentTexCoords).r;

    while(currentLayerDepth < currentDepthMapValue)
    {
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = 1.0 - texture(texture_height1, currentTexCoords).r;
        currentLayerDepth += layerDepth;
    }

    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = 1.0 - texture(texture_height1, prevTexCoords).r - currentLayerDepth + layerDepth;

    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

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
    vec2 texCoords = ParallaxMapping(fs_in.TexCoords,  viewDir);

    vec3 normal = texture(texture_normal1, texCoords).rgb;
    normal = normalize(normal * 2.0 - 1.0);

    vec3 color = vec3(0.0, 0.0, 0.0);
    for (int i = 0; i < NUM_LIGHTS; i++) {
        color += BlinnPhong(lights[i], normal, viewDir, texCoords, fs_in.LightPos[i]);
    }

    color += texture(texture_emission1, texCoords).rgb;

    FragColor = vec4(color, 1.0);
}