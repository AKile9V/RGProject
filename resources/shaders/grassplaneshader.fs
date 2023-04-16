#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

struct Material {
    sampler2D diffuse;
    sampler2D specular;
    sampler2D ambient;
    float shininess;
};
struct DirLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct PointLight {
    vec3 position;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
uniform SpotLight spotLight;
uniform PointLight pointLight;

vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec4 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec4 result = CalcDirLight(dirLight, norm, viewDir);
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    result += CalcPointLight(pointLight, norm, FragPos, viewDir);

    if(result.a/9.0 < 0.7)
       discard;
	FragColor = vec4(result.xyz, 1.0);
}

// I'm using vec4 just because I want to save the alpha parameter from texture() function and use it for Blending the grass
// instead of making another shader to do just discard blend. Math logic for advanced light stays the same
vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir+viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);
    // combine results
    vec4 ambient = vec4(light.ambient, 1.0) * texture(material.ambient, TexCoord);
    vec4 diffuse = vec4(light.diffuse * diff, 1.0)  * texture(material.diffuse, TexCoord);
    vec4 specular = vec4(light.specular * spec, 1.0) * texture(material.specular, TexCoord);
    return (ambient + diffuse + specular);
}
// calculates the color when using a spot light.
vec4 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir+viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // spotlight intensity
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);
    // combine results
    float attInt = attenuation * intensity;
    vec4 ambient = vec4(light.ambient * attInt, 1.0) * texture(material.ambient, TexCoord);
    vec4 diffuse = vec4(light.diffuse * diff * attInt, 1.0)  * texture(material.diffuse, TexCoord);
    vec4 specular = vec4(light.specular * spec * attInt, 1.0) * texture(material.specular, TexCoord);
    return (ambient + diffuse + specular);
}

// calculates the color when using a point light.
vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 halfwayDir = normalize(lightDir+viewDir);
    float spec = pow(max(dot(viewDir, halfwayDir), 0.0), material.shininess);
    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec4 ambient = vec4(light.ambient * attenuation, 1.0) * texture(material.ambient, TexCoord);
    vec4 diffuse = vec4(light.diffuse * diff * attenuation, 1.0)  * texture(material.diffuse, TexCoord);
    vec4 specular = vec4(light.specular * spec * attenuation, 1.0) * texture(material.specular, TexCoord);
    return (ambient + diffuse + specular);
}
