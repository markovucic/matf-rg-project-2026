#ifndef RUBIKSAPP_HPP
#define RUBIKSAPP_HPP

#include <engine/core/Engine.hpp>

namespace app {
// entry point into the engine, app_setup runs once before the main loop starts
class RubiksApp final : public engine::core::App {
    void app_setup() override;
};
}// namespace app
#endif//RUBIKSAPP_HPP
