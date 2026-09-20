/**
 * @file GBuffer.hpp
 * @brief Defines the GBuffer class that implements the geometry buffer for deferred shading.
 */

#ifndef GBUFFER_HPP
#define GBUFFER_HPP

#include <cstdint>
#include <engine/core/Controller.hpp>
#include <engine/platform/PlatformEventObserver.hpp>

namespace engine::resources {
class Shader;
}

namespace engine::graphics {
/**
* @class GBuffer
* @brief Implements the geometry buffer (G-buffer) used by deferred shading.
*
*/
class GBuffer final : public core::Controller {
public:
    std::string_view name() const override {
        return "GBuffer";
    }

    /**
    * @brief Binds the G-buffer framebuffer and clears it. Call this before drawing the
    * scene's geometry with a shader that writes to the G-buffer's attachments.
    */
    void begin_geometry_pass();

    /**
    * @brief Unbinds the G-buffer framebuffer. Call this once the geometry pass is done.
    */
    void end_geometry_pass();

    /**
    * @brief Binds the G-buffer's textures to texture units 0-2 and sets the matching
    * sampler uniforms on the given shader, ready for a lighting pass to sample them.
    */
    void bind_textures(const resources::Shader *shader);

    /**
    * @brief Draws the full-screen quad used to run the deferred lighting pass over the
    * G-buffer. Call this after binding the lighting shader and its uniforms/textures.
    */
    void draw_quad();

    /**
    * @brief Recreates the G-buffer for a new window size. Called automatically on window
    * resize, but exposed publicly for the platform event observer to call.
    */
    void resize(int width, int height);

private:
    void initialize() override;

    void terminate() override;

    void create_framebuffer(int width, int height);

    void destroy_framebuffer();

    uint32_t m_gbuffer_fbo{0};
    uint32_t m_position_texture{0};
    uint32_t m_normal_texture{0};
    uint32_t m_albedo_spec_texture{0};
    uint32_t m_depth_renderbuffer{0};

    uint32_t m_screen_quad_vao{0};

    int m_width{0};
    int m_height{0};
};

/**
* @class GBufferPlatformEventObserver
* @brief Observes changes in window size in order to resize the G-buffer.
*/
class GBufferPlatformEventObserver final : public platform::PlatformEventObserver {
public:
    explicit GBufferPlatformEventObserver(GBuffer *gbuffer)
        : m_gbuffer(gbuffer) {
    }

    void on_window_resize(int width, int height) override;

private:
    GBuffer *m_gbuffer;
};
}// namespace engine::graphics
#endif//GBUFFER_HPP
