#include "Vbo.hpp"

Space3d::Vbo::Vbo() : ref(0) {
    glGenBuffers(1, &ref);
}

Space3d::Vbo::~Vbo() {
    if (ref) {
        glDeleteBuffers(1, &ref);
    }
}

void Space3d::Vbo::bufferData(const uint8_t* data, const size_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, ref);
    glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
}

void Space3d::Vbo::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, ref);
}

Space3d::Vbo::Vbo(Vbo&& other) noexcept : ref(0) {
    swap(other);
}

void Space3d::Vbo::swap(Vbo& other) noexcept {
    std::swap(ref, other.ref);
}

Space3d::Vbo& Space3d::Vbo::operator=(Vbo&& other) noexcept {
    if (this != &other) {
        swap(other);
    }
    return *this;
}
