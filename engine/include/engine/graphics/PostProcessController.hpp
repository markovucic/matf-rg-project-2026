/**
 * @file PostProcessController.hpp
 * @brief Defines the PostProcessController class that implements HDR bloom post-processing.
 */

#ifndef POSTPROCESSCONTROLLER_HPP
#define POSTPROCESSCONTROLLER_HPP

#include <cstdint>
#include <engine/core/Controller.hpp>
#include <engine/platform/PlatformEventObserver.hpp>

namespace engine::graphics {
/**
* @class PostProcessController
* @brief Implements HDR bloom
*
* The scene is rendered into an HDR framebuffer with two color attachments: the normal
* scene color, and a "bright" color that the app's own shader is responsible for writing
* to whenever a fragment should bloom. The bright color attachment is blurred with a
* separable Gaussian blur over a number of ping-pong passes, then additively composited
* back with the scene color, tone-mapped, and gamma-corrected onto the default framebuffer.
*
*/
class PostProcessController final : public core::Controller {
public:
    std::string_view name() const override {
        return "PostProcessController";
    }

    /**
    * @brief Binds the HDR framebuffer so that subsequent draw calls render into it
    * instead of the default framebuffer. Call this at the start of the draw phase.
    */
    void begin_scene_capture();

    /**
    * @brief Blurs the bright color attachment (if bloom is enabled), then composites the
    * scene and blurred bloom together onto the default framebuffer.
    */
    void end_scene_capture_and_composite();

    /**
    * @brief Recreates the framebuffers for a new window size. Called automatically on
    * window resize, but exposed publicly for the platform event observer to call.
    */
    void resize(int width, int height);

    /**
    * @brief Enables or disables the bloom effect. When disabled, the scene is still
    * captured and composited, but without the blurred bright pass added on top.
    */
    void set_bloom_enabled(bool enabled) {
        m_bloom_enabled = enabled;
    }

    /**
    * @returns Whether bloom is currently enabled.
    */
    bool is_bloom_enabled() const {
        return m_bloom_enabled;
    }

    /**
    * @brief Use this to read or change the exposure used by the tone-mapping step.
    * @returns A mutable reference to the exposure value.
    */
    float &exposure() {
        return m_exposure;
    }

    /**
    * @brief Use this to read or change how many ping-pong blur passes are done. Must be
    * an even number for the blurred result to end up in the expected ping-pong texture.
    * @returns A mutable reference to the blur pass count.
    */
    int &blur_passes() {
        return m_blur_passes;
    }

private:
    void initialize() override;

    void terminate() override;

    void create_framebuffers(int width, int height);

    void destroy_framebuffers();

    void blur_bright_texture();

    void composite_to_default_framebuffer();

    uint32_t m_hdr_fbo{0};
    uint32_t m_scene_color_texture{0};
    uint32_t m_bright_color_texture{0};
    uint32_t m_depth_renderbuffer{0};

    uint32_t m_pingpong_fbo[2]{0, 0};
    uint32_t m_pingpong_texture[2]{0, 0};
    uint32_t m_blurred_bright_texture{0};

    uint32_t m_screen_quad_vao{0};

    bool m_bloom_enabled{true};
    float m_exposure{1.0f};
    int m_blur_passes{10};

    int m_width{0};
    int m_height{0};
};

/**
* @class PostProcessPlatformEventObserver
* @brief Observes changes in window size in order to resize the post-process framebuffers.
*/
class PostProcessPlatformEventObserver final : public platform::PlatformEventObserver {
public:
    explicit PostProcessPlatformEventObserver(PostProcessController *post_process)
        : m_post_process(post_process) {
    }

    void on_window_resize(int width, int height) override;

private:
    PostProcessController *m_post_process;
};
}// namespace engine::graphics
#endif//POSTPROCESSCONTROLLER_HPP
