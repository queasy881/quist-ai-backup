#pragma once
#include <glad/gl.h>

// Shared full-screen triangle for all screen-space passes.
// Uses the oversized-triangle trick (3 verts, no VBO needed in GL 4.6,
// but we keep a VAO for compatibility).
class FullscreenQuad {
public:
    static void init();
    static void draw();  // binds VAO, draws 3 verts
    static void shutdown();
private:
    static GLuint s_vao;
    static GLuint s_vbo;
    static bool   s_inited;
};
