#include <app/MainController.hpp>
#include <engine/core/Engine.hpp>
#include <engine/graphics/GBuffer.hpp>
#include <engine/graphics/GraphicsController.hpp>
#include <engine/graphics/PostProcessController.hpp>
#include <imgui.h>

namespace app {
void MainController::initialize() {
    engine::graphics::OpenGL::enable_depth_testing();

    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    m_rubiks_cube = std::make_unique<RubiksCube>(resources->model("sub_cube"));

    // start from corner view
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    glm::vec3 start_position(2.2f, 4.5f, 8.2f);
    camera->Position = start_position;

    glm::vec3 direction_to_center = glm::normalize(glm::vec3(0.0f) - start_position);
    camera->Yaw = glm::degrees(glm::atan(direction_to_center.z, direction_to_center.x));
    camera->Pitch = glm::degrees(glm::asin(direction_to_center.y));
    camera->rotate_camera(0.0f, 0.0f);// refresh Front/Right/Up from the new Yaw/Pitch

    camera->MouseSensitivity *= 0.7f;

    // cursor starts visible/free since camera control starts off
    engine::core::Controller::get<engine::platform::PlatformController>()->set_enable_cursor(true);
}

bool MainController::loop() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    return platform->key(engine::platform::KEY_ESCAPE).state() != engine::platform::Key::State::JustPressed;
}

void MainController::poll_events() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    bool click_is_on_gui = m_show_gui && ImGui::GetIO().WantCaptureMouse;
    auto left_click_state = platform->key(engine::platform::MOUSE_BUTTON_LEFT).state();
    if (!click_is_on_gui && left_click_state == engine::platform::Key::State::JustPressed) {
        // hold left click to look around, release to let go
        m_camera_control_enabled = true;

        platform->set_enable_cursor(false);
        m_skip_next_mouse_delta = true;
    } else if (left_click_state == engine::platform::Key::State::JustReleased) {
        m_camera_control_enabled = false;
        platform->set_enable_cursor(true);
    }

    if (platform->key(engine::platform::KEY_F1).state() == engine::platform::Key::State::JustPressed) {
        m_show_gui = !m_show_gui;
    }

    if (platform->key(engine::platform::KEY_N).state() == engine::platform::Key::State::JustPressed) {
        m_neon_active = !m_neon_active;
    }

    // don't queue up another layer rotation while one is still playing
    if (m_rubiks_cube->is_rotating()) {
        return;
    }
    if (platform->key(engine::platform::KEY_U).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::Y, 1, -90.0f);
    }
    if (platform->key(engine::platform::KEY_F).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::Z, 1, -90.0f);
    }
    if (platform->key(engine::platform::KEY_R).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::X, 1, -90.0f);
    }
    if (platform->key(engine::platform::KEY_L).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::X, -1, 90.0f);
    }
    if (platform->key(engine::platform::KEY_D).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::Y, -1, 90.0f);
    }
    if (platform->key(engine::platform::KEY_B).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::Z, -1, 90.0f);
    }
}

void MainController::update() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    m_rubiks_cube->update(platform->dt());
    update_camera();
    update_neon_mix();
}

void MainController::begin_draw() {
    engine::core::Controller::get<engine::graphics::GBuffer>()->begin_geometry_pass();
}

