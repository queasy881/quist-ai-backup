#include "core/Time.h"
#include <GLFW/glfw3.h>

namespace Time {

static float s_delta      = 0.0f;
static float s_last       = 0.0f;
static float s_total      = 0.0f;
static int   s_fps        = 0;
static int   s_frames     = 0;
static float s_fpsAccum   = 0.0f;

void init() {
    s_last = static_cast<float>(glfwGetTime());
}

void update() {
    float now = static_cast<float>(glfwGetTime());
    s_delta   = now - s_last;
    s_last    = now;
    s_total   = now;

    s_frames++;
    s_fpsAccum += s_delta;
    if (s_fpsAccum >= 1.0f) {
        s_fps      = s_frames;
        s_frames   = 0;
        s_fpsAccum -= 1.0f;
    }
}

float deltaTime()  { return s_delta; }
float totalTime()  { return s_total; }
int   fps()        { return s_fps;   }

} // namespace Time
