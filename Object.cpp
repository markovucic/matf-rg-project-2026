//
// Created by W10 on 9/17/2026.
//

#include "Object.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>


void Object::setVertices(const std::vector<float>& vertices) {
    this->vertices = vertices;

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

}

void Object::setIndices(const std::vector<unsigned>& indices) {
    this->indices = indices;

    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned), indices.data(), GL_STATIC_DRAW);
}


int Object::loadFromFile(const std::string &verticesFilename, const std::string &indicesFilename) {

    std::ifstream vifs(verticesFilename);
    if (!vifs.is_open()) {
        std::cerr << "Error opening file " << verticesFilename << std::endl;
        return 1;
    }

    std::ifstream iifs(indicesFilename);
    if (!iifs.is_open()) {
        std::cerr << "Error opening file " << indicesFilename << std::endl;
        return 1;
    }

    std::vector<float> vertices;
    std::vector<unsigned> indices;
    std::string line;
    while (std::getline(vifs, line)) {

        if (line[0] == '/' && line[1] == '/') continue;
        if (std::all_of(line.begin(), line.end(), [](unsigned char c) {return std::isspace(c);})) continue;
        if (line[0] != '-' && line[0] != '0' && line[0] != '1') {
            std::cerr << "File format not supported: line starts with" << line[0] << std::endl;
            return 2;
        }

        std::stringstream ss(line);
        std::string cell;

        while (std::getline(ss, cell, ',')) {
            vertices.push_back(std::stof(cell));
        }
    }

    while (std::getline(iifs, line)) {

        if (line[0] == '/' && line[1] == '/') continue;
        if (std::all_of(line.begin(), line.end(), [](unsigned char c) {return std::isspace(c);})) continue;
        if (line[0] != '-' && line[0] != '0' && line[0] != '1') {
            std::cerr << "File format not supported: line starts with" << line[0] << std::endl;
            return 2;
        }

        std::stringstream ss(line);
        std::string cell;
        std::vector<std::string> row;

        while (std::getline(ss, cell, ',')) {
            indices.push_back(std::stoi(cell));
        }
    }





    // Primer pristupa podacima iz trenutnog reda:


    // row[0] je 1. kolona, row[1] je 2. kolona...
        std::cout << "Kolona 1: " << row[0] << " | Kolona 2: " << row[1] << '\n';

}

void Object::setAttribute(unsigned index, unsigned length) {
    attrib_lengths[index] = length;
    n_attr++;
    vertexAttribLength += length;
}

void Object::enableAttributes() {

    glBindVertexArray(VAO);
    unsigned vertexOffset = 0;
    for (unsigned i = 0; i < n_attr; i++) {
        glVertexAttribPointer(
            i, attrib_lengths[i], GL_FLOAT,
            GL_FALSE, vertexAttribLength  * sizeof(float), (void*)(vertexOffset * sizeof(float))
        );
        glEnableVertexAttribArray(i);

        vertexOffset += attrib_lengths[i];
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Object::drawObject() const {
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(),GL_UNSIGNED_INT, 0);
}


Object::~Object() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

}
