// clang-format off
#include <glad/glad.h>
// clang-format on
#include <cstdint>
#include <engine/graphics/GBuffer.hpp>
#include <engine/graphics/OpenGL.hpp>
#include <engine/platform/PlatformController.hpp>
#include <engine/resources/Shader.hpp>
#include <engine/util/Errors.hpp>
#include <memory>

namespace engine::graphics {

void GBuffer::initialize() {
    auto platform = core::Controller::get<platform::PlatformController>();
    m_width = platform->window()->width();
    m_height = platform->window()->height();

    create_framebuffer(m_width, m_height);

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

    auto observer = std::make_unique<GBufferPlatformEventObserver>(this);
    platform->register_platform_event_observer(std::move(observer));
}

void GBuffer::terminate() {
    destroy_framebuffer();
    CHECKED_GL_CALL(glDeleteVertexArrays, 1, &m_screen_quad_vao);
}

void GBuffer::create_framebuffer(int width, int height) {
    CHECKED_GL_CALL(glGenFramebuffers, 1, &m_gbuffer_fbo);
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_gbuffer_fbo);

    CHECKED_GL_CALL(glGenTextures, 1, &m_position_texture);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_position_texture);
    CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_position_texture, 0);

    CHECKED_GL_CALL(glGenTextures, 1, &m_normal_texture);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_normal_texture);
    CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_normal_texture, 0);

    CHECKED_GL_CALL(glGenTextures, 1, &m_albedo_spec_texture);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_albedo_spec_texture);
    // RGBA16F over RGBA8 - specular strength (alpha) can exceed 1.0
    CHECKED_GL_CALL(glTexImage2D, GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    CHECKED_GL_CALL(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, m_albedo_spec_texture, 0);

    uint32_t attachments[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    CHECKED_GL_CALL(glDrawBuffers, 3, attachments);

    CHECKED_GL_CALL(glGenRenderbuffers, 1, &m_depth_renderbuffer);
    CHECKED_GL_CALL(glBindRenderbuffer, GL_RENDERBUFFER, m_depth_renderbuffer);
    CHECKED_GL_CALL(glRenderbufferStorage, GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
    CHECKED_GL_CALL(glFramebufferRenderbuffer, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_renderbuffer);

    RG_GUARANTEE(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "G-buffer framebuffer is not complete.");

    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

void GBuffer::destroy_framebuffer() {
    CHECKED_GL_CALL(glDeleteFramebuffers, 1, &m_gbuffer_fbo);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_position_texture);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_normal_texture);
    CHECKED_GL_CALL(glDeleteTextures, 1, &m_albedo_spec_texture);
    CHECKED_GL_CALL(glDeleteRenderbuffers, 1, &m_depth_renderbuffer);
}

void GBuffer::resize(int width, int height) {
    destroy_framebuffer();
    create_framebuffer(width, height);
    m_width = width;
    m_height = height;
}

void GBuffer::begin_geometry_pass() {
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, m_gbuffer_fbo);
    CHECKED_GL_CALL(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GBuffer::end_geometry_pass() {
    CHECKED_GL_CALL(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

void GBuffer::bind_textures(const resources::Shader *shader) {
    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE0);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_position_texture);
    shader->set_int("gPosition", 0);

    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE1);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_normal_texture);
    shader->set_int("gNormal", 1);

    CHECKED_GL_CALL(glActiveTexture, GL_TEXTURE2);
    CHECKED_GL_CALL(glBindTexture, GL_TEXTURE_2D, m_albedo_spec_texture);
    shader->set_int("gAlbedoSpec", 2);
}

void GBuffer::draw_quad() {
    CHECKED_GL_CALL(glBindVertexArray, m_screen_quad_vao);
    CHECKED_GL_CALL(glDrawArrays, GL_TRIANGLES, 0, 6);
    CHECKED_GL_CALL(glBindVertexArray, 0);
}

void GBufferPlatformEventObserver::on_window_resize(int width, int height) {
    m_gbuffer->resize(width, height);
}
}// namespace engine::graphics
