#include <app/MainController.hpp>
#include <engine/core/Engine.hpp>
#include <engine/graphics/GraphicsController.hpp>

namespace app {
void MainController::initialize() {
    engine::graphics::OpenGL::enable_depth_testing();

    auto resources = engine::core::Controller::get<engine::resources::ResourcesController>();
    m_rubiks_cube = std::make_unique<RubiksCube>(resources->model("sub_cube"));

    // move the camera back a bit so the whole cube actually fits in frame
    engine::core::Controller::get<engine::graphics::GraphicsController>()->camera()->Position =
            glm::vec3(0.0f, 0.0f, 6.0f);
}

bool MainController::loop() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    return platform->key(engine::platform::KEY_ESCAPE).state() != engine::platform::Key::State::JustPressed;
}

void MainController::poll_events() {
    auto platform = engine::core::Controller::get<engine::platform::PlatformController>();
    if (platform->key(engine::platform::MOUSE_BUTTON_LEFT).state() == engine::platform::Key::State::JustPressed) {
        m_camera_control_enabled = !m_camera_control_enabled;
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
    engine::graphics::OpenGL::clear_buffers();
}

void MainController::draw() {
    auto graphics = engine::core::Controller::get<engine::graphics::GraphicsController>();
    auto shader = engine::core::Controller::get<engine::resources::ResourcesController>()->shader("rubiks");

    shader->use();
    shader->set_mat4("projection", graphics->projection_matrix());
    shader->set_mat4("view", graphics->camera()->view_matrix());
    // light is just fixed in place for now, could hook this up to config/gui later
    shader->set_vec3("lightPos", glm::vec3(3.0f, 4.0f, 5.0f));
    shader->set_vec3("lightColor", glm::vec3(1.0f));
    shader->set_float("ambientInt", 0.3f);
    shader->set_float("diffuseInt", 0.8f);

    m_rubiks_cube->draw(shader);
}

void MainController::end_draw() {
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

    // only rotate/zoom the camera with the mouse while it's "grabbed" (see poll_events)
    if (m_camera_control_enabled) {
        auto mouse = platform->mouse();
        camera->rotate_camera(mouse.dx, mouse.dy);
        camera->zoom(mouse.scroll);
    }
}
}// namespace app
