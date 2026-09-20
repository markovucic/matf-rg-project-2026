#include <app/MainController.hpp>
#include <engine/core/Engine.hpp>
#include <engine/graphics/GBuffer.hpp>
#include <engine/graphics/GraphicsController.hpp>
#include <engine/graphics/PostProcessController.hpp>
#include <imgui.h>
#include <random>
#include <spdlog/spdlog.h>
#include <string>

namespace app {
void MainController::initialize() {
    engine::graphics::OpenGL::enable_depth_testing();

    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    m_rubiks_cube = std::make_unique<RubiksCube>(resources->model("sub_cube"), 1.05f, 0.1f);// 10x smaller

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
        toggle_neon();
    }

    if (m_scramble_moves_remaining > 0) {
        if (!m_rubiks_cube->is_rotating()) {
            // same 6 moves the U/F/R/L/D/B keys can do, just randomly chosen
            struct ScrambleMove {
                CubeAxis axis;
                int layer;
                float angle_deg;
            };
            static const ScrambleMove MOVES[6] = {
                    {CubeAxis::Y, 1, -90.0f},
                    {CubeAxis::Z, 1, -90.0f},
                    {CubeAxis::X, 1, -90.0f},
                    {CubeAxis::X, -1, 90.0f},
                    {CubeAxis::Y, -1, 90.0f},
                    {CubeAxis::Z, -1, 90.0f}};

            static std::mt19937 rng{std::random_device{}()};
            std::uniform_int_distribution<int> pick_move(0, 5);
            const ScrambleMove &move = MOVES[pick_move(rng)];
            m_rubiks_cube->start_rotation(move.axis, move.layer, move.angle_deg);
            --m_scramble_moves_remaining;
        }
        return;// block manual layer-rotation input for the whole scramble
    }

    // don't queue up another layer rotation while one is still playing
    if (m_rubiks_cube->is_rotating()) {
        return;
    }
    // holding alt reverses the move (e.g. U becomes U')
    bool alt_held = platform->key(engine::platform::KEY_LEFT_ALT).is_down()
                  || platform->key(engine::platform::KEY_RIGHT_ALT).is_down();
    float reverse = alt_held ? -1.0f : 1.0f;

