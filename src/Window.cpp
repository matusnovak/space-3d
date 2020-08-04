// clang-format off
#include <glad/glad.h> // Needs to be first
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "Window.hpp"
#include "Skybox.hpp"
#include <exception>
#include <iostream>
#include <random>
// clang-format on

static const std::string SKYBOX_SHADER_FRAG = R"(#version 330 core
in vec3 v_texCoords;

out vec4 fragmentColor;

uniform samplerCube skyboxTexture;

void main() {
    vec3 emissive = texture(skyboxTexture, v_texCoords).rgb;
    fragmentColor = vec4(emissive, 1.0);
}
)";

static const std::string SKYBOX_SHADER_VERT = R"(#version 330 core
layout(location = 0) in vec3 position;

out vec3 v_texCoords;

uniform mat4 modelMatrix;
uniform mat4 transformationProjectionMatrix;

void main() {
    v_texCoords = position;
    vec4 worldPos = modelMatrix * vec4(position, 1.0);
    gl_Position = transformationProjectionMatrix * worldPos;
}
)";

// Simple skybox box with two triangles per side.
static const float SKYBOX_VERTICES[] = {
    // positions
    -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
    -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

    1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

    -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

    -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
    1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

    -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
    1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Space3d::Window::Window() : window(nullptr), angle(0.0f) {
}

Space3d::Window::~Window() {
    if (window) {
        glfwDestroyWindow(window);
        glfwTerminate();
    }
}

void Space3d::Window::run() {
    glfwSetErrorCallback(errorCallback);

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize glfw");
    }

    // Basic GLFW window stuff
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

    window = glfwCreateWindow(1280, 720, "Space 3D", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        throw std::runtime_error("Failed to create glfw window");
    }

    glfwSetWindowUserPointer(window, this);
    glfwSetKeyCallback(window, keyCallback);

    glfwMakeContextCurrent(window);
    gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress));
    glfwSwapInterval(1);

    // You need to enable these
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_CUBE_MAP);
    glEnable(GL_BLEND);

    // This shader will be used to render the skybox texture (not to generate it!)
    Shader skyboxShader(SKYBOX_SHADER_VERT, SKYBOX_SHADER_FRAG, std::nullopt);
    skyboxShader.use();
    skyboxShader.setInt("skyboxTexture", 0);
    skyboxShader.setMat4("modelMatrix", glm::scale(glm::mat4x4(1.0f), glm::vec3{100.0f}));
    const auto projection = glm::perspective(static_cast<float>(M_PI) / 2.0f, 1.0f, 0.1f, 1000.0f);

    // This is just a simple box skybox
    Vao vaoSkybox;
    Vbo vboSkybox;

    vaoSkybox.bind();
    vboSkybox.bind();
    vboSkybox.bufferData(reinterpret_cast<const uint8_t*>(SKYBOX_VERTICES), sizeof(SKYBOX_VERTICES));

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    // Create the skybox generator instance and create a new skybox
    // with seed 12345LL and texture size 1024x1024 (6 sides).
    skybox = std::make_unique<Skybox>();
    result = skybox->generate(12345LL, 1024);

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        // Default blending
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);

        const auto transformation = glm::rotate(glm::mat4x4(1.0f), angle, glm::vec3{0.0f, 1.0f, 0.0f});
        angle += 0.001f;

        // Render the skybox on the screen
        skyboxShader.use();
        vaoSkybox.bind();
        result.value().bind();
        skyboxShader.setMat4("transformationProjectionMatrix", projection * transformation);
        skyboxShader.drawArrays(GL_TRIANGLES, 6 * 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Space3d::Window::errorCallback(const int error, const char* description) {
    std::cerr << "error: " << error << " description: " << description << std::endl;
}

void Space3d::Window::keyCallback(GLFWwindow* window, const int key, const int scancode, const int action,
                                  const int mods) {
    (void)scancode;
    (void)mods;

    auto& self = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        // Generate a new skybox with a random device seed.
        // Will create 1024x1024 cubemap texture.
        const auto seed = std::random_device{}();
        std::cout << "new seed: " << seed << std::endl;
        self.result = self.skybox->generate(seed, 1024);
    }
}

int main(const int argc, char** argv) {
    using namespace Space3d;
    try {
        Window window;
        window.run();
        return EXIT_SUCCESS;
    } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
