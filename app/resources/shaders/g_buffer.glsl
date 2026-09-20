//#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;

out vec3 pos;
out vec3 WorldNormal;
out vec3 WorldTangent;
out vec3 LocalNormal;
out vec3 LocalPos;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    pos = vec3(model * vec4(aPos, 1.0));

    mat3 normalMatrix = mat3(transpose(inverse(model)));
    // this one rotates with the cube, used for lighting
    WorldNormal = normalMatrix * aNormal;
    WorldTangent = normalMatrix * aTangent;

    // this one stays fixed relative to the sub cube, so the sticker color doesn't spin with it
    LocalNormal = aNormal;
    LocalPos = aPos;// same idea, used to find how close a fragment is to a face's edge
    TexCoords = aTexCoords;

    gl_Position = projection * view * vec4(pos, 1.0);
}

//#shader fragment
#version 330 core

in vec3 pos;
in vec3 WorldNormal;
in vec3 WorldTangent;
in vec3 LocalNormal;
in vec3 LocalPos;
in vec2 TexCoords;

// the geometry pass of deferred shading
layout (location = 0) out vec4 gPosition;// rgb = world position, a = neon glow mask
layout (location = 1) out vec4 gNormal;// rgb = world normal (bumped by the normal map)
layout (location = 2) out vec4 gAlbedoSpec;// rgb = base color, a = specular strength

uniform vec3 homePos;// where this sub cube STARTED, -1/0/1 per axis

// textures: diffuse adds subtle surface variation, specular marks
// where the shinier neon tube band is, normal bulges that same band outward
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_normal1;

uniform float specularStrength;
uniform float tubeSpecularStrength;
uniform float neonEdgeOffset;
uniform float neonEdgeWidth;
// false for meshes with no material of their own - skips
// sampling the cubie's textures/tangent
uniform bool useMaterialMaps;

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

void main(){
    vec3 localNorm = normalize(LocalNormal);
    vec3 worldNorm = normalize(WorldNormal);

    // both localNorm and homePos are fixed to the subcube and never change as it moves around
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

    float glow = isSticker ? edge_glow_factor(LocalPos, localNorm) : 0.0;

    vec3 bumpedNormal = worldNorm;
    float tubeMask = 0.0;

    if (useMaterialMaps) {
        baseColor *= texture(texture_diffuse1, TexCoords).rgb;// subtle plastic surface variation

        // bulge the neon tube band outward using the normal map, in tangent space
        vec3 tangent = normalize(WorldTangent - dot(WorldTangent, worldNorm) * worldNorm);
        vec3 bitangent = cross(worldNorm, tangent);
        mat3 TBN = mat3(tangent, bitangent, worldNorm);
        vec3 sampledNormal = texture(texture_normal1, TexCoords).rgb * 2.0 - 1.0;
        bumpedNormal = normalize(TBN * sampledNormal);

        tubeMask = texture(texture_specular1, TexCoords).r;
    }

    // the hidden plastic body shouldn't be as shiny as stickers
    float faceSpecularStrength = isSticker ? specularStrength : specularStrength * 0.2;
    faceSpecularStrength = mix(faceSpecularStrength, tubeSpecularStrength, tubeMask);

    gPosition = vec4(pos, glow);
    gNormal = vec4(bumpedNormal, 1.0);
    gAlbedoSpec = vec4(baseColor, faceSpecularStrength);
}
