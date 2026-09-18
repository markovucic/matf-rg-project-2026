//
// Created by W10 on 9/17/2026.
//

#ifndef RG_OBJECT_H
#define RG_OBJECT_H

#include <vector>
#include <string>
#include <glad/glad.h>


class Object {

    unsigned VBO, VAO, EBO;
    unsigned n_attr = 0;
    unsigned attrib_lengths[16] = {};
    unsigned vertexAttribLength = 0;

    std::vector<float> vertices{};
    std::vector<unsigned> indices{};

public:

    Object() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    };

    // Object(Object& o);

    void setVertices(const std::vector<float>& vertices);

    void setIndices(const std::vector<unsigned>& indices);

    int loadFromFile(const std::string &verticesFilename, const std::string &indicesFilename);

    void setAttribute(unsigned index, unsigned length);

    void enableAttributes();

    void drawObject() const;

    ~Object();

    // virtual bool operator==(Object *obj) const;

    // virtual int hashCode() const;
};


#endif //RG_OBJECT_H
