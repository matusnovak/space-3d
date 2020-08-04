#pragma once

#include "Skybox.hpp"
#include <GLFW/glfw3.h>

namespace Space3d {
class Window {
public:
    Window();
    ~Window();

    void run();

private:
    static void errorCallback(int error, const char* description);
    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

    GLFWwindow* window;
    float angle;

    std::unique_ptr<Skybox> skybox;
    std::optional<Skybox::Result> result;
};
} // namespace Space3d
