//
// Created by W10 on 9/13/2026.
//
#include <cassert>

#include "fstream"
#include "iostream"
#include "sstream"
#include "string"

#include "glad/glad.h"
#include "GLFW/glfw3.h"

#ifndef RG_SHADER_H
#define RG_SHADER_H

inline std::string readFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();

    assert(!buffer.str().empty() && "File empty");

    return buffer.str();
}

class Shader {
    unsigned int program;

public:

    Shader(const std::string& vertexShaderPath, const std::string& fragmentShaderPath) {

        unsigned vertexShader = _initializeShader(vertexShaderPath, GL_VERTEX_SHADER);
        unsigned fragmentShader = _initializeShader(fragmentShaderPath, GL_FRAGMENT_SHADER);

        int success;
        char infoLog[512];
        // Linkovanje Shading Programa
        program = glCreateProgram();
        glAttachShader(program, vertexShader);
        glAttachShader(program, fragmentShader);
        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 512, nullptr, infoLog);
            std::cout << "ERROR: LINKING FAILED\n" << infoLog << std::endl;
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);



    }

    static unsigned _initializeShader(const std::string &shaderPath, int shaderType) {

        std::string shaderSource = readFile(shaderPath);
        const char* shaderCode = shaderSource.c_str();

        unsigned shader = glCreateShader(shaderType);
        glShaderSource(shader, 1, &shaderCode, nullptr);
        glCompileShader(shader);

        int success = 0;
        char infoLog[512];
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cout << "ERROR: CMP FAILED FOR SHADER ON PATH " << shaderPath << ": \n" << infoLog << std::endl;
        }

        return shader;
    }

    void use() const {
        glUseProgram(program);
    }

    void setBool(const std::string &name, bool value) const {
        glUniform1i(glGetUniformLocation(program, name.c_str()), (int)value);
    }


    void setUniform4f(const std::string& name, float x, float y, float z, float w) const {
        int uniformId = glGetUniformLocation(program, name.c_str());
        glUniform4f(uniformId, x, y, z, w);
    }

    void setUniform3f(const std::string& name, float x, float y, float z) const {
        int uniformId = glGetUniformLocation(program, name.c_str());
        glUniform3f(uniformId, x, y, z);
    }

    void setUniform1f(const std::string& name, float x) const {
        int uniformId = glGetUniformLocation(program, name.c_str());
        glUniform1f(uniformId, x);
    }

    void setUniform1i(const std::string& name, int x) const {
        int uniformId = glGetUniformLocation(program, name.c_str());
        glUniform1i(uniformId, x);
    }

    void setUniformMatrix4f(const std::string& name, const glm::mat4& matrix) const {
        int uniformId = glGetUniformLocation(program, name.c_str());
        glUniformMatrix4fv(uniformId, 1, GL_FALSE, &matrix[0][0]);
    }


    void deleteProgram() {
        glDeleteProgram(program);
        program = 0;
    }

};





#endif //RG_SHADER_H
