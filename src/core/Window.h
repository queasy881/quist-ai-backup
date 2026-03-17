#pragma once
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <string>

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();

    GLFWwindow* getHandle() const { return m_window; }
    int  getWidth()  const { return m_width;  }
    int  getHeight() const { return m_height; }
    float getAspect() const { return static_cast<float>(m_width) / static_cast<float>(m_height); }

private:
    GLFWwindow* m_window = nullptr;
    int m_width, m_height;

    static void framebufferCallback(GLFWwindow* win, int w, int h);
};
