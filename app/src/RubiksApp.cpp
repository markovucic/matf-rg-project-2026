#include <app/MainController.hpp>
#include <app/RubiksApp.hpp>
#include <engine/graphics/GBuffer.hpp>
#include <engine/graphics/PostProcessController.hpp>

namespace app {
void RubiksApp::app_setup() {
    auto main_controller = register_controller<MainController>();
    auto post_process = register_controller<engine::graphics::PostProcessController>();
    auto gbuffer = register_controller<engine::graphics::GBuffer>();
    main_controller->after(engine::core::Controller::get<engine::core::EngineControllersEnd>());
    post_process->after(engine::core::Controller::get<engine::core::EngineControllersEnd>());
    gbuffer->after(engine::core::Controller::get<engine::core::EngineControllersEnd>());
}
}// namespace app

int main(int argc, char **argv) {
    return std::make_unique<app::RubiksApp>()->run(argc, argv);
}
