//
// Created by W10 on 9/12/2026.
//

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <vector>

#include "shader.h"
#include "camera.h"
#include "Object.h"

typedef struct light {
    glm::vec3 position;
    glm::vec3 color;
    float ambient;
    float diffuse;
    float specular;

    light (glm::vec3 position, glm::vec3 color, float ambient, float diffuse, float specular) {
        this->position = position;
        this->color = color;
        this->ambient = ambient;
        this->diffuse = diffuse;
        this->specular = specular;
    }
} Light;


static Camera camera;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

int main() {
    if (!glfwInit()) return EXIT_FAILURE;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_FALSE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "hello_window", nullptr, nullptr);
    if (window == nullptr) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    Shader shader("resources/shaders/vertex_shader.shd", "resources/shaders/fragment_shader.shd");
    // Geometrija trougla
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f,
    };

    // glm::vec3 lightPos(-2.0f, 3.0f, )
    Light light1(glm::vec3(-1.0f, 5.0f, -2.0f), glm::vec3(1.0f, 0.8f, 1.0f), 0.1f, 0.4f, 0.3f);

    std::vector<float> cube_vertices = {
        // Pozicija (x, y, z)          // Normala (nx, ny, nz)
        // 1. ZADNJA STRANICA (Z = -0.5)
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 0
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 1
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 2
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 3

        // 2. PREDNJA STRANICA (Z = 0.5)
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 4
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 5
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 6
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 7

        // 3. LEVA STRANICA (X = -0.5)
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, // 8
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, // 9
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, // 10
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, // 11

        // 4. DESNA STRANICA (X = 0.5)
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 12
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, // 13
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, // 14
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 15

        // 5. DONJA STRANICA (Y = -0.5)
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, // 16
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, // 17
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, // 18
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, // 19

        // 6. GORNJA STRANICA (Y = 0.5)
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, // 20
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, // 21
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, // 22
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f  // 23
    };


    std::vector<unsigned> cube_indices = {
        0, 1, 2,      2, 3, 0,     // Zadnja stranica
        4, 5, 6,      6, 7, 4,     // Prednja stranica
        8, 9, 10,     10, 11, 8,   // Leva stranica
        12, 13, 14,   14, 15, 12,  // Desna stranica
        16, 17, 18,   18, 19, 16,  // Donja stranica
        20, 21, 22,   22, 23, 20   // Gornja stranica
    };

    Object cube;
    cube.setVertices(cube_vertices);
    cube.setIndices(cube_indices);
    cube.setAttribute(0, 3);
    cube.setAttribute(1, 3);
    cube.enableAttributes();

    // unsigned int VBO, VAO, EBO;
    // glGenVertexArrays(1, &VAO);
    // glGenBuffers(1, &VBO);
    // glGenBuffers(1, &EBO);
    //
    // glBindVertexArray(VAO);
    // glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);
    //
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    // glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    //
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    // glEnableVertexAttribArray(0);
    //
    // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));
    // glEnableVertexAttribArray(1);
    //
    // glBindBuffer(GL_ARRAY_BUFFER, 0);
    // glBindVertexArray(0);

    shader.use();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
    shader.setUniformMatrix4f("projection", glm::value_ptr(projection));

    glEnable(GL_DEPTH_TEST);
    // Pronalaženje lokacije uniform varijable
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float timeValue = glfwGetTime();

        shader.setUniformMatrix4f("view", glm::value_ptr(camera.getViewMatrix()));

        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f * glm::sin(timeValue), -0.5f, -5.0f + glm::cos(timeValue * 2)));
        shader.setUniformMatrix4f("model", glm::value_ptr(model));

        // Plavičasta pozadina
        glClearColor(0.2f, 0.2f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Postavljanje dinamičke boje trougla

        shader.setUniform3f("vColor", 0.2f, 0.7f, 0.5f);
        shader.setUniform3f("lightColor", light1.color.x, light1.color.y, light1.color.z);
        shader.setUniform3f("lightPos", light1.position.x, light1.position.y, light1.position.z);
        shader.setUniform1f("ambientInt", light1.ambient);
        shader.setUniform1f("diffuseInt", light1.diffuse);
        shader.setUniform1f("specularInt", light1.specular);

        // Crtanje trougla
        cube.drawObject();
        // glBindVertexArray(VAO);
        // glDrawElements(GL_TRIANGLES, 36,GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
    }

    // glDeleteVertexArrays(1, &VAO);
    // glDeleteBuffers(1, &VBO);
    shader.deleteProgram();

    glfwTerminate();
    return EXIT_SUCCESS;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    float KEY_SPEED_SENSITITVITY = 0.01f;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        camera.move(camera.LEFT, KEY_SPEED_SENSITITVITY);
    }

    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        camera.move(camera.RIGHT, KEY_SPEED_SENSITITVITY);
    }

    if (glfwGetKey(window, GLFW_KEY_DOWN)== GLFW_PRESS) {
        camera.move(camera.DOWN, KEY_SPEED_SENSITITVITY);
    }

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        camera.move(camera.UP, KEY_SPEED_SENSITITVITY);
    }
}


void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    float zoomSpeed = 0.1f;
    camera.move(camera.BACK , ((float)yoffset * zoomSpeed));
}