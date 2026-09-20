#ifndef APP_MAINCONTROLLER_HPP
#define APP_MAINCONTROLLER_HPP

#include <app/RubiksCube.hpp>
#include <engine/core/Engine.hpp>
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

    std::unique_ptr<RubiksCube> m_rubiks_cube;
    // click left mouse button to grab/release the camera, otherwise moving the mouse
    // while rotating a layer would also spin the camera around
    bool m_camera_control_enabled{false};

    bool m_skip_next_mouse_delta{false};
};
}// namespace app
#endif//APP_MAINCONTROLLER_HPP
