//#shader vertex
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
}

//#shader fragment
#version 330 core

in vec2 TexCoords;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;// fragments brighter than 1.0 bloom

uniform sampler2D gPosition;// rgb = world position, a = neon glow mask
uniform sampler2D gNormal;// rgb = world normal
uniform sampler2D gAlbedoSpec;// rgb = base color, a = specular strength

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;// cos(inner cone angle)
    float outerCutOff;// cos(outer cone angle)
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float constant;
    float linear;
    float quadratic;
};

uniform PointLight pointLight;// room light
uniform SpotLight spotLight;// lamp light
uniform vec3 viewPos;
uniform float shininess;

uniform float neonMix;// 0 = normal lighting, 1 = full neon look
uniform float neonIntensity;// neon brightness boost

vec3 calc_point_light(PointLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec3 base_color, float spec_strength) {
    vec3 light_dir = normalize(light.position - frag_pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);

    float diff = max(dot(normal, light_dir), 0.0);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), shininess);

    float dist = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);

    vec3 ambient = light.ambient * base_color;
    vec3 diffuse = light.diffuse * diff * base_color;
    vec3 specular = light.specular * spec * spec_strength;

    return (ambient + diffuse + specular) * attenuation;
}

vec3 calc_spot_light(SpotLight light, vec3 normal, vec3 frag_pos, vec3 view_dir, vec3 base_color, float spec_strength) {
    vec3 light_dir = normalize(light.position - frag_pos);
    vec3 halfway_dir = normalize(light_dir + view_dir);

    float diff = max(dot(normal, light_dir), 0.0);
    float spec = pow(max(dot(normal, halfway_dir), 0.0), shininess);

    float dist = length(light.position - frag_pos);
    float attenuation = 1.0 / (light.constant + light.linear * dist + light.quadratic * dist * dist);

    // how far inside the cone this fragment is, with a soft falloff between the inner/outer angle
    float theta = dot(light_dir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float cone_intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient = light.ambient * base_color;
    vec3 diffuse = light.diffuse * diff * base_color * cone_intensity;
    vec3 specular = light.specular * spec * spec_strength * cone_intensity;

    return (ambient + diffuse + specular) * attenuation;
}

void main(){
    vec3 fragPos = texture(gPosition, TexCoords).rgb;
    float glow = texture(gPosition, TexCoords).a;
    vec3 normal = normalize(texture(gNormal, TexCoords).rgb);
    vec4 albedoSpec = texture(gAlbedoSpec, TexCoords);
    vec3 baseColor = albedoSpec.rgb;
    float specStrength = albedoSpec.a;

    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 result = calc_point_light(pointLight, normal, fragPos, viewDir, baseColor, specStrength)
                + calc_spot_light(spotLight, normal, fragPos, viewDir, baseColor, specStrength);

    // neon mode: dark except a glowing outline around each sticker
    vec3 neonLook = baseColor * neonIntensity * glow;
    result = mix(result, neonLook, neonMix);

    FragColor = vec4(result, 1.0);

    float brightness = max(result.r, max(result.g, result.b));
    if (brightness > 1.0) {
        BrightColor = vec4(result, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
