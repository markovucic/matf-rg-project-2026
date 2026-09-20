//#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 pos;
out vec3 WorldNormal;
out vec3 LocalNormal;
out vec3 LocalPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    pos = vec3(model * vec4(aPos, 1.0));

    // this one rotates with the cube, used for lighting
    WorldNormal = mat3(transpose(inverse(model))) * aNormal;

    // this one stays fixed relative to the sub cube, so the sticker color doesn't spin with it
    LocalNormal = aNormal;
    LocalPos = aPos;// same idea, used to find how close a fragment is to a face's edge

    gl_Position = projection * view * vec4(pos, 1.0);
}

//#shader fragment
#version 330 core

in vec3 pos;
in vec3 WorldNormal;
in vec3 LocalNormal;
in vec3 LocalPos;

layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 BrightColor;// fragments brighter than 1.0 bloom

uniform vec3 homePos;// where this sub cube STARTED, -1/0/1 per axis

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
uniform float specularStrength;

uniform float neonMix;// 0 = normal lighting, 1 = full neon look
uniform float neonIntensity;// neon brightness boost
uniform float neonEdgeOffset;
uniform float neonEdgeWidth;

// distance-based glow
float edge_glow_factor(vec3 local_pos, vec3 local_norm) {
    float u, v;
    if (abs(local_norm.x) > 0.8) {
        u = local_pos.y;
        v = local_pos.z;
    } else if (abs(local_norm.y) > 0.8) {
        u = local_pos.x;
        v = local_pos.z;
    } else {
        u = local_pos.x;
        v = local_pos.y;
    }

    float edge_dist = min(0.5 - abs(u), 0.5 - abs(v));
    return 1.0 - smoothstep(0.0, neonEdgeWidth, abs(edge_dist - neonEdgeOffset));
}

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
    vec3 localNorm = normalize(LocalNormal);
    vec3 worldNorm = normalize(WorldNormal);

    // both localNorm and homePos are fixed to the subcube and never change as it moves around,
    bool isSticker = (localNorm.x > 0.8 && homePos.x > 0.5) || (localNorm.x < -0.8 && homePos.x < -0.5)
                   || (localNorm.y > 0.8 && homePos.y > 0.5) || (localNorm.y < -0.8 && homePos.y < -0.5)
                   || (localNorm.z > 0.8 && homePos.z > 0.5) || (localNorm.z < -0.8 && homePos.z < -0.5);

    vec3 baseColor = vec3(0.02);// dark plastic body

    if (isSticker) {
        if (abs(localNorm.x) > 0.8) {
            baseColor = (localNorm.x > 0.0) ? vec3(0.9, 0.0, 0.0)
                                           : vec3(1.0, 0.4, 0.0);
        }
        else if (abs(localNorm.y) > 0.8) {
            baseColor = (localNorm.y > 0.0) ? vec3(0.95, 0.95, 0.95)
                                           : vec3(0.9, 0.8, 0.0);
        }
        else if (abs(localNorm.z) > 0.8) {
            baseColor = (localNorm.z > 0.0) ? vec3(0.0, 0.7, 0.1)
                                           : vec3(0.0, 0.2, 0.8);
        }
    }

    // the hidden plastic body shouldn't be as shiny as stickrs
    float faceSpecularStrength = isSticker ? specularStrength : specularStrength * 0.2;

    vec3 viewDir = normalize(viewPos - pos);
    vec3 result = calc_point_light(pointLight, worldNorm, pos, viewDir, baseColor, faceSpecularStrength)
                + calc_spot_light(spotLight, worldNorm, pos, viewDir, baseColor, faceSpecularStrength);

    // neon mode: dark except a glowing outline around each sticker,
    float glow = isSticker ? edge_glow_factor(LocalPos, localNorm) : 0.0;
    vec3 neonLook = baseColor * neonIntensity * glow;
    result = mix(result, neonLook, neonMix);

    FragColor = vec4(result, 1.0);

    float brightness = dot(result, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        BrightColor = vec4(result, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
