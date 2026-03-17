#pragma once
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Input {
public:
    static void init(GLFWwindow* window);
    static void update();

    static bool isKeyDown(int key);
    static bool isKeyPressed(int key);

    static bool isMouseButtonDown(int button);
    static bool isMouseButtonPressed(int button);

    static glm::vec2 getMousePos();
    static glm::vec2 getMouseDelta();
    static float     getScrollDelta();

    static void setCursorLocked(bool locked);
    static bool isCursorLocked();

private:
    static void keyCallback(GLFWwindow*, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow*, int button, int action, int mods);
    static void cursorCallback(GLFWwindow*, double x, double y);
    static void scrollCallback(GLFWwindow*, double xoff, double yoff);
};
