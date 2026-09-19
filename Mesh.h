//
// Created by W10 on 9/19/2026.
//

#ifndef RG_MESH_H
#define RG_MESH_H

#ifndef MESH_H
#define MESH_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include "shader.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    unsigned int VAO = 0, VBO = 0, EBO = 0;

    Mesh(std::vector<Vertex> vert, std::vector<unsigned int> ind, std::vector<Texture> tex = {});

    void draw(Shader &shader) const;

    void cleanup();

private:
    void setupMesh();
};

#endif

#endif //RG_MESH_H
