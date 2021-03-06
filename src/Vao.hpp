#pragma once

#include <glad/glad.h>
#include <string>

namespace Space3d {
class Vao {
public:
    Vao();
    Vao(const Vao& other) = delete;
    Vao(Vao&& other) noexcept;
    ~Vao();

    void swap(Vao& other) noexcept;
    Vao& operator=(const Vao& other) = delete;
    Vao& operator=(Vao&& other) noexcept;

    void bind() const;

    GLuint get() const {
        return ref;
    }

private:
    GLuint ref;
};
} // namespace Space3d