void MainController::draw() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    auto gbuffer = engine::core::Controller::get<engine::graphics::GBuffer>();

    // geometry pass: fill the G-buffer with material data, no lighting yet
    auto g_buffer_shader = resources->shader("g_buffer");
    g_buffer_shader->use();
    g_buffer_shader->set_mat4("projection", graphics->projection_matrix());
    g_buffer_shader->set_mat4("view", graphics->camera()->view_matrix());
    g_buffer_shader->set_float("specularStrength", m_specular_strength);
    g_buffer_shader->set_float("tubeSpecularStrength", m_tube_specular_strength);
    g_buffer_shader->set_float("neonEdgeOffset", m_neon_edge_offset);
    g_buffer_shader->set_float("neonEdgeWidth", m_neon_edge_width);

    // solid black core filling the gaps between subcubes
    constexpr float core_size = 1.5f;
    g_buffer_shader->set_mat4("model", glm::scale(glm::mat4(1.0f), glm::vec3(core_size * 0.96f)));
    g_buffer_shader->set_vec3("homePos", glm::vec3(0.0f));
    resources->model("sphere_core")->draw(g_buffer_shader);

    m_rubiks_cube->draw(g_buffer_shader);

    gbuffer->end_geometry_pass();

    // lighting pass: read the G-buffer back once per screen pixel instead of once per
    // fragment per light like the old forward-shaded version did
    auto post_process = engine::core::Controller::get<engine::graphics::PostProcessController>();
    post_process->begin_scene_capture();
    engine::graphics::OpenGL::clear_buffers();

    auto lighting_shader = resources->shader("deferred_lighting");
    lighting_shader->use();
    gbuffer->bind_textures(lighting_shader);
    lighting_shader->set_vec3("viewPos", graphics->camera()->Position);
    set_light_uniforms(lighting_shader);

    float neon_intensity = m_neon_intensity;
    float pulse_start = m_neon_fade_duration + m_neon_hold_duration;
    if (m_neon_active_time > pulse_start) {
        constexpr float k_two_pi = 6.28318530718f;
        float pulse_time = m_neon_active_time - pulse_start;
        float phase = k_two_pi * pulse_time / m_neon_pulse_period;
        neon_intensity *= 0.81f + 0.19f * (glm::sin(phase)); // fine-tuned so it looks the best
    }

    lighting_shader->set_float("neonMix", m_neon_mix);
    lighting_shader->set_float("neonIntensity", neon_intensity);

    gbuffer->draw_quad();
}

void MainController::set_light_uniforms(engine::resources::Shader *shader) {
    shader->set_float("shininess", m_shininess);

    shader->set_vec3("pointLight.position", m_point_light_pos);
    shader->set_vec3("pointLight.ambient", m_point_light_color * 0.15f);
    shader->set_vec3("pointLight.diffuse", m_point_light_color * 0.6f);
    shader->set_vec3("pointLight.specular", m_point_light_color);
    shader->set_float("pointLight.constant", 1.0f);
    shader->set_float("pointLight.linear", 0.09f);
    shader->set_float("pointLight.quadratic", 0.032f);

    // aim the lamp at the cube's center, wherever it's currently positioned
    glm::vec3 spot_direction = glm::normalize(glm::vec3(0.0f) - m_spot_light_pos);
    shader->set_vec3("spotLight.position", m_spot_light_pos);
    shader->set_vec3("spotLight.direction", spot_direction);
    shader->set_float("spotLight.cutOff", glm::cos(glm::radians(m_spot_inner_cutoff_deg)));
    shader->set_float("spotLight.outerCutOff", glm::cos(glm::radians(m_spot_outer_cutoff_deg)));
    shader->set_vec3("spotLight.ambient", m_spot_light_color * 0.05f);
    shader->set_vec3("spotLight.diffuse", m_spot_light_color);
    shader->set_vec3("spotLight.specular", m_spot_light_color);
    shader->set_float("spotLight.constant", 1.0f);
    shader->set_float("spotLight.linear", 0.045f);
    shader->set_float("spotLight.quadratic", 0.0075f);
}

