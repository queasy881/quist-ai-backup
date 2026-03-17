#include "core/Window.h"
#include <cstdio>
#include <cstdlib>

Window::Window(int width, int height, const std::string& title)
    : m_width(width), m_height(height)
{
    if (!glfwInit()) {
        std::fprintf(stderr, "Failed to initialise GLFW\n");
        std::exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        std::exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // vsync

    int version = gladLoadGL(glfwGetProcAddress);
    if (version == 0) {
        std::fprintf(stderr, "Failed to initialise OpenGL loader\n");
        std::exit(EXIT_FAILURE);
    }
    std::printf("OpenGL %d.%d loaded\n", GLAD_VERSION_MAJOR(version), GLAD_VERSION_MINOR(version));

    glfwSetWindowUserPointer(m_window, this);
    glfwSetFramebufferSizeCallback(m_window, framebufferCallback);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

Window::~Window() {
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool Window::shouldClose() const { return glfwWindowShouldClose(m_window); }
void Window::swapBuffers()       { glfwSwapBuffers(m_window); }
void Window::pollEvents()        { glfwPollEvents(); }

void Window::framebufferCallback(GLFWwindow* win, int w, int h) {
    glViewport(0, 0, w, h);
    // Propagate new size back to Window instance
    Window* self = static_cast<Window*>(glfwGetWindowUserPointer(win));
    if (self) { self->m_width = w; self->m_height = h; }
}