    if (platform->key(engine::platform::KEY_U).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::Y, 1, -90.0f * reverse);
    }
    if (platform->key(engine::platform::KEY_F).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::Z, 1, -90.0f * reverse);
    }
    if (platform->key(engine::platform::KEY_R).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::X, 1, -90.0f * reverse);
    }
    if (platform->key(engine::platform::KEY_L).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::X, -1, 90.0f * reverse);
    }
    if (platform->key(engine::platform::KEY_D).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::Y, -1, 90.0f * reverse);
    }
    if (platform->key(engine::platform::KEY_B).is_down()) {
        m_rubiks_cube->start_rotation(CubeAxis::Z, -1, 90.0f * reverse);
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

    // solid black core filling the gaps between subcubes, no material:
    // skip sampling the cubies' texture maps for it
    constexpr float CORE_SIZE = 1.5f / 10.0f;// follows the cube's own 5x shrink
    g_buffer_shader->set_mat4("model", glm::scale(glm::mat4(1.0f), glm::vec3(CORE_SIZE * 0.96f)));
    g_buffer_shader->set_vec3("homePos", glm::vec3(0.0f));
    g_buffer_shader->set_bool("useMaterialMaps", false);
    resources->model("sphere_core")->draw(g_buffer_shader);

    g_buffer_shader->set_bool("useMaterialMaps", true);
    m_rubiks_cube->draw(g_buffer_shader);

    // desk
    g_buffer_shader->set_bool("useMaterialMaps", false);
    g_buffer_shader->set_bool("isGenericMesh", true);
    glm::mat4 desk_model = glm::translate(glm::mat4(1.0f), m_desk_position);
    desk_model = glm::scale(desk_model, glm::vec3(m_desk_scale));
    g_buffer_shader->set_mat4("model", desk_model);
    g_buffer_shader->set_vec3("genericMeshColor", m_desk_color);
    resources->model("desk")->draw(g_buffer_shader);

    // lamp
    glm::mat4 lamp_model = glm::translate(glm::mat4(1.0f), m_spot_light_pos);
    // setting the model orientation
    lamp_model = glm::rotate(lamp_model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    lamp_model = glm::rotate(lamp_model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    lamp_model = glm::scale(lamp_model, glm::vec3(m_lamp_scale));
    g_buffer_shader->set_mat4("model", lamp_model);
    g_buffer_shader->set_vec3("genericMeshColor", m_lamp_color);
    resources->model("lamp")->draw(g_buffer_shader);
    g_buffer_shader->set_bool("isGenericMesh", false);

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
        constexpr float TWO_PI = 6.28318530718f;
        float pulse_time = m_neon_active_time - pulse_start;
        float phase = TWO_PI * pulse_time / m_neon_pulse_period;
        neon_intensity *= 0.81f + 0.19f * (glm::sin(phase)); // fine-tuned so it looks the best
    }

    lighting_shader->set_float("neonMix", m_neon_mix);
    lighting_shader->set_float("neonIntensity", neon_intensity);

    // one light per sticker, following that sticker's own cubie transform
    auto glow_sources = m_rubiks_cube->glow_sources();
    for (int i = 0; i < static_cast<int>(glow_sources.size()); ++i) {
        lighting_shader->set_vec3("neonLightPositions[" + std::to_string(i) + "]", glow_sources[i].position);
        lighting_shader->set_vec3("neonLightColors[" + std::to_string(i) + "]", glow_sources[i].color);
    }
    lighting_shader->set_float("neonLightStrength", neon_intensity * m_neon_mix * 0.01f);// dimmed a lot

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
    glm::vec3 effective_spot_pos = m_spot_light_pos + m_lamp_offset;
    // fixed aim direction
    glm::vec3 spot_direction = glm::normalize(glm::vec3(-1.0f, -1.0f, 0.0f));
    shader->set_vec3("spotLight.position", effective_spot_pos);
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

    if (m_scramble_moves_remaining > 0) {
        ImGui::BeginDisabled();
        ImGui::Button("Scrambling...");
        ImGui::EndDisabled();
    } else if (ImGui::Button("Scramble the cube")) {
        static std::mt19937 rng{std::random_device{}()};
        std::uniform_int_distribution<int> pick_count(20, 30);
        m_scramble_moves_remaining = pick_count(rng);
        spdlog::info("[EVENT SEQUENCE] ACTION_X (scramble started, {} moves queued)", m_scramble_moves_remaining);
    }

    ImGui::Separator();
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
        toggle_neon();
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

    ImGui::Separator();
    ImGui::Text("Lamp model");
    ImGui::ColorEdit3("Lamp color", &m_lamp_color.x);
    ImGui::SliderFloat3("Lamp bulb offset", &m_lamp_offset.x, -2.0f, 2.0f);
    ImGui::SliderFloat("Lamp scale", &m_lamp_scale, 0.01f, 300.0f);

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

        if (!m_event_a_logged && m_neon_mix >= 1.0f) {
            spdlog::info("[EVENT SEQUENCE] EVENT_A triggered (lights faded out, neon fully on) ---AFTER_N_SECONDS(N={:.1f}s)---Triggers---> EVENT_B", m_neon_hold_duration);
            m_event_a_logged = true;
        }
        if (!m_event_b_logged && m_neon_active_time >= m_neon_fade_duration + m_neon_hold_duration) {
            spdlog::info("[EVENT SEQUENCE] EVENT_B triggered (neon glow now pulsing/breathing)");
            m_event_b_logged = true;
        }
    } else {
        m_neon_active_time = 0.0f;
    }
}

void MainController::toggle_neon() {
    m_neon_active = !m_neon_active;
    m_event_a_logged = false;
    m_event_b_logged = false;

    if (m_neon_active) {
        spdlog::info("[EVENT SEQUENCE] ACTION_X (neon activated) ---AFTER_M_SECONDS(M={:.1f}s)---Triggers---> EVENT_A", m_neon_fade_duration);
    } else {
        spdlog::info("[EVENT SEQUENCE] ACTION_X (neon deactivated) -> fading back to normal lighting over {:.1f}s", m_neon_fade_duration);
    }
}
}