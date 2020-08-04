#pragma once
#include "Shader.hpp"
#include "Vao.hpp"
#include "Vbo.hpp"
#include <glad/glad.h>
#include <memory>

namespace Space3d {
class Skybox {
public:
    class Result {
    public:
        Result();
        Result(const Result& other) = delete;
        Result(Result&& other) noexcept;
        ~Result();

        void swap(Result& other) noexcept;
        Result& operator=(const Result& other) = delete;
        Result& operator=(Result&& other) noexcept;

        void setStorage(int width, GLenum internalFormat, GLenum format, GLenum type);
        void generateMipmaps();
        void bind() const;
        void release();
        GLuint get() const {
            return ref;
        }

    private:
        GLuint ref;
    };

    Skybox();

    Result generate(int64_t seed, int width) const;

private:
    struct Mesh {
        Vao vao;
        Vbo vbo;
    };

    Shader shaderStars;
    Shader shaderNebula;
    Mesh meshSkybox;
};
} // namespace Space3d