void MainController::draw_gui() {
    if (!m_show_gui) {
        return;
    }

    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    graphics->begin_gui();

    ImGui::Begin("Lighting (F1 to hide)");

    ImGui::Text("Room light");
    ImGui::ColorEdit3("Room color", &m_point_light_color.x);
    ImGui::SliderFloat3("Room position", &m_point_light_pos.x, -10.0f, 10.0f);

    ImGui::Separator();
    ImGui::Text("Lamp");
    ImGui::ColorEdit3("Lamp color", &m_spot_light_color.x);
    ImGui::SliderFloat3("Lamp position", &m_spot_light_pos.x, -10.0f, 10.0f);
    ImGui::SliderFloat("Inner cutoff", &m_spot_inner_cutoff_deg, 1.0f, 45.0f);
    if (m_spot_outer_cutoff_deg < m_spot_inner_cutoff_deg) {
        m_spot_outer_cutoff_deg = m_spot_inner_cutoff_deg;
    }
    ImGui::SliderFloat("Outer cutoff", &m_spot_outer_cutoff_deg, m_spot_inner_cutoff_deg, 60.0f);

    ImGui::Separator();
    ImGui::SliderFloat("Shininess", &m_shininess, 2.0f, 256.0f);
    ImGui::SliderFloat("Specular strength", &m_specular_strength, 0.0f, 1.0f);
    ImGui::SliderFloat("Tube specular strength", &m_tube_specular_strength, 0.0f, 2.0f);

    ImGui::Separator();
    ImGui::Text("Neon (N to toggle)");
    ImGui::SliderFloat("Neon intensity", &m_neon_intensity, 1.0f, 8.0f);
    if (ImGui::Button(m_neon_active ? "Turn neon off" : "Turn neon on")) {
        m_neon_active = !m_neon_active;
    }

    ImGui::Separator();
    ImGui::Text("Bloom");
    auto post_process = engine::core::Controller::get<engine::graphics::PostProcessController>();
    bool bloom_enabled = post_process->is_bloom_enabled();
    if (ImGui::Checkbox("Enable bloom", &bloom_enabled)) {
        post_process->set_bloom_enabled(bloom_enabled);
    }
    ImGui::SliderFloat("Exposure", &post_process->exposure(), 0.1f, 5.0f);
    ImGui::SliderFloat("Blur spread", &post_process->blur_spread(), 0.5f, 4.0f);

    ImGui::End();

    graphics->end_gui();
}

void MainController::end_draw() {
    // gui is drawn straight onto the screen, not through the bloom pipeline
    engine::core::Controller::get<engine::graphics::PostProcessController>()->end_scene_capture_and_composite();
    draw_gui();
    engine::core::Controller::get<engine::platform::PlatformController>()->swap_buffers();
}

void MainController::update_camera() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    auto camera = engine::core::Controller::get<engine::graphics::GraphicsController>()->camera();
    float dt = platform->dt();

    if (platform->key(engine::platform::KEY_UP).is_down()) {
        camera->move_camera(engine::graphics::Camera::Movement::FORWARD, dt);
    }
    if (platform->key(engine::platform::KEY_DOWN).is_down()) {
        camera->move_camera(engine::graphics::Camera::Movement::BACKWARD, dt);
    }
    if (platform->key(engine::platform::KEY_LEFT).is_down()) {
        camera->move_camera(engine::graphics::Camera::Movement::LEFT, dt);
    }
    if (platform->key(engine::platform::KEY_RIGHT).is_down()) {
        camera->move_camera(engine::graphics::Camera::Movement::RIGHT, dt);
    }
    if (platform->key(engine::platform::KEY_PAGE_UP).is_down()) {
        camera->move_camera(engine::graphics::Camera::Movement::UP, dt);
    }
    if (platform->key(engine::platform::KEY_PAGE_DOWN).is_down()) {
        camera->move_camera(engine::graphics::Camera::Movement::DOWN, dt);
    }

    // only rotate/zoom the camera with the mouse while it's "grabbed"
    if (m_camera_control_enabled) {
        auto mouse = platform->mouse();
        if (m_skip_next_mouse_delta) {
            m_skip_next_mouse_delta = false;
        } else {
            camera->rotate_camera(mouse.dx, mouse.dy);
        }
        camera->zoom(mouse.scroll);
    }
}

void MainController::update_neon_mix() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    float target = m_neon_active ? 1.0f : 0.0f;
    float step = platform->dt() / m_neon_fade_duration;

    if (m_neon_mix < target) {
        m_neon_mix += step;
        if (m_neon_mix > target) {
            m_neon_mix = target;
        }
    } else if (m_neon_mix > target) {
        m_neon_mix -= step;
        if (m_neon_mix < target) {
            m_neon_mix = target;
        }
    }

    if (m_neon_active) {
        m_neon_active_time += platform->dt();
    } else {
        m_neon_active_time = 0.0f;
    }
}
}