#include "rendering/FullscreenQuad.h"

GLuint FullscreenQuad::s_vao    = 0;
GLuint FullscreenQuad::s_vbo    = 0;
bool   FullscreenQuad::s_inited = false;

void FullscreenQuad::init() {
    if (s_inited) return;
    // Oversized triangle covers entire screen
    float verts[] = {
        -1.0f, -1.0f,  0.0f, 0.0f,
         3.0f, -1.0f,  2.0f, 0.0f,
        -1.0f,  3.0f,  0.0f, 2.0f,
    };
    glCreateVertexArrays(1, &s_vao);
    glCreateBuffers(1, &s_vbo);
    glNamedBufferStorage(s_vbo, sizeof(verts), verts, 0);

    glVertexArrayVertexBuffer(s_vao, 0, s_vbo, 0, 4 * sizeof(float));
    glEnableVertexArrayAttrib(s_vao, 0);
    glVertexArrayAttribFormat(s_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(s_vao, 0, 0);

    glEnableVertexArrayAttrib(s_vao, 1);
    glVertexArrayAttribFormat(s_vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
    glVertexArrayAttribBinding(s_vao, 1, 0);

    s_inited = true;
}

void FullscreenQuad::draw() {
    glBindVertexArray(s_vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void FullscreenQuad::shutdown() {
    if (s_vao) glDeleteVertexArrays(1, &s_vao);
    if (s_vbo) glDeleteBuffers(1, &s_vbo);
    s_vao = 0; s_vbo = 0; s_inited = false;
}
