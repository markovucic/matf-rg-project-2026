// clang-format off
#include <glad/glad.h>
// clang-format on
#include <cstdint>
#include <engine/graphics/OpenGL.hpp>
#include <engine/graphics/PostProcessController.hpp>
#include <engine/platform/PlatformController.hpp>
#include <engine/resources/ResourcesController.hpp>
#include <engine/util/Errors.hpp>
#include <memory>

namespace engine::graphics {

void PostProcessController::initialize() {
    auto platform = core::Controller::get<platform::PlatformController>();
    m_width = platform->window()->width();
    m_height = platform->window()->height();

    create_framebuffers(m_width, m_height);

    float quad_vertices[] = {
            // positions   // texCoords
            -1.0f, 1.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f,
            1.0f, -1.0f, 1.0f, 0.0f,

            -1.0f, 1.0f, 0.0f, 1.0f,
            1.0f, -1.0f, 1.0f, 0.0f,
            1.0f, 1.0f, 1.0f, 1.0f};

    uint32_t quad_vbo = 0;
    CHECKED_GL_CALL(glGenVertexArrays, 1, &m_screen_quad_vao);
    CHECKED_GL_CALL(glGenBuffers, 1, &quad_vbo);
    CHECKED_GL_CALL(glBindVertexArray, m_screen_quad_vao);
    CHECKED_GL_CALL(glBindBuffer, GL_ARRAY_BUFFER, quad_vbo);
    CHECKED_GL_CALL(glBufferData, GL_ARRAY_BUFFER, sizeof(quad_vertices), &quad_vertices, GL_STATIC_DRAW);
    CHECKED_GL_CALL(glEnableVertexAttribArray, 0);
    CHECKED_GL_CALL(glVertexAttribPointer, 0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) 0);// NOLINT
    CHECKED_GL_CALL(glEnableVertexAttribArray, 1);
    CHECKED_GL_CALL(glVertexAttribPointer, 1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *) (2 * sizeof(float)));// NOLINT
    CHECKED_GL_CALL(glBindVertexArray, 0);

    auto observer = std::make_unique<PostProcessPlatformEventObserver>(this);
    platform->register_platform_event_observer(std::move(observer));
}

void PostProcessController::terminate() {
    destroy_framebuffers();
    CHECKED_GL_CALL(glDeleteVertexArrays, 1, &m_screen_quad_vao);
}

void PostProcessController::create_framebuffers(int width, int height) {
    CHECKED_GL_CALL(glGenFramebuffers, 1, &m_hdr_fbo);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_hdr_fbo);

    CHECKED_GL_CALL(glGenTextures, 1, &m_scene_color_texture);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_scene_color_texture);
    CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_scene_color_texture, 0);

    CHECKED_GL_CALL(glGenTextures, 1, &m_bright_color_texture);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_bright_color_texture);
    CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_bright_color_texture, 0);

    uint32_t attachments[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
    CHECKED_GL_CALL(glDrawBuffers, 2, attachments);

    CHECKED_GL_CALL(glGenRenderbuffers, 1, &m_depth_renderbuffer);
    CHECKED_GL_CALL(glBindRenderbuffer, GL_RENDERBUFFER, m_depth_renderbuffer);
    CHECKED_GL_CALL(glRenderbufferStorage, GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    CHECKED_GL_CALL(glFramebufferRenderbuffer, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_renderbuffer);

    RG_GUARANTEE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "HDR framebuffer is not complete.");

    for (int i = 0; i < 2; ++i) {
        CHECKED_GL_CALL(glGenFramebuffers, 1, &m_pingpong_fbo[i]);
        CHECKED_GL_CALL(glGenTextures, 1, &m_pingpong_texture[i]);

        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_pingpong_fbo[i]);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_pingpong_texture[i]);
        CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_pingpong_texture[i], 0);

        RG_GUARANTEE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Ping-pong framebuffer {} is not complete.", i);
    }

    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

void PostProcessController::destroy_framebuffers() {
    CHECKED_GL_CALL(glDeleteFramebuffers, 1, &m_hdr_fbo);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_scene_color_texture);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_bright_color_texture);
    CHECKED_GL_CALL(glDeleteRenderbuffers, 1, &m_depth_renderbuffer);

    for (int i = 0; i < 2; ++i) {
        CHECKED_GL_CALL(glDeleteFramebuffers, 1, &m_pingpong_fbo[i]);
        CHECKED_GL_CALL(glDeleteTextures, 1, &m_pingpong_texture[i]);
    }
}

void PostProcessController::resize(int width, int height) {
    destroy_framebuffers();
    create_framebuffers(width, height);
    m_width = width;
    m_height = height;
}

void PostProcessController::begin_scene_capture() {
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_hdr_fbo);
}

void PostProcessController::end_scene_capture_and_composite() {
    if (m_bloom_enabled) {
        blur_bright_texture();
    }
    composite_to_default_framebuffer();
}

void PostProcessController::blur_bright_texture() {
    auto shader = core::Controller::get<resources::ResourcesController>()->shader("post_process_blur");
    shader->use();

    int write_index = 0;
    uint32_t read_texture = m_bright_color_texture;

    CHECKED_GL_CALL(glBindVertexArray, m_screen_quad_vao);
    for (int i = 0; i < m_blur_passes; ++i) {
        bool horizontal = (i % 2 == 0);

        CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_pingpong_fbo[write_index]);
        shader->set_bool("horizontal", horizontal);

        CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
        CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, read_texture);
        shader->set_int("image", 0);

        CHECKED_GL_CALL(glDrawArrays, GL_TRIANGLES, 0, 6);

        read_texture = m_pingpong_texture[write_index];
        write_index = 1 - write_index;
    }
    CHECKED_GL_CALL(glBindVertexArray, 0);

    m_blurred_bright_texture = read_texture;
}

void PostProcessController::composite_to_default_framebuffer() {
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
    CHECKED_GL_CALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    auto shader = core::Controller::get<resources::ResourcesController>()->shader("post_process_composite");
    shader->use();

    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_scene_color_texture);
    shader->set_int("scene", 0);

    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE1);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_bloom_enabled ? m_blurred_bright_texture : 0);
    shader->set_int("bloomBlur", 1);
    shader->set_bool("bloomEnabled", m_bloom_enabled);
    shader->set_float("exposure", m_exposure);

    CHECKED_GL_CALL(glBindVertexArray, m_screen_quad_vao);
    CHECKED_GL_CALL(glDrawArrays, GL_TRIANGLES, 0, 6);
    CHECKED_GL_CALL(glBindVertexArray, 0);
}

void PostProcessPlatformEventObserver::on_window_resize(int width, int height) {
    m_post_process->resize(width, height);
}
}
