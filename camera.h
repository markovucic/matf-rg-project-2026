//
// Created by W10 on 9/16/2026.
//

#ifndef RG_CAMERA_H
#define RG_CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera {

    glm::vec3 position{};
    glm::vec3 front{};
    glm::vec3 up{};

    void _move(glm::vec3 displacement) {
        position += displacement;
        front += displacement;
    }

public:
    glm::vec3 UP = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 DOWN = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 LEFT = glm::vec3(-1.0f, 0.0f, 0.0f);
    glm::vec3 RIGHT = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 FRONT = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 BACK = glm::vec3(0.0f, 0.0f, 1.0f);


    Camera(glm::vec3 _position, glm::vec3 _front, glm::vec3 _up): position(_position), front(_front), up(_up) {};

    Camera() {
        position = glm::vec3(0.0f, 0.0f, 0.0f);
        front = glm::vec3(0.0f, 0.0f, -1.0f);
        up = glm::vec3(0.0f, 1.0f, 0.0f);
    };

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(position, position + front, up);
    }

    void move(glm::vec3 direction, float speed) {
        _move(speed * direction);
    }

};



#endif //RG_CAMERA_H
