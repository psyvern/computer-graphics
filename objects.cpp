#pragma once

#include "stb/stb_image.h"
#include "types.h"
#include <GL/glew.h>
#include <cstdio>
#include <format>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <string>
#include <vector>

#define DELETE_FUNCS(name, function, friendly)                                 \
  private:                                                                     \
    name(u32 id) { this->id = id; }                                            \
                                                                               \
  public:                                                                      \
    u32 id;                                                                    \
                                                                               \
    static name empty() { return name(0); }                                    \
                                                                               \
    ~name() {                                                                  \
        if (id != 0) {                                                         \
            function(id);                                                      \
            /* printf("Deleted %s: %d\n", friendly, id); */                    \
        }                                                                      \
    }                                                                          \
                                                                               \
    name& operator=(name&& other) {                                            \
        if (this != &other) {                                                  \
            this->id = other.id;                                               \
            other.id = 0;                                                      \
        }                                                                      \
                                                                               \
        return *this;                                                          \
    }

#define DELETE_FUNCS_2(name, function, friendly)                               \
  private:                                                                     \
    name(u32 id) { this->id = id; }                                            \
                                                                               \
  public:                                                                      \
    u32 id;                                                                    \
                                                                               \
    static name empty() { return name(0); }                                    \
                                                                               \
    ~name() {                                                                  \
        if (id != 0) {                                                         \
            function(1, &id);                                                  \
            /* printf("Deleted %s: %d\n", friendly, id); */                    \
        }                                                                      \
    }                                                                          \
                                                                               \
    name& operator=(name&& other) {                                            \
        if (this != &other) {                                                  \
            this->id = other.id;                                               \
            other.id = 0;                                                      \
        }                                                                      \
                                                                               \
        return *this;                                                          \
    }

class VAO {
  public:
    static VAO gen() {
        u32 id;
        glGenVertexArrays(1, &id);

        glBindVertexArray(id);
        return VAO(id);
    }

    void bind() { glBindVertexArray(id); }

    void enableAttributeArray(u32 index) {
        glEnableVertexArrayAttrib(id, index);
    }

    DELETE_FUNCS_2(VAO, glDeleteVertexArrays, "VAO")
};

class VBData {
  public:
    u32 type;
    u32 amount;
    bool normalized;

    u64 size() {
        return amount * (type == GL_FLOAT ? sizeof(f32) : sizeof(u32));
    }
};

class VBO {
  public:
    static VBO create(std::vector<VBData> attributes) {
        u32 id;
        glGenBuffers(1, &id);
        glBindBuffer(GL_ARRAY_BUFFER, id);

        u64 stride = 0;
        for (auto attribute : attributes) {
            stride += attribute.size();
        }

        u32 index = 0;
        u64 current_stride = 0;
        for (auto attribute : attributes) {
            glVertexAttribPointer(
                index,
                attribute.amount,
                attribute.type,
                attribute.normalized,
                stride,
                (void*) current_stride
            );

            index += 1;
            current_stride += attribute.size();
        }

        return VBO(id, stride);
    }

    void bind(u32 type) { glBindBuffer(type, id); }

    void data(u32 size, const void* data, u32 usage) {
        if (size % stride != 0) {
            std::cout << std::format("{} vs {}\n", this->stride, size);
        }
        glNamedBufferData(id, size, data, usage);
    }

    void data(std::vector<float>* data, u32 usage) {
        this->data(sizeof(float) * data->size(), data->data(), usage);
    }

  private:
    VBO(u32 id, u64 stride) {
        this->id = id;
        this->stride = stride;
    }

  public:
    u32 id;
    u64 stride;

    static VBO empty() { return VBO(0, 0); }

    ~VBO() {
        if (id != 0) {
            glDeleteBuffers(1, &id);
            /* printf("Deleted %s: %d\n", "buffer", id); */
        }
    }

    VBO& operator=(VBO&& other) {
        if (this != &other) {
            this->id = other.id;
            this->stride = other.stride;
            other.id = 0;
            other.stride = 0;
        }

        return *this;
    }
};

class Shader {
  public:
    static Shader create(i32 type, const char* string) {
        u32 id = glCreateShader(type);

        glShaderSource(id, 1, &string, NULL);
        glCompileShader(id);

        i32 result;
        glGetShaderiv(id, GL_COMPILE_STATUS, &result);

        if (!result) {
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &result);
            char* log = (char*) malloc(result);
            glGetShaderInfoLog(id, result, NULL, log);
            printf("\033[1;91mError in shader\033[0m: %s", log);

            return 0;
        }

        return Shader(id);
    }

    static Shader vertex(const char* string) {
        return create(GL_VERTEX_SHADER, string);
    }

    static Shader fragment(const char* string) {
        return create(GL_FRAGMENT_SHADER, string);
    }

    DELETE_FUNCS(Shader, glDeleteShader, "shader")
};

class ShaderProgram {
  public:
    static ShaderProgram create(std::vector<Shader*> shaders) {
        auto id = glCreateProgram();
        for (auto shader : shaders) {
            glAttachShader(id, shader->id);
        }

        glLinkProgram(id);
        return ShaderProgram(id);
    }

    void use() { glUseProgram(id); }

    void uniform(const char* name, glm::vec3 data, bool transpose = false) {
        u32 location = glGetUniformLocation(id, name);
        glProgramUniform3f(id, location, data.x, data.y, data.z);
    }

    void uniform(const char* name, glm::vec4 data, bool transpose = false) {
        u32 location = glGetUniformLocation(id, name);
        glProgramUniform4f(id, location, data.x, data.y, data.z, data.w);
    }

    void uniform(const char* name, glm::mat3 data, bool transpose = false) {
        u32 location = glGetUniformLocation(id, name);
        glProgramUniformMatrix3fv(
            id, location, 1, transpose, glm::value_ptr(data)
        );
    }

    void uniform(const char* name, glm::mat4 data, bool transpose = false) {
        u32 location = glGetUniformLocation(id, name);
        glProgramUniformMatrix4fv(
            id, location, 1, transpose, glm::value_ptr(data)
        );
    }

    DELETE_FUNCS(ShaderProgram, glDeleteProgram, "program")
};

class Texture2D {
  public:
    static Texture2D create(
        u32 width,
        u32 height,
        u8* data,
        bool nearest = false,
        bool rgb = true
    ) {
        u32 id;
        glGenTextures(1, &id);

        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            rgb ? GL_RGBA : GL_RED,
            width,
            height,
            0,
            rgb ? GL_RGBA : GL_RED,
            GL_UNSIGNED_BYTE,
            data
        );

        glGenerateTextureMipmap(id);

        glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTextureParameteri(
            id, GL_TEXTURE_MAG_FILTER, nearest ? GL_NEAREST : GL_LINEAR
        );

        return Texture2D(id);
    }

    static Texture2D read(std::string path, bool nearest = true) {
        i32 width, height, channels;
        u8* data = stbi_load(path.c_str(), &width, &height, &channels, 4);

        return create(width, height, data, nearest);
    }

    void bind() { glBindTexture(GL_TEXTURE_2D, id); }

    static void unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

    DELETE_FUNCS_2(Texture2D, glDeleteTextures, "texture")
};
