#pragma once
#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
using namespace glm;
// Attribute struct for VAO setup
struct attribute {
    std::string name;
    unsigned int offset;
    unsigned int size;
    unsigned int stride;
    GLenum type;
};

// Shader utilities
std::string readShaderFile(const std::string& filePath);
unsigned int createShaderProgram(std::string vertexShaderFileName, std::string fragmentShaderFileName, std::string tessellationControlFileName = "", std::string tessellationEvaluationFileName = "");

// VAO utility
unsigned int createInterleavedVAO(
    const float* vertices,
    size_t vertexBufferSizeBytes,
    const unsigned int* indices,
    size_t indexCount,
    const std::vector<attribute>& attributes,
    unsigned int program
);
unsigned int createTexture1D(int width, GLenum internalFormat, GLenum format, GLenum type, const void* data = nullptr);
unsigned int createTexture2D(int width, int height, GLenum internalFormat, GLenum format, GLenum type,GLint minFilter = GL_NEAREST, GLint magFilter = GL_NEAREST, GLint wrapS = GL_CLAMP_TO_EDGE, GLint wrapT = GL_CLAMP_TO_EDGE, const void* data = nullptr);
unsigned int createTextureArray2D(int width, int height, int layers, GLenum internalFormat, GLenum format, GLenum type, GLint minFilter = GL_NEAREST, GLint magFilter = GL_NEAREST, GLint wrapS = GL_CLAMP_TO_EDGE, GLint wrapT = GL_CLAMP_TO_EDGE, const void* data = nullptr);
unsigned int createDepthTexture(int width, int height);
unsigned int importTexture2D(const char* imagePath, int width, int height, int nrChannels);
void bindTexture(unsigned int texture, const char* key, unsigned int slot, GLenum textureSlot = GL_TEXTURE0);
void bindTextureArray(unsigned int textureArray, const char* key, unsigned int slot, GLenum textureSlot = GL_TEXTURE0);
mat4 createModelMatrix(vec3 position, vec3 rotation, vec3 scale);
unsigned int createComputeShaderProgram(std::string computeShaderFileName);