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

    void update_neon_mix();

    void toggle_neon();

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
    glm::vec3 m_spot_light_pos{1.0f, -0.2f, 0.0f};
    // lm::vec3(-0.5f, -4.0f, -2.0f
    glm::vec3 m_spot_light_color{1.0f, 0.95f, 0.8f};
    float m_spot_inner_cutoff_deg{15.0f};
    float m_spot_outer_cutoff_deg{25.0f};

    float m_shininess{15.0f};
    float m_specular_strength{0.4f};
    // the glass/plastic tube housing the neon glow is shinier than sticker
    float m_tube_specular_strength{1.5f};

    bool m_neon_active{false};
    // animated blend between normal lighting (0) and full neon look (1)
    float m_neon_mix{0.0f};
    float m_neon_fade_duration{1.5f};
    // seconds since neon was last turned on, resets to 0 the moment it's turned off
    float m_neon_active_time{0.0f};
    // once the fade-in finishes, glow holds steady for this long before it starts pulsing
    float m_neon_hold_duration{4.0f};
    float m_neon_pulse_period{2.0f};
    // logs EVENT_A/EVENT_B once each per activation instead of every frame
    bool m_event_a_logged{false};
    bool m_event_b_logged{false};
    // brightness multiplier for the glow - color itself comes from each sticker's own color
    float m_neon_intensity{2.5f};
    // how far the glowing line sits inset from the sticker's true edge, and how wide it is -
    // fixed rather than GUI-tunable so it can't be widened enough to overrun the sticker edge
    float m_neon_edge_offset{0.05f};
    float m_neon_edge_width{0.06f};

    glm::vec3 m_desk_position{0.0f, -1.46f, 0.0f};
    float m_desk_scale{300.0f};
    glm::vec3 m_desk_color{0.45f, 0.32f, 0.2f};

    glm::vec3 m_lamp_offset{-0.5f, 0.5f, 0.0f};
    float m_lamp_scale{200.0f};
    glm::vec3 m_lamp_color{0.3f, 0.3f, 0.25f};
};
}// namespace app
#endif//APP_MAINCONTROLLER_HPP
