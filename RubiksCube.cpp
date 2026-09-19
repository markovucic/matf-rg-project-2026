#include "RubiksCube.h"
#include <cmath>

RubiksCube::RubiksCube(Mesh* mesh, float spacing) : cubeMesh(mesh) {
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                int ix = x + 1;
                int iy = y + 1;
                int iz = z + 1;

                cubes[ix][iy][iz].gridPos = glm::ivec3(x, y, z);
                
                // Početna transformacija u prostoru
                cubes[ix][iy][iz].transform = glm::translate(glm::mat4(1.0f), glm::vec3(x * spacing, y * spacing, z * spacing));
            }
        }
    }
}

void RubiksCube::startRotation(CubeAxis axis, int layer, float angleDeg) {
    if (isRotating) return; // Ako se već rotira, ignorišemo novu komandu

    isRotating = true;
    activeAxis = axis;
    activeLayer = layer;
    currentAngle = 0.0f;
    targetAngle = angleDeg;
}

void RubiksCube::update(float deltaTime) {
    if (!isRotating) return;

    // Izračunavanje koraka rotacije u ovoj sekundi
    float step = (targetAngle > 0 ? rotationSpeed : -rotationSpeed) * deltaTime;

    // Sprečavamo da prebaci targetAngle
    if (std::abs(currentAngle + step) >= std::abs(targetAngle)) {
        step = targetAngle - currentAngle;
        currentAngle = targetAngle;
        isRotating = false; // Animacija je završena
    } else {
        currentAngle += step;
    }

    // Određujemo osu oko koje vršimo trenutnu rotaciju
    glm::vec3 rotVector(0.0f);
    if (activeAxis == CubeAxis::X) rotVector.x = 1.0f;
    else if (activeAxis == CubeAxis::Y) rotVector.y = 1.0f;
    else if (activeAxis == CubeAxis::Z) rotVector.z = 1.0f;

    glm::mat4 rotMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(step), rotVector);

    // Primenjujemo privremenu rotaciju na sve kockice iz odabranog sloja
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 3; ++z) {
                SubCube& sc = cubes[x][y][z];

                bool inLayer = false;
                if (activeAxis == CubeAxis::X && sc.gridPos.x == activeLayer) inLayer = true;
                if (activeAxis == CubeAxis::Y && sc.gridPos.y == activeLayer) inLayer = true;
                if (activeAxis == CubeAxis::Z && sc.gridPos.z == activeLayer) inLayer = true;

                if (inLayer) {
                    sc.transform = rotMatrix * sc.transform;
                }
            }
        }
    }

    // Kada se animacija završi, fiksiramo nove diskretne pozicije u mreži
    if (!isRotating) {
        int direction = (targetAngle > 0) ? 1 : -1;
        updateGridPositions(activeAxis, activeLayer, direction);
    }
}

void RubiksCube::updateGridPositions(CubeAxis axis, int layer, int direction) {
    // Sređivanje logičke mreže nakon rotacije za +-90 stepeni
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 3; ++z) {
                SubCube& sc = cubes[x][y][z];

                bool inLayer = false;
                if (axis == CubeAxis::X && sc.gridPos.x == layer) inLayer = true;
                if (axis == CubeAxis::Y && sc.gridPos.y == layer) inLayer = true;
                if (axis == CubeAxis::Z && sc.gridPos.z == layer) inLayer = true;

                if (inLayer) {
                    glm::ivec3 oldPos = sc.gridPos;
                    glm::ivec3 newPos = oldPos;

                    if (axis == CubeAxis::X) {
                        newPos.y = (direction > 0) ? -oldPos.z : oldPos.z;
                        newPos.z = (direction > 0) ? oldPos.y : -oldPos.y;
                    } else if (axis == CubeAxis::Y) {
                        newPos.x = (direction > 0) ? oldPos.z : -oldPos.z;
                        newPos.z = (direction > 0) ? -oldPos.x : oldPos.x;
                    } else if (axis == CubeAxis::Z) {
                        newPos.x = (direction > 0) ? -oldPos.y : oldPos.y;
                        newPos.y = (direction > 0) ? oldPos.x : -oldPos.x;
                    }
                    sc.gridPos = newPos;
                }
            }
        }
    }
}

void RubiksCube::draw(Shader& shader, bool isNeonMode) {
    shader.use();

    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 3; ++z) {
                // Preskačemo unutrašnje jezgro (x=1, y=1, z=1 u 0..2 indeksima)
                if (x == 1 && y == 1 && z == 1) continue;

                SubCube& sc = cubes[x][y][z];
                shader.setUniformMatrix4f("model", sc.transform);

                // Neon efekat za uglove kocke
                bool isCorner = (std::abs(sc.gridPos.x) + std::abs(sc.gridPos.y) + std::abs(sc.gridPos.z)) == 3;

                if (isNeonMode && isCorner) {
                    shader.setUniform3f("emissiveColor", 4.0f, 0.0f, 2.0f);
                    shader.setBool("useNeon", true);
                } else {
                    shader.setUniform3f("emissiveColor", 0.0f, 0.0f, 0.0f);
                    shader.setBool("useNeon", false);
                }

                if (cubeMesh) {
                    cubeMesh->draw(shader);
                }
            }
        }
    }
}