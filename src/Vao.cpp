#include "Vao.hpp"

Space3d::Vao::Vao() : ref(0) {
    glGenVertexArrays(1, &ref);
}

Space3d::Vao::~Vao() {
    if (ref) {
        glDeleteVertexArrays(1, &ref);
    }
}

void Space3d::Vao::bind() const {
    glBindVertexArray(ref);
}

Space3d::Vao::Vao(Vao&& other) noexcept : ref(0) {
    swap(other);
}

void Space3d::Vao::swap(Vao& other) noexcept {
    std::swap(ref, other.ref);
}

Space3d::Vao& Space3d::Vao::operator=(Vao&& other) noexcept {
    if (this != &other) {
        swap(other);
    }
    return *this;
}
