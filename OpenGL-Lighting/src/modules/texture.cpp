#include "Texture.h"

const std::unordered_map<GLenum, GLenum> Texture::formatToType = {
    { GL_RED, GL_UNSIGNED_BYTE },
    { GL_RG, GL_UNSIGNED_BYTE },
    { GL_RGB, GL_UNSIGNED_BYTE },
    { GL_BGR, GL_UNSIGNED_BYTE },
    { GL_RGBA, GL_UNSIGNED_BYTE },
    { GL_BGRA, GL_UNSIGNED_BYTE },
    { GL_DEPTH_COMPONENT, GL_FLOAT },
    { GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 }
};