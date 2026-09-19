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
#include "Model.h"
#include "RubiksCube.h"

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


float deltaTime = 0.0f;
float lastFrame = 0.0f;

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// static Camera camera;
Camera camera(glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;


// void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
// void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window, RubiksCube &cube, bool &isNeon);


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

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
    // glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback); // <-- Dodaj ovo!


    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    Shader shader("../resources/shaders/vertex_shader.shd", "../resources/shaders/fragment_shader.shd");
    // Geometrija trougla
    // float vertices[] = {
    //     -0.5f, -0.5f, 0.0f,
    //      0.5f, -0.5f, 0.0f,
    //      0.0f,  0.5f, 0.0f,
    // };

    // glm::vec3 lightPos(-2.0f, 3.0f, )
    // Light light1(glm::vec3(-1.0f, 5.0f, -2.0f), glm::vec3(1.0f, 0.8f, 1.0f), 0.1f, 0.4f, 0.3f);
    Light light1(
        glm::vec3(3.0f, 4.0f, 5.0f),
        glm::vec3(1.0f, 1.0f, 1.0f),
        0.3f, 0.8f, 0.5f
    );
    // std::vector<float> cube_vertices = {
    //     // Pozicija (x, y, z)          // Normala (nx, ny, nz)
    //     // 1. ZADNJA STRANICA (Z = -0.5)
    //     -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 0
    //      0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 1
    //      0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 2
    //     -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, // 3
    //
    //     // 2. PREDNJA STRANICA (Z = 0.5)
    //     -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 4
    //      0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 5
    //      0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 6
    //     -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, // 7
    //
    //     // 3. LEVA STRANICA (X = -0.5)
    //     -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, // 8
    //     -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, // 9
    //     -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, // 10
    //     -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, // 11
    //
    //     // 4. DESNA STRANICA (X = 0.5)
    //      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 12
    //      0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, // 13
    //      0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, // 14
    //      0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, // 15
    //
    //     // 5. DONJA STRANICA (Y = -0.5)
    //     -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, // 16
    //      0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, // 17
    //      0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, // 18
    //     -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, // 19
    //
    //     // 6. GORNJA STRANICA (Y = 0.5)
    //     -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, // 20
    //      0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, // 21
    //      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, // 22
    //     -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f  // 23
    // };
    //
    //
    // std::vector<unsigned> cube_indices = {
    //     0, 1, 2,      2, 3, 0,     // Zadnja stranica
    //     4, 5, 6,      6, 7, 4,     // Prednja stranica
    //     8, 9, 10,     10, 11, 8,   // Leva stranica
    //     12, 13, 14,   14, 15, 12,  // Desna stranica
    //     16, 17, 18,   18, 19, 16,  // Donja stranica
    //     20, 21, 22,   22, 23, 20   // Gornja stranica
    // };

    Model cubeModel("../resources/objects/sub_cube.obj");

    if (cubeModel.meshes.empty()) {
        std::cerr << "GRESKA: Model nije ucitan!" << std::endl;
        return -1;
    }

    RubiksCube rubiksCube(&cubeModel.meshes[0]);
    bool isNeonActive = true;

    // cubeModel.setVertices(cube_vertices);
    // cube.setIndices(cube_indices);
    // cube.setAttribute(0, 3);
    // cube.setAttribute(1, 3);
    // cube.enableAttributes();


    // shader.use();
    // glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 1000.0f);
    // shader.setUniformMatrix4f("projection", glm::value_ptr(projection));

    glEnable(GL_DEPTH_TEST);
    // Pronalaženje lokacije uniform varijable
    while (!glfwWindowShouldClose(window)) {
        // Izračunavanje deltaTime-a
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // Obrada unosa sa tastature
        processInput(window, rubiksCube, isNeonActive);

        // Ažuriranje animacije rotacije kocke
        rubiksCube.update(deltaTime);

        // Čišćenje ekrana
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. Aktivacija šejdera
        shader.use();

        int currentW, currentH;
        glfwGetFramebufferSize(window, &currentW, &currentH);
        float aspect = (float)currentW / (float)(currentH > 0 ? currentH : 1);
        // 2. Postavljanje View i Projection matrica
        // glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), aspect, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        // glm::mat4 view = glm::lookAt(glm::vec3(6.0f, 5.0f, 8.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        shader.setUniformMatrix4f("projection", projection);
        shader.setUniformMatrix4f("view", view);

        // 3. POSTAVLJANJE UNIFORM VARIJABLI ZA SVETLO I BOJU (MORA PRE DRAW!)
        // shader.setUniform3f("vColor", 0.2f, 0.7f, 0.5f);
        shader.setUniform3f("lightColor", light1.color.x, light1.color.y, light1.color.z);
        shader.setUniform3f("lightPos", light1.position.x, light1.position.y, light1.position.z);
        shader.setUniform1f("ambientInt", light1.ambient);
        shader.setUniform1f("diffuseInt", light1.diffuse);
        shader.setUniform1f("specularInt", light1.specular);

        // 4. ISCRTAVANJE RUBIKOVE KOCKE
        rubiksCube.draw(shader, isNeonActive);

        // Zamena bafera i događaji
        glfwSwapBuffers(window);
        glfwPollEvents();
        //
        // glfwPollEvents();
        //
        // float timeValue = glfwGetTime();
        //
        // shader.setUniformMatrix4f("view", glm::value_ptr(camera.getViewMatrix()));
        //
        // glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f * glm::sin(timeValue), -0.5f, -5.0f + glm::cos(timeValue * 2)));
        // shader.setUniformMatrix4f("model", glm::value_ptr(model));
        //
        // // Plavičasta pozadina
        // glClearColor(0.2f, 0.2f, 0.5f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //
        // // Postavljanje dinamičke boje trougla
        //
        //
        // // Crtanje trougla
        // cube.drawObject();
        // // glBindVertexArray(VAO);
        // // glDrawElements(GL_TRIANGLES, 36,GL_UNSIGNED_INT, 0);
        //
        // glfwSwapBuffers(window);
    }

    // glDeleteVertexArrays(1, &VAO);
    // glDeleteBuffers(1, &VBO);
    shader.deleteProgram();

    glfwTerminate();
    return EXIT_SUCCESS;
}

