#include "Texture.h"

const std::unordered_map<GLenum, GLenum> Texture::unsizedFormatToType = {
    { GL_RED, GL_UNSIGNED_BYTE },
    { GL_RG, GL_UNSIGNED_BYTE },
    { GL_RGB, GL_UNSIGNED_BYTE },
    { GL_BGR, GL_UNSIGNED_BYTE },
    { GL_RGBA, GL_UNSIGNED_BYTE },
    { GL_BGRA, GL_UNSIGNED_BYTE },
    { GL_DEPTH_COMPONENT, GL_FLOAT },
    { GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 }
};

const std::unordered_map<GLenum, GLenum> Texture::sizedFormatToType = {
    { GL_R8,             GL_UNSIGNED_BYTE },
    { GL_RG8,            GL_UNSIGNED_BYTE },
    { GL_RGB8,           GL_UNSIGNED_BYTE },
    { GL_RGBA8,          GL_UNSIGNED_BYTE },
    { GL_R16F,           GL_FLOAT },
    { GL_RG16F,          GL_FLOAT },
    { GL_RGB16F,         GL_FLOAT },
    { GL_RGBA16F,        GL_FLOAT },
    { GL_R32F,           GL_FLOAT },
    { GL_RG32F,          GL_FLOAT },
    { GL_RGB32F,         GL_FLOAT },
    { GL_RGBA32F,        GL_FLOAT },
    { GL_DEPTH_COMPONENT16, GL_UNSIGNED_SHORT },
    { GL_DEPTH_COMPONENT24, GL_UNSIGNED_INT },
    { GL_DEPTH_COMPONENT32F, GL_FLOAT },
    { GL_DEPTH24_STENCIL8,   GL_UNSIGNED_INT_24_8 }
};
