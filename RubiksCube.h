//
// Created by W10 on 9/18/2026.
//
#ifndef RUBIKSCUBE_H
#define RUBIKSCUBE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "shader.h"
#include "Mesh.h"

// Ose rotacije
enum class CubeAxis { X, Y, Z };

// Struktura pojedinačne male kockice (SubCube)
struct SubCube {
    glm::mat4 transform = glm::mat4(1.0f);
    glm::ivec3 gridPos; // Trenutna pozicija u mreži (-1, 0, 1 po osama)
};

class RubiksCube {
private:
    SubCube cubes[3][3][3];
    Mesh* cubeMesh; // Pokazivač na geomeriju kocke

    // Varijable za kontrolu animacije rotacije
    bool isRotating = false;
    CubeAxis activeAxis = CubeAxis::Y;
    int activeLayer = 0;       // -1, 0, ili 1
    float currentAngle = 0.0f; // Trenutni ugao animacije
    float targetAngle = 0.0f;  // Ciljni ugao (+90 ili -90 stepeni)
    float rotationSpeed = 300.0f; // Brzina rotacije u stepenima po sekundi

    // Pomoćna metoda za ažuriranje matrica i mreže nakon završene rotacije
    void updateGridPositions(CubeAxis axis, int layer, int direction);

public:
    RubiksCube(Mesh* mesh, float spacing = 1.05f);

    // Glavna metoda za započinjanje rotacije sloja (layer: -1, 0, ili 1, angleDeg: 90 ili -90)
    void startRotation(CubeAxis axis, int layer, float angleDeg);

    // Ažuriranje animacije u render petlji (prima deltaTime iz main-a)
    void update(float deltaTime);

    // Isrtavanje kocke (Opcija B - sa Neon podrškom)
    void draw(Shader& shader, bool isNeonMode);

    // Provera da li je animacija u toku (da se spreči više rotacija odjednom)
    bool getIsRotating() const { return isRotating; }
};

#endif // RUBIKSCUBE_H