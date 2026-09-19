#include <app/MainController.hpp>
#include <app/RubiksApp.hpp>

namespace app {
void RubiksApp::app_setup() {
    // just the one controller for now, make sure it runs after the engine finishes its own setup
    auto main_controller = register_controller<MainController>();
    main_controller->after(engine::core::Controller::get<engine::core::EngineControllersEnd>());
}
}// namespace app

int main(int argc, char **argv) {
    return std::make_unique<app::RubiksApp>()->run(argc, argv);
}
