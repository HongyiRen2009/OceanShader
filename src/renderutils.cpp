#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "renderutils.h"
#include "stb.h"
using namespace std;
using namespace glm;

string readShaderFile(const string& filePath) {
    ifstream fileStream(filePath);
    if (!fileStream.is_open()) {
        cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << filePath << endl;
        return "";
    }
    stringstream sstream;
    sstream << fileStream.rdbuf();
    fileStream.close();
    return sstream.str();
}
unsigned int createShaderProgram(string vertexShaderFileName, string fragmentShaderFileName)
{
    string vertexShaderSource = readShaderFile(vertexShaderFileName);
    string fragmentShaderSource = readShaderFile(fragmentShaderFileName);
    unsigned int vertexShader, fragmentShader, shaderProgram;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexShaderSourceCStr = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vertexShaderSourceCStr, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << endl;
    }
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentShaderSourceCStr = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fragmentShaderSourceCStr, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << endl;
    }
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}
unsigned int createInterleavedVAO(
    const float* vertices,
    size_t vertexBufferSizeBytes,
    const unsigned int* indices,
    size_t indexCount,
    const std::vector<attribute>& attributes,
    unsigned int program
) {
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBufferSizeBytes, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    for (size_t i = 0; i < attributes.size(); i++) {
        GLint loc = glGetAttribLocation(program, attributes[i].name.c_str());
        if (loc < 0) {
            cout << "ERROR: Attribute location not found for " << attributes[i].name << endl;
            continue;
        }
        glEnableVertexAttribArray(loc);

        if (attributes[i].type == GL_UNSIGNED_INT) {
            glVertexAttribIPointer(
                loc,
                attributes[i].size,
                attributes[i].type,
                attributes[i].stride,
                (void*)(uintptr_t)attributes[i].offset
            );
        }
        else {
            glVertexAttribPointer(
                loc,
                attributes[i].size,
                attributes[i].type,
                GL_FALSE,
                attributes[i].stride,
                (void*)(uintptr_t)attributes[i].offset
            );
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    return VAO;
}
mat4 createModelMatrix(vec3 position, vec3 rotation, vec3 scale) {
    mat4 model = mat4(1.0f);
    model = translate(model, position);
    model = rotate(model, radians(rotation.x), vec3(1.0f, 0.0f, 0.0f));
    model = rotate(model, radians(rotation.y), vec3(0.0f, 1.0f, 0.0f));
    model = rotate(model, radians(rotation.z), vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    return model;
}
unsigned int importTexture2D(const char* imagePath, int width, int height, int nrChannels) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Set texture wrapping/filtering options for a pixel-art style atlas
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Load image data (do not force number of channels)
    unsigned char* data = stbi_load(imagePath, &width, &height, &nrChannels, 0);
    if (data)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 2)
            format = GL_RG;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    }
    else
    {
        std::cout << "Failed to load texture: " << imagePath << std::endl;
    }
    stbi_image_free(data);
    return texture;
}
void bindTexture2D(unsigned int texture, const char* key, unsigned int slot, unsigned int program,GLenum textureSlot = GL_TEXTURE0) {
    glActiveTexture(textureSlot + slot);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(program, key), slot);
}
void bindTexture(unsigned int texture, const char* key, unsigned int slot, GLenum textureSlot) {
    GLenum target = textureSlot == GL_TEXTURE0 ? GL_TEXTURE_2D : textureSlot;
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(target, texture);
    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    if (program != 0) {
        glUniform1i(glGetUniformLocation(program, key), slot);
    }
}
unsigned int createComputeShaderProgram(string computeShaderFileName) {
    string computeShaderSource = readShaderFile(computeShaderFileName);
    unsigned int computeShader, shaderProgram;
    computeShader = glCreateShader(GL_COMPUTE_SHADER);
    const char* computeShaderSourceCStr = computeShaderSource.c_str();
    glShaderSource(computeShader, 1, &computeShaderSourceCStr, NULL);
    glCompileShader(computeShader);
    int success;
    char infoLog[512];
    glGetShaderiv(computeShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(computeShader, 512, NULL, infoLog);
        cout << "ERROR::SHADER::COMPUTE::COMPILATION_FAILED\n" << infoLog << endl;
    }
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, computeShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }
    glDeleteShader(computeShader);
    return shaderProgram;
}
unsigned int createDepthTexture(int width, int height) {
    unsigned int depthTexture;
    glGenTextures(1, &depthTexture);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return depthTexture;
}
unsigned int createTexture2D(int width, int height, GLenum internalFormat, GLenum format, GLenum type,GLint minFilter, GLint magFilter, GLint wrapS, GLint wrapT, const void* data) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    return texture;
}
unsigned int createTexture1D(int width, GLenum internalFormat, GLenum format, GLenum type, const void* data) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_1D, texture);
    glTexImage1D(GL_TEXTURE_1D, 0, internalFormat, width, 0, format, type, data);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    return texture;
}
void bindTexture2D(unsigned int texture, const char* key, unsigned int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUniform1i(glGetUniformLocation(slot, key), slot);
}