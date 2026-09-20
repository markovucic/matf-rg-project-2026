#ifndef APP_MAINCONTROLLER_HPP
#define APP_MAINCONTROLLER_HPP

#include <app/RubiksCube.hpp>
#include <engine/core/Engine.hpp>
#include <glm/glm.hpp>
#include <memory>

namespace app {
class MainController final : public engine::core::Controller {
public:
    std::string_view name() const override {
        return "app::MainController";
    }

private:
    void initialize() override;

    bool loop() override;

    void poll_events() override;

    void update() override;

    void begin_draw() override;

    void draw() override;

    void end_draw() override;

    void update_camera();

    void set_light_uniforms(engine::resources::Shader *shader);

    void draw_gui();

    std::unique_ptr<RubiksCube> m_rubiks_cube;

    bool m_camera_control_enabled{false};

    bool m_skip_next_mouse_delta{false};

    bool m_show_gui{true};

    // room light - point light with no direction
    glm::vec3 m_point_light_pos{-4.0f, 3.0f, 4.0f};
    glm::vec3 m_point_light_color{0.6f, 0.65f, 0.8f};

    // lamp light - spotlight aimed at the cube from above
    glm::vec3 m_spot_light_pos{2.5f, 4.0f, 3.0f};
    glm::vec3 m_spot_light_color{1.0f, 0.95f, 0.8f};
    float m_spot_inner_cutoff_deg{15.0f};
    float m_spot_outer_cutoff_deg{25.0f};

    float m_shininess{32.0f};
    float m_specular_strength{0.5f};

    bool m_neon_active{false};
    // brightness multiplier for the glow - color itself comes from each sticker's own color
    float m_neon_intensity{2.0f};
    // how far the glowing line sits inset from the sticker's true edge, and how wide it is
    float m_neon_edge_offset{0.05f};
    float m_neon_edge_width{0.02f};
};
}// namespace app
#endif//APP_MAINCONTROLLER_HPP
