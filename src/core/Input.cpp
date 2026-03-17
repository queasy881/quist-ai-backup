#include "core/Input.h"
#include <cstring>

static GLFWwindow* s_window = nullptr;

static bool s_keys[512]         = {};
static bool s_keysPrev[512]     = {};
static bool s_buttons[8]        = {};
static bool s_buttonsPrev[8]    = {};

static glm::vec2 s_mousePos   = {0, 0};
static glm::vec2 s_mouseDelta = {0, 0};
static glm::vec2 s_mouseLast  = {0, 0};
static float     s_scrollDelta = 0.0f;
static bool      s_firstMouse  = true;
static bool      s_cursorLocked = false;

void Input::init(GLFWwindow* window) {
    s_window = window;
    glfwSetKeyCallback(window, keyCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorCallback);
    glfwSetScrollCallback(window, scrollCallback);

    std::memset(s_keys, 0, sizeof(s_keys));
    std::memset(s_keysPrev, 0, sizeof(s_keysPrev));
    std::memset(s_buttons, 0, sizeof(s_buttons));
    std::memset(s_buttonsPrev, 0, sizeof(s_buttonsPrev));
}

void Input::update() {
    std::memcpy(s_keysPrev, s_keys, sizeof(s_keys));
    std::memcpy(s_buttonsPrev, s_buttons, sizeof(s_buttons));
    s_scrollDelta = 0.0f;
    s_mouseDelta  = {0, 0};
}

bool Input::isKeyDown(int key)          { return key >= 0 && key < 512 && s_keys[key]; }
bool Input::isKeyPressed(int key)       { return key >= 0 && key < 512 && s_keys[key] && !s_keysPrev[key]; }
bool Input::isMouseButtonDown(int btn)  { return btn >= 0 && btn < 8 && s_buttons[btn]; }
bool Input::isMouseButtonPressed(int btn){ return btn >= 0 && btn < 8 && s_buttons[btn] && !s_buttonsPrev[btn]; }
glm::vec2 Input::getMousePos()          { return s_mousePos; }
glm::vec2 Input::getMouseDelta()        { return s_mouseDelta; }
float     Input::getScrollDelta()       { return s_scrollDelta; }
bool      Input::isCursorLocked()       { return s_cursorLocked; }

void Input::setCursorLocked(bool locked) {
    s_cursorLocked = locked;
    glfwSetInputMode(s_window, GLFW_CURSOR,
        locked ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (locked) s_firstMouse = true;
}

void Input::keyCallback(GLFWwindow*, int key, int /*scancode*/, int action, int /*mods*/) {
    if (key >= 0 && key < 512) {
        if (action == GLFW_PRESS)   s_keys[key] = true;
        if (action == GLFW_RELEASE) s_keys[key] = false;
    }
}

void Input::mouseButtonCallback(GLFWwindow*, int button, int action, int /*mods*/) {
    if (button >= 0 && button < 8) {
        if (action == GLFW_PRESS)   s_buttons[button] = true;
        if (action == GLFW_RELEASE) s_buttons[button] = false;
    }
}

void Input::cursorCallback(GLFWwindow*, double x, double y) {
    glm::vec2 pos(static_cast<float>(x), static_cast<float>(y));
    if (s_firstMouse) {
        s_mouseLast  = pos;
        s_firstMouse = false;
    }
    s_mouseDelta = pos - s_mouseLast;
    s_mouseLast  = pos;
    s_mousePos   = pos;
}

void Input::scrollCallback(GLFWwindow*, double /*xoff*/, double yoff) {
    s_scrollDelta = static_cast<float>(yoff);
}
