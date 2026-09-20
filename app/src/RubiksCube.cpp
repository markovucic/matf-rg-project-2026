#include <app/RubiksCube.hpp>
#include <cmath>

namespace app {
RubiksCube::RubiksCube(engine::resources::Model *cube_model, float spacing, float cube_scale)
    : m_cube_model(cube_model) {
    // spacing just pushes the cubes apart a bit so we can see the gaps between them
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                auto &sub_cube = m_cubes[x + 1][y + 1][z + 1];
                sub_cube.grid_position = glm::ivec3(x, y, z);
                sub_cube.home_position = glm::ivec3(x, y, z);
                sub_cube.transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z) * spacing * cube_scale)
                                    * glm::scale(glm::mat4(1.0f), glm::vec3(cube_scale));
            }
        }
    }
}

void RubiksCube::start_rotation(CubeAxis axis, int layer, float angle_deg) {
    if (m_is_rotating) return;

    m_is_rotating = true;
    m_active_axis = axis;
    m_active_layer = layer;
    m_current_angle = 0.0f;
    m_target_angle = angle_deg;
}

void RubiksCube::update(float delta_time) {
    if (!m_is_rotating) return;

    float step = (m_target_angle > 0 ? m_rotation_speed : -m_rotation_speed) * delta_time;
    // don't overshoot the target on the last frame of the animation
    if (std::abs(m_current_angle + step) >= std::abs(m_target_angle)) {
        step = m_target_angle - m_current_angle;
        m_current_angle = m_target_angle;
        m_is_rotating = false;
    } else {
        m_current_angle += step;
    }

    glm::vec3 rotation_axis(0.0f);
    if (m_active_axis == CubeAxis::X) rotation_axis.x = 1.0f;
    else if (m_active_axis == CubeAxis::Y) rotation_axis.y = 1.0f;
    else if (m_active_axis == CubeAxis::Z) rotation_axis.z = 1.0f;

    glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(step), rotation_axis);

    // apply this frame's rotation step to every cube in the active layer
    for (auto &plane: m_cubes) {
        for (auto &row: plane) {
            for (auto &sub_cube: row) {
                bool in_layer = false;
                if (m_active_axis == CubeAxis::X && sub_cube.grid_position.x == m_active_layer) in_layer = true;
                if (m_active_axis == CubeAxis::Y && sub_cube.grid_position.y == m_active_layer) in_layer = true;
                if (m_active_axis == CubeAxis::Z && sub_cube.grid_position.z == m_active_layer) in_layer = true;

                if (in_layer) {
                    sub_cube.transform = rotation * sub_cube.transform;
                }
            }
        }
    }

    // animation just finished, snap the grid coordinates to their new (still integer) spots
    if (!m_is_rotating) {
        int direction = (m_target_angle > 0) ? 1 : -1;
        update_grid_positions(m_active_axis, m_active_layer, direction);
    }
}

void RubiksCube::update_grid_positions(CubeAxis axis, int layer, int direction) {
    for (auto &plane: m_cubes) {
        for (auto &row: plane) {
            for (auto &sub_cube: row) {
                bool in_layer = false;
                if (axis == CubeAxis::X && sub_cube.grid_position.x == layer) in_layer = true;
                if (axis == CubeAxis::Y && sub_cube.grid_position.y == layer) in_layer = true;
                if (axis == CubeAxis::Z && sub_cube.grid_position.z == layer) in_layer = true;

                if (!in_layer) continue;

                // rotate the (x, y, z) grid coords the same way the visual transform just did
                glm::ivec3 old_position = sub_cube.grid_position;
                glm::ivec3 new_position = old_position;

                if (axis == CubeAxis::X) {
                    new_position.y = (direction > 0) ? -old_position.z : old_position.z;
                    new_position.z = (direction > 0) ? old_position.y : -old_position.y;
                } else if (axis == CubeAxis::Y) {
                    new_position.x = (direction > 0) ? old_position.z : -old_position.z;
                    new_position.z = (direction > 0) ? -old_position.x : old_position.x;
                } else if (axis == CubeAxis::Z) {
                    new_position.x = (direction > 0) ? -old_position.y : old_position.y;
                    new_position.y = (direction > 0) ? old_position.x : -old_position.x;
                }
                sub_cube.grid_position = new_position;
            }
        }
    }
}

void RubiksCube::draw(const engine::resources::Shader *shader) {
    for (int x = 0; x < 3; ++x) {
        for (int y = 0; y < 3; ++y) {
            for (int z = 0; z < 3; ++z) {
                if (x == 1 && y == 1 && z == 1) continue;// center piece is never visible, skip drawing it

                shader->set_mat4("model", m_cubes[x][y][z].transform);
                shader->set_vec3("homePos", glm::vec3(m_cubes[x][y][z].home_position));
                m_cube_model->draw(shader);
            }
        }
    }
}

std::vector<GlowSource> RubiksCube::glow_sources() const {
    // matches the color-per-axis table in g_buffer.glsl
    static const glm::vec3 k_colors[3][2] = {
            {glm::vec3(0.9f, 0.0f, 0.0f), glm::vec3(1.0f, 0.4f, 0.0f)},  // x: +red, -orange
            {glm::vec3(0.95f, 0.95f, 0.95f), glm::vec3(0.9f, 0.8f, 0.0f)},// y: +white, -yellow
            {glm::vec3(0.0f, 0.7f, 0.1f), glm::vec3(0.0f, 0.2f, 0.8f)},  // z: +green, -blue
    };

    std::vector<GlowSource> sources;
    for (const auto &plane: m_cubes) {
        for (const auto &row: plane) {
            for (const auto &sub_cube: row) {
                for (int axis = 0; axis < 3; ++axis) {
                    if (sub_cube.home_position[axis] == 0) continue;// no sticker on this axis

                    glm::vec3 local_center(0.0f);
                    local_center[axis] = static_cast<float>(sub_cube.home_position[axis]) * 0.5f;

                    GlowSource source;
                    source.position = glm::vec3(sub_cube.transform * glm::vec4(local_center, 1.0f));
                    source.color = k_colors[axis][sub_cube.home_position[axis] > 0 ? 0 : 1];
                    sources.push_back(source);
                }
            }
        }
    }
    return sources;
}
}// namespace app