void processInput(GLFWwindow *window, RubiksCube &cube, bool &isNeon) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Rotiranje gornjeg sloja (Up - Y osa, layer = 1) na taster U
    if (glfwGetKey(window, GLFW_KEY_U) == GLFW_PRESS) {
        cube.startRotation(CubeAxis::Y, 1, -90.0f);
    }
    // Rotiranje prednjeg sloja (Front - Z osa, layer = 1) na taster F
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        cube.startRotation(CubeAxis::Z, 1, -90.0f);
    }
    // Rotiranje desnog sloja (Right - X osa, layer = 1) na taster R
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
        cube.startRotation(CubeAxis::X, 1, -90.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        cube.startRotation(CubeAxis::X, -1, 90.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        cube.startRotation(CubeAxis::Y, -1, 90.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS) {
        cube.startRotation(CubeAxis::Z, -1, 90.0f);
    }


    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        camera.ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        camera.ProcessKeyboard(DOWN, deltaTime);


}

// void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
//     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
//         glfwSetWindowShouldClose(window, GLFW_TRUE);
//     }
//
//     // float KEY_SPEED_SENSITITVITY = 0.01f;
//     // if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
//     //     camera.move(camera.LEFT, KEY_SPEED_SENSITITVITY);
//     // }
//     //
//     // if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
//     //     camera.move(camera.RIGHT, KEY_SPEED_SENSITITVITY);
//     // }
//     //
//     // if (glfwGetKey(window, GLFW_KEY_DOWN)== GLFW_PRESS) {
//     //     camera.move(camera.DOWN, KEY_SPEED_SENSITITVITY);
//     // }
//     //
//     // if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
//     //     camera.move(camera.UP, KEY_SPEED_SENSITITVITY);
//     // }
// }


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}


// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{

    std::cout << "cursor pos callback, mouse_movement=" << camera.mouse_movement << "\n";

    // Ako NE držimo lijevi klik, kursor se samo pomjera preko ekrana
    if (!camera.mouse_movement)
        return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    // Prvi frejm nakon što je taster pritisnut
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Obrnuto jer Y ide odozdo ka vrhu

    lastX = xpos;
    lastY = ypos;

    // Rotiramo kameru
    camera.ProcessMouseMovement(xoffset, yoffset);
}
// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    float sensitivity = 0.5f;
    camera.ProcessMouseScroll(yoffset * sensitivity);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        camera.mouse_movement = !camera.mouse_movement;

        if (camera.mouse_movement)
        {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            lastX = static_cast<float>(xpos);
            lastY = static_cast<float>(ypos);
            firstMouse = false;
        }
    }
}
// void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
//     // float zoomSpeed = 0.1f;
//     // camera.move(camera.BACK , ((float)yoffset * zoomSpeed));
// }