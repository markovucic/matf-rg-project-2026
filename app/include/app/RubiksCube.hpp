#ifndef APP_RUBIKSCUBE_HPP
#define APP_RUBIKSCUBE_HPP

#include <engine/resources/Model.hpp>
#include <engine/resources/Shader.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace app {
enum class CubeAxis { X,
                      Y,
                      Z };

struct SubCube {
    glm::mat4 transform{1.0f};
    glm::ivec3 grid_position{};// where it currently sits in the grid, updates every move
    glm::ivec3 home_position{}; // where it started out - from this we deduce which faces are stickers.
};

// where one sticker's glowing edge currently sits in the world, and its color - used to
// give the neon strips real point lights that move and turn with the cubie they belong to
struct GlowSource {
    glm::vec3 position;
    glm::vec3 color;
};

// 27 little cubes arranged in a grid, all pointing at the same Model.
// update() advances whatever layer rotation is currently animating.
class RubiksCube {
public:
    explicit RubiksCube(engine::resources::Model *cube_model, float spacing = 1.05f);

    // ignored if a rotation is already playing, so you can't spam two layers at once
    void start_rotation(CubeAxis axis, int layer, float angle_deg);

    void update(float delta_time);

    void draw(const engine::resources::Shader *shader);

    // one entry per sticker (54 for a solved 3x3x3), following each cubie's current
    // transform - so a light stays attached to its own sticker through every rotation
    std::vector<GlowSource> glow_sources() const;

    bool is_rotating() const {
        return m_is_rotating;
    }

private:
    void update_grid_positions(CubeAxis axis, int layer, int direction);

    SubCube m_cubes[3][3][3];
    // Model instead of Mesh because engine::resources::Model only hands out meshes()
    // as const, and Mesh::draw isn't a const method - so we just hold the whole model
    engine::resources::Model *m_cube_model;

    bool m_is_rotating{false};
    CubeAxis m_active_axis{CubeAxis::Y};
    int m_active_layer{0};
    float m_current_angle{0.0f};
    float m_target_angle{0.0f};
    float m_rotation_speed{300.0f};
};
}// namespace app
#endif//APP_RUBIKSCUBE_HPP
