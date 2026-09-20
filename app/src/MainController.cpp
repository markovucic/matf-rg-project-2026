#include <app/MainController.hpp>
#include <engine/core/Engine.hpp>
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
    glm::vec3 start_position(6.0f, 4.5f, 6.0f);
    camera->Position = start_position;

    glm::vec3 direction_to_center = glm::normalize(glm::vec3(0.0f) - start_position);
    camera->Yaw = glm::degrees(glm::atan(direction_to_center.z, direction_to_center.x));
    camera->Pitch = glm::degrees(glm::asin(direction_to_center.y));
    camera->rotate_camera(0.0f, 0.0f);// refresh Front/Right/Up from the new Yaw/Pitch

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
    if (!click_is_on_gui && platform->key(engine::platform::MOUSE_BUTTON_LEFT).state() == engine::platform::Key::State::JustPressed) {
        m_camera_control_enabled = !m_camera_control_enabled;
        // tried GLFW_CURSOR_DISABLED here, but it made the camera spiral out of control
        m_skip_next_mouse_delta = true;
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
}

void MainController::begin_draw() {
    engine::core::Controller::get<engine::graphics::PostProcessController>()->begin_scene_capture();
    engine::graphics::OpenGL::clear_buffers();
}

void MainController::draw() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("rubiks");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());
    shader->set_vec3("viewPos", graphics->camera()->Position);
    set_light_uniforms(shader);

    shader->set_float("neonMix", m_neon_active ? 1.0f : 0.0f);
    shader->set_float("neonIntensity", m_neon_intensity);
    shader->set_float("neonEdgeOffset", m_neon_edge_offset);
    shader->set_float("neonEdgeWidth", m_neon_edge_width);

    m_rubiks_cube->draw(shader);
}

void MainController::set_light_uniforms(engine::resources::Shader *shader) {
    shader->set_float("shininess", m_shininess);
    shader->set_float("specularStrength", m_specular_strength);

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

    ImGui::Separator();
    ImGui::Text("Neon (N to toggle)");
    ImGui::SliderFloat("Neon intensity", &m_neon_intensity, 1.0f, 8.0f);
    ImGui::SliderFloat("Edge offset", &m_neon_edge_offset, 0.0f, 0.4f);
    ImGui::SliderFloat("Edge width", &m_neon_edge_width, 0.005f, 0.15f);
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
}