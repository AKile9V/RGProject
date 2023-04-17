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
uniform float far_plane;
uniform samplerCube depthMap;
uniform bool shadows;

uniform vec3 viewPos;
uniform Material material;
uniform DirLight dirLight;
uniform SpotLight spotLight;
uniform PointLight pointLight;
uniform vec3 cameraPos;

float ShadowCalculation(vec3 fragPos);
vec4 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec4 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec4 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

vec4 allAmbient = vec4(0.0);


void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec4 result = CalcDirLight(dirLight, norm, viewDir);
    result += CalcSpotLight(spotLight, norm, FragPos, viewDir);
    result += CalcPointLight(pointLight, norm, FragPos, viewDir);
    result += allAmbient;
    float shadow = shadows ? ShadowCalculation(FragPos) : 0.0;

    if(result.a/9.0 < 0.7)
       discard;
    result -= allAmbient;
	FragColor = vec4((allAmbient.xyz)+(result.xyz)*(1.0-shadow), 1.0);
}

float ShadowCalculation(vec3 fragPos)
{
    // get vector between fragment position and light position
    vec3 fragToLight = fragPos - pointLight.position;
    // ise the fragment to light vector to sample from the depth map
    float closestDepth = texture(depthMap, fragToLight).r;
    // it is currently in linear range between [0,1], let's re-transform it back to original depth value
    closestDepth *= far_plane;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    // test for shadows
    float bias = 0.05; // we use a much larger bias since depth is now in [near_plane, far_plane] range
    float shadow = currentDepth -  bias > closestDepth ? 1.0 : 0.0;
    // display closestDepth as debug (to visualize depth cubemap)
    // FragColor = vec4(vec3(closestDepth / far_plane), 1.0);

    return shadow;
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
    allAmbient += ambient;
    return (diffuse + specular);
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
    allAmbient += ambient;
    return (diffuse + specular);
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
    allAmbient += ambient;
    return (diffuse + specular);
}
