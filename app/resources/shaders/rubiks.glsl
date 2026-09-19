//#shader vertex
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 pos;
out vec3 WorldNormal;
out vec3 LocalNormal;

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

    gl_Position = projection * view * vec4(pos, 1.0);
}

//#shader fragment
#version 330 core

in vec3 pos;
in vec3 WorldNormal;
in vec3 LocalNormal;

out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float ambientInt;
uniform float diffuseInt;

void main(){
    // pick a face color based on which way the sub cube's local normal points
    // (basically hardcoding the sticker colors instead of using a texture)
    vec3 localNorm = normalize(LocalNormal);
    vec3 baseColor = vec3(0.05);

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

    vec3 worldNorm = normalize(WorldNormal);
    vec3 lightDir = normalize(lightPos - pos);

    vec3 ambient = ambientInt * lightColor;

    float diff = max(dot(worldNorm, lightDir), 0.0);
    vec3 diffuse = diffuseInt * diff * lightColor;

    vec3 result = (ambient + diffuse) * baseColor;
    FragColor = vec4(result, 1.0);
}
