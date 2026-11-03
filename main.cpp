#include <algorithm>
#include <numbers>
#define GLM_ENABLE_EXPERIMENTAL
#define STB_IMAGE_IMPLEMENTATION

#include "assets.h"
#include "font.cpp"
#include "objects.cpp"
#include "types.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <format>
#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <math.h>
#include <ostream>
#include <time.h>
#include <vector>

using std::numbers::pi;

class Particle {
  public:
    glm::vec3 position;
    glm::vec3 scale;
    u32 lifetime;
};

class Mesh {
  public:
    std::vector<float> data;
    std::vector<u32> indices;

    static Mesh read(std::string path) {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate | aiProcess_FlipUVs |
                aiProcess_JoinIdenticalVertices
        );

        std::vector<float> data;
        std::vector<u32> indices;
        for (int i = 0; i < scene->mNumMeshes; i++) {
            auto mesh = scene->mMeshes[i];

            for (int j = 0; j < mesh->mNumVertices; j++) {
                auto position = mesh->mVertices[j];
                auto normal = mesh->mNormals[j];
                auto texture = mesh->mTextureCoords[0][j];
                data.push_back(position.x);
                data.push_back(position.y);
                data.push_back(position.z);
                data.push_back(1.0f);
                data.push_back(1.0f);
                data.push_back(1.0f);
                data.push_back(texture.x);
                data.push_back(texture.y);
                data.push_back(normal.x);
                data.push_back(normal.y);
                data.push_back(normal.z);
            }

            for (int j = 0; j < mesh->mNumFaces; j++) {
                auto face = mesh->mFaces[j];
                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }
        }

        return Mesh{
            data,
            indices,
        };
    }

    void draw(VBO* vbo) {
        vbo->data(&data, GL_STATIC_DRAW);
        glDrawElements(
            GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data()
        );
    }
};

void framebuffer_size_callback(GLFWwindow* window, int width, int height);

u32 window_width = 800;
u32 window_height = 600;

class Camera {
  public:
    float speed = 0.05f;
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    f32 pitch = 0.0f;
    float yaw = 0.0f;

    glm::vec3 position = glm::vec3(0);
    glm::vec3 target = glm::vec3(0);
    glm::vec3 front = glm::vec3(1);

    glm::mat4 viewMatrix() {
        return glm::lookAt(position, position + front, up);
    }

    void move(
        bool forward,
        bool backward,
        bool left,
        bool right,
        bool up,
        bool down
    ) {
        auto displacement = glm::vec3(0);
        if (right)
            displacement += glm::normalize(glm::cross(front, this->up));
        if (left)
            displacement -= glm::normalize(glm::cross(front, this->up));
        if (forward)
            displacement += glm::normalize(front * glm::vec3(1, 0, 1));
        if (backward)
            displacement -= glm::normalize(front * glm::vec3(1, 0, 1));

        if (displacement != glm::vec3(0)) {
            displacement = glm::normalize(displacement);
        }

        if (up)
            displacement.y += 1;
        if (down)
            displacement.y -= 1;

        position += displacement * speed;
    }
};

class Transform {
  public:
    glm::vec3 position;
    glm::vec3 scale = glm::vec3(1);
    glm::quat rotation;

    glm::mat4 matrix() {
        auto t = glm::translate(glm::mat4(1), position);
        auto r = glm::toMat4(rotation);
        auto s = glm::scale(glm::mat4(1), scale);

        return t * r * s;
    }
};

glm::quat offsetOrientation(
    glm::quat orientation,
    glm::vec3 axis,
    float angle,
    bool relative = true
) {
    axis = glm::normalize(axis) * sin(angle / 2);

    auto offset = glm::quat(cos(angle / 2), axis);

    if (relative) {
        orientation = orientation * offset;
    } else {
        orientation = offset * orientation;
    }
    // case CAMERA_RELATIVE:
    //     Matrix4 camMat = calcLookAtMatrix(
    //         resolveCamPosition(), camTarget, new Vector3(0, 1, 0)
    //     );
    //     auto viewQuat = camMat.toQuaternion();
    //     orientation = new Quaternion(viewQuat)
    //                       .conjugate()
    //                       .mult(offset)
    //                       .mult(viewQuat)
    //                       .mult(orientation);
    //     break;
    // }

    return glm::normalize(orientation);
}

class Car {
  public:
    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::vec3 scale = glm::vec3(0.1);

    glm::quat orientation = glm::quat();

    glm::vec3 speed = glm::vec3(0);
    glm::vec2 rotation_speed = glm::vec2(0);

    bool space_last_pressed = false;
    i32 jump_state = 1;
    float wheel_direction = 0;

    glm::vec3 front() {
        // return glm::vec3(-sin(yaw), 0, -cos(yaw));
        auto front = orientation * glm::vec3(0, 0, -1);
        front.y = 0;
        return glm::normalize(front);
    }

    float yaw() {
        auto front = this->front();
        auto forward = glm::vec3(0, 0, -1);
        auto up = glm::vec3(0, 1, 0);

        return atan2(
            glm::dot(glm::cross(forward, front), up), glm::dot(forward, front)
        );
    }

    void move(float vertical, float horizontal, bool space, bool fast) {
        auto max_speed = (float) (fast * 3 + 1);
        auto on_floor = position.y <= 0.0f;

        if (on_floor) {
            wheel_direction = horizontal;
        } else {
            wheel_direction = 0;
        }

        float yaw = 0;
        float pitch = 0;
        float roll = 0;

        const float FLIP_TIME = 30.0f;

        if (rotation_speed == glm::vec2(0)) {
            if (on_floor)
                yaw -= glm::radians(horizontal * 3.0f * vertical);
            else {
                if (fast) {
                    roll -= 3 * glm::radians(horizontal);
                } else {
                    yaw -= 3 * glm::radians(horizontal);
                }
                pitch -= 3 * glm::radians(vertical);
            }
        }

        auto front = this->front();

        if (on_floor) {
            speed += max_speed * vertical * 0.1f * front;
            auto speed_horizontal = glm::vec2(speed.x, speed.z);
            if (glm::length(speed_horizontal) > max_speed) {
                speed_horizontal = glm::normalize(speed_horizontal) * max_speed;
                speed.x = speed_horizontal.x;
                speed.z = speed_horizontal.y;
            }
        }

        if (space) {
            if (on_floor && !space_last_pressed) {
                speed.y = 0.25f;
            } else {
                auto right = glm::cross(front, { 0, 1, 0 });

                if (jump_state == 1 && !space_last_pressed) {
                    rotation_speed = glm::vec2(-horizontal, -vertical);
                    if (rotation_speed != glm::vec2(0)) {
                        jump_state = -FLIP_TIME;
                        rotation_speed = glm::normalize(rotation_speed);
                        speed = front * (vertical * 0.3f) +
                                right * (horizontal * 0.3f);
                    } else {
                        auto up = orientation * glm::vec3(0, 1, 0);
                        speed = up * 0.25f;
                    }
                }
            }
        }

        space_last_pressed = space;

        if (jump_state < 0) {
            roll += 2.0f * pi / FLIP_TIME * rotation_speed.x;
            pitch += 2.0f * pi / FLIP_TIME * rotation_speed.y;
            jump_state += 1;

            if (jump_state == 0)
                rotation_speed = glm::vec2(0);
        } else {
            speed.y -= 0.01f;
        }

        // roll += rotation_speed.x;
        // pitch += rotation_speed.y;
        position += speed;
        // position.y = 1;

        front = this->front();

        if (position.y <= 0.0f) {
            position.y = 0;

            roll = 0;
            pitch = 0;
            orientation = glm::angleAxis(this->yaw(), glm::vec3(0, 1, 0));

            speed = glm::vec3(0);
            rotation_speed = glm::vec2(0);
            jump_state = 1;
        }

        orientation =
            offsetOrientation(orientation, glm::vec3(0, 1, 0), yaw, true);
        orientation =
            offsetOrientation(orientation, glm::vec3(1, 0, 0), pitch, true);
        orientation =
            offsetOrientation(orientation, glm::vec3(0, 0, 1), roll, true);
    }
};

class Color {
  public:
    glm::vec4 inner;

    Color(glm::vec4 rgba) { inner = rgba; }
    Color(u32 argb) {
        inner = { ((argb >> 16) & 0xFF) / 256.0f,
            ((argb >> 8) & 0xFF) / 256.0f,
            ((argb >> 0) & 0xFF) / 256.0f,
            ((argb >> 24) & 0xFF) / 256.0f };
    }
};

auto camera = Camera();
auto car = Car();

class GameState {
  public:
    u32 count = 0;
    bool paused = false;

    bool pause_prev_pressed = false;
    bool next_prev_pressed = false;

    Mesh car_mesh;
    Mesh wheel_mesh;
    Mesh cube_2_mesh;

    Texture2D rainbow_texture = Texture2D::empty();
    Texture2D white_texture = Texture2D::empty();
    Texture2D car_texture = Texture2D::empty();

    ShaderProgram pbrShaderProgram;
    ShaderProgram shaderProgramOrange;
    ShaderProgram shaderProgramYellow;
    ShaderProgram guiShader;

    VAO VAOs[2] = { VAO::empty(), VAO::empty() };
    VAO gui_vao = VAO::empty();
    VAO pbrVAO = VAO::empty();

    VBO VBO0 = VBO::empty();
    VBO VBO1 = VBO::empty();
    VBO gui_vbo = VBO::empty();
    VBO pbrVBO = VBO::empty();

    std::vector<Particle> particles;

    Texture2D font_texture;

    void drawString(
        std::string text,
        float x,
        float y,
        Color color = 0xFFFFFFFF,
        bool shadow = true,
        int x_align = -1,
        int y_align = -1
    ) {
        std::vector<float> values;
        values.reserve(4 * 4 * text.length());

        std::vector<u32> indices;
        indices.reserve(6 * text.length());

        auto current_x = 0;
        auto current_y = 0;

        const f32 PIXEL_SIZE = 6.0f;

        for (auto c : text) {
            if (c == '\n') {
                current_x = 0;
                current_y += 1;
                continue;
            }

            if (c == ' ') {
                current_x += 1;
                continue;
            }

            c = c - '!';

            float x1 = (current_x + 0) * 8.0f;
            float x2 = (current_x + 1) * 8.0f;
            float y1 = (current_y + 0) * 8.0f;
            float y2 = (current_y + 1) * 8.0f;

            float u1 = 0.0f;
            float u2 = 1.0f;
            float v1 = c / 64.0f;
            float v2 = (c + 1) / 64.0f;

            values.push_back(x1);
            values.push_back(y1);
            values.push_back(u1);
            values.push_back(v1);

            values.push_back(x1);
            values.push_back(y2);
            values.push_back(u1);
            values.push_back(v2);

            values.push_back(x2);
            values.push_back(y2);
            values.push_back(u2);
            values.push_back(v2);

            values.push_back(x2);
            values.push_back(y1);
            values.push_back(u2);
            values.push_back(v1);

            current_x += 1;
        }

        for (auto i = 0; i < text.length(); i++) {
            indices.push_back(i * 4 + 0);
            indices.push_back(i * 4 + 1);
            indices.push_back(i * 4 + 3);
            indices.push_back(i * 4 + 1);
            indices.push_back(i * 4 + 2);
            indices.push_back(i * 4 + 3);
        }

        this->font_texture.bind();
        this->guiShader.use();
        auto transform = glm::mat4(1);
        transform = glm::translate(
            transform,
            glm::vec3(x_align == 0 ? 0 : -1, y_align == 0 ? 0 : +1, 0)
        );
        transform = glm::scale(
            transform,
            glm::vec3(PIXEL_SIZE / window_width, -PIXEL_SIZE / window_height, 1)
        );

        if (x_align == 0) {
            x -= text.length() * 8 / 2.0f;
        }

        if (y_align == 0) {
            y -= current_y * 8 / 2.0f;
        }

        this->gui_vbo.data(&values, GL_DYNAMIC_DRAW);
        this->gui_vao.bind();

        glDepthMask(false);

        if (shadow) {
            this->guiShader.uniform("color", { 0, 0, 0, 0.75 });
            this->guiShader.uniform(
                "MVP", glm::translate(transform, glm::vec3(x + 1, y + 1, 0))
            );
            glDrawElements(
                GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data()
            );
        }

        this->guiShader.uniform("color", color.inner);
        this->guiShader.uniform(
            "MVP", glm::translate(transform, glm::vec3(x, y, 0))
        );

        glDrawElements(
            GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data()
        );

        glDepthMask(true);
    }
};

void render(GameState* state);
void update(GLFWwindow* window, GameState* state);

int main() {
    u8 TEXTURE[27 * 4] = { 0 };
    u32 i = 0;
    for (u32 r = 0; r < 3; r++) {
        for (u32 g = 0; g < 3; g++) {
            for (u32 b = 0; b < 3; b++) {
                TEXTURE[i * 4 + 0] = r * 127;
                TEXTURE[i * 4 + 1] = g * 127;
                TEXTURE[i * 4 + 2] = b * 127;
                TEXTURE[i * 4 + 3] = (0xFF * (27) / 27);
                i += 1;
            }
        }
    }

    glewExperimental = true;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        window_width, window_height, "LearnOpenGL", NULL, NULL
    );
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSwapInterval(0);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

    std::cout << "" << std::endl;
    std::cout << "OpenGL Vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "OpenGL Shading Language Version: "
              << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
    std::cout << std::endl;

    glewExperimental = true; // Needed in core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    auto vertexShader = Shader::vertex(assets::shader::VERTEX);
    auto vertexPbrShader = Shader::vertex(assets::shader::VERTEX_PBR);
    auto fragmentShader = Shader::fragment(assets::shader::FRAGMENT);
    auto fragment1Shader = Shader::fragment(assets::shader::FRAGMENT_1);
    auto fragmentPbrShader = Shader::fragment(assets::shader::FRAGMENT_PBR);
    auto vertexGuiShader = Shader::vertex(assets::shader::VERTEX_GUI);
    auto fragmentGuiShader = Shader::fragment(assets::shader::FRAGMENT_GUI);

    GameState state = {
        .pbrShaderProgram =
            ShaderProgram::create({ &vertexPbrShader, &fragmentPbrShader }),
        .shaderProgramOrange =
            ShaderProgram::create({ &vertexShader, &fragmentShader }),
        .shaderProgramYellow =
            ShaderProgram::create({ &vertexShader, &fragment1Shader }),
        .guiShader = ShaderProgram::create(
            {
                &vertexGuiShader,
                &fragmentGuiShader,
            }
        ),
        .font_texture = fontTexture(),
    };
    state.car_mesh = Mesh::read("./car.obj");
    state.wheel_mesh = Mesh::read("./wheel.obj");
    state.cube_2_mesh = Mesh::read("./cube_2.obj");

    state.rainbow_texture = Texture2D::create(9, 3, TEXTURE, true);
    state.white_texture = Texture2D::create(1, 1, assets::texture::WHITE);
    state.car_texture = Texture2D::read("./car.png");

    state.shaderProgramOrange.uniform("MVP", glm::mat4(1));
    state.guiShader.uniform("color", glm::vec4(1));

    state.VAOs[0] = VAO::gen();
    state.VAOs[0].enableAttributeArray(0);
    state.VAOs[0].enableAttributeArray(1);

    state.VAOs[1] = VAO::gen();
    state.VAOs[1].enableAttributeArray(0);
    state.VAOs[1].enableAttributeArray(1);
    state.VAOs[1].enableAttributeArray(2);

    state.VAOs[0].bind();
    state.VBO0 = VBO::create(
        {
            { .type = GL_FLOAT, .amount = 3, .normalized = false },
            { .type = GL_FLOAT, .amount = 3, .normalized = false },
    }
    );
    state.VBO0.data(
        sizeof(assets::mesh::TRIANGLE), assets::mesh::TRIANGLE, GL_STATIC_DRAW
    );

    state.VAOs[1].bind();
    state.VBO1 = VBO::create(
        {
            { GL_FLOAT, 3 },
            { GL_FLOAT, 3 },
            { GL_FLOAT, 2 },
    }
    );

    state.pbrVAO = VAO::gen();
    state.pbrVAO.bind();
    state.pbrVAO.enableAttributeArray(0);
    state.pbrVAO.enableAttributeArray(1);
    state.pbrVAO.enableAttributeArray(2);
    state.pbrVAO.enableAttributeArray(3);
    state.pbrVBO = VBO::create(
        {
            { GL_FLOAT, 3 },
            { GL_FLOAT, 3 },
            { GL_FLOAT, 2 },
            { GL_FLOAT, 3 },
    }
    );

    state.gui_vao = VAO::gen();
    state.gui_vao.enableAttributeArray(0);
    state.gui_vao.enableAttributeArray(1);

    state.gui_vbo = VBO::create(
        {
            { GL_FLOAT, 2 },
            { GL_FLOAT, 2 },
    }
    );

    constexpr float MAX_FPS = 60.0f;
    constexpr float FRAME_TIME = 1.f / MAX_FPS;

    float last_time = glfwGetTime();
    float last_update_time = glfwGetTime();
    float accumulated_time = 0.f;

    while (!glfwWindowShouldClose(window)) {
        float current_time = glfwGetTime();
        float dt = current_time - last_time;
        last_time = current_time;
        accumulated_time += dt;

        glfwPollEvents();

        if (accumulated_time >= FRAME_TIME) {
            float update_dt = current_time - last_update_time;
            last_update_time = current_time;
            accumulated_time = 0.f;

            // printf("update_dt: %f\n", update_dt);
            // update(update_dt);

            state.count += 1;
            state.count %= 512;
            update(window, &state);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            render(&state);
            state.drawString(
                std::format("{:.2f} FPS", 1.0f / update_dt),
                2,
                2,
                0xFFFFFFFF,
                true,
                -1,
                0
            );

            glfwSwapBuffers(window);
        }
    }

    glfwTerminate();

    return 0;
}

void drawRect(
    GameState* state,
    float x1,
    float y1,
    float x2,
    float y2,
    Color color = 0xFFFFFFFF
) {
    std::vector<float> values;
    values.reserve(4 * 4);

    std::vector<u32> indices;
    indices.reserve(6);

    const f32 PIXEL_SIZE = 6.0f;

    float u1 = 0.0f;
    float u2 = 1.0f;
    float v1 = 0.0f;
    float v2 = 1.0f;

    values.push_back(x1);
    values.push_back(y1);
    values.push_back(u1);
    values.push_back(v1);

    values.push_back(x1);
    values.push_back(y2);
    values.push_back(u1);
    values.push_back(v2);

    values.push_back(x2);
    values.push_back(y2);
    values.push_back(u2);
    values.push_back(v2);

    values.push_back(x2);
    values.push_back(y1);
    values.push_back(u2);
    values.push_back(v1);

    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(3);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(3);

    state->white_texture.bind();
    state->guiShader.use();
    auto transform = glm::mat4(1);
    transform = glm::translate(transform, glm::vec3(-1, +1, 0));
    transform = glm::scale(
        transform,
        glm::vec3(
            1.0f / window_width * PIXEL_SIZE,
            -1.0f / window_height * PIXEL_SIZE,
            1
        )
    );

    state->gui_vbo.data(&values, GL_DYNAMIC_DRAW);
    state->gui_vao.bind();

    glDepthMask(false);

    state->guiShader.uniform("color", color.inner);
    state->guiShader.uniform(
        "MVP", glm::translate(transform, glm::vec3(0, 0, 0))
    );

    glDrawElements(
        GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, indices.data()
    );

    glDepthMask(true);
}

void render(GameState* state) {
    auto projection = glm::perspective(
        glm::radians(70.0f), (float) window_width / window_height, 0.1f, 100.0f
    );

    auto view = glm::lookAt(camera.position, camera.target, camera.up);

    Transform transform = {
        glm::vec3(0, 1, -1),
    };

    state->shaderProgramOrange.use();
    state->shaderProgramOrange.uniform(
        "MVP", projection * view * transform.matrix()
    );

    state->VAOs[0].bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);

    transform = {
        glm::vec3(1, 1, 0),
        glm::vec3(0.25),
        glm::vec3(0, 0, ((float) state->count) / 512.0f * 2.0f * M_PIf),
    };

    state->shaderProgramYellow.use();
    state->shaderProgramYellow.uniform(
        "MVP", projection * view * transform.matrix()
    );

    state->white_texture.bind();

    state->VBO1.data(
        sizeof(assets::mesh::CUBE), assets::mesh::CUBE, GL_STATIC_DRAW
    );
    state->VAOs[1].bind();
    // glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, assets::indices::CUBE);

    transform = {
        glm::vec3(0),
        glm::vec3(50),
        glm::vec3(glm::radians(90.0f), 0, 0),
    };

    state->shaderProgramYellow.uniform(
        "MVP", projection * view * transform.matrix()
    );

    state->rainbow_texture.bind();

    state->VBO1.data(
        sizeof(assets::mesh::PLANE), assets::mesh::PLANE, GL_STATIC_DRAW
    );
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, assets::indices::PLANE);

    transform = {
        car.position + glm::vec3(0, 0.25, 0),
        car.scale,
        car.orientation,
    };

    state->car_texture.bind();

    state->pbrShaderProgram.use();
    state->pbrShaderProgram.uniform(
        "MVP", projection * view * transform.matrix()
    );

    state->pbrVAO.bind();
    state->car_mesh.draw(&state->pbrVBO);

    std::vector<glm::vec2> wheel_positions = {
        glm::vec2(-1, -1),
        glm::vec2(1, -1),
        glm::vec2(1, 1),
        glm::vec2(-1, 1),
    };

    for (auto i = 0; i < 4; i++) {
        auto is_front = i < 2;
        auto pos = wheel_positions[i];
        auto wheel = Transform{
            .position =
                glm::vec3((0.9 + is_front * 0.1) * pos.x, 0, 1.2 * pos.y),
            .rotation = glm::vec3(
                0, -glm::radians(car.wheel_direction * is_front * 30.0f), 0
            ),
        };

        state->pbrShaderProgram.uniform(
            "MVP", projection * view * transform.matrix() * wheel.matrix()
        );
        state->wheel_mesh.draw(&state->pbrVBO);
    }

    drawRect(state, 1, 1, 2 + 12 + 9 * 8 + 1, 2 + 3 * 8 + 1, 0x5F000000);

    state->drawString("X", 2, 2, 0xFFFF3F3F);
    state->drawString("Y", 2, 2 + 8, 0xFF3FFF3F);
    state->drawString("Z", 2, 2 + 16, 0xFF3F3FFF);
    state->drawString(
        std::format(
            "{:+09.3f}\n{:+09.3f}\n{:+09.3f}",
            car.position.x,
            car.position.y,
            car.position.z
        ),
        2 + 12,
        2
    );

    drawRect(state, 1, 4 + 24, 2 + 12 + 6 * 8 + 1, 5 + 7 * 8 + 1, 0x5F000000);

    state->drawString("X", 2, 5 + 24, 0xFF3FFFFF);
    state->drawString("Y", 2, 5 + 32, 0xFFFF3FFF);
    state->drawString("Z", 2, 5 + 40, 0xFFFFFF3F);
    state->drawString("W", 2, 5 + 48, 0xFFBFBFBF);
    state->drawString(
        std::format(
            "{:+06.3f}\n{:+06.3f}\n{:+06.3f}\n{:+06.3f}\n{:+}",
            car.orientation.x,
            car.orientation.y,
            car.orientation.z,
            car.orientation.w,
            car.jump_state
        ),
        2 + 12,
        5 + 24
    );

    if (state->paused) {
        state->drawString("PAUSED", 0, 0, 0xFFFFBFBF, true, 0, 0);
    }
}

class Controls {
  public:
    bool pause;
    bool next_frame;

    float horizontal;
    float vertical;

    float camera_horizontal;
    float camera_vertical;

    bool boost;
    bool jump;

    bool exit;

    static Controls read(GLFWwindow* window) {
        Controls result;

        result.pause = glfwGetKey(window, GLFW_KEY_P);
        result.next_frame = glfwGetKey(window, GLFW_KEY_L);

        result.horizontal =
            glfwGetKey(window, GLFW_KEY_D) - glfwGetKey(window, GLFW_KEY_A);
        result.vertical =
            glfwGetKey(window, GLFW_KEY_W) - glfwGetKey(window, GLFW_KEY_S);

        result.camera_horizontal = glfwGetKey(window, GLFW_KEY_LEFT) -
                                   glfwGetKey(window, GLFW_KEY_RIGHT);
        result.camera_vertical =
            glfwGetKey(window, GLFW_KEY_UP) - glfwGetKey(window, GLFW_KEY_DOWN);

        result.boost = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT);
        result.jump = glfwGetKey(window, GLFW_KEY_SPACE);

        result.exit = glfwGetKey(window, GLFW_KEY_ESCAPE);

        for (int i = 0; i <= GLFW_JOYSTICK_LAST; i++) {
            auto present = glfwJoystickPresent(i);
            if (present) {
                int count;

                auto buttons = glfwGetJoystickButtons(i, &count);
                for (int j = 0; j < count; j++) {
                    printf("button %d = %d\n", j, buttons[j]);
                }

                auto axes = glfwGetJoystickAxes(i, &count);
                for (int j = 0; j < count; j++) {
                    printf("axis %d = %f\n", j, axes[j]);
                }

                result.jump = buttons[0];
                result.boost = buttons[1];

                result.horizontal = std::clamp(axes[0] * 2.0f, -1.0f, 1.0f);
                result.vertical = std::clamp(-axes[1] * 2.0f, -1.0f, 1.0f);
            }
        }

        return result;
    }
};

float min_abs(float x, float y) { return abs(x) < abs(y) ? x : y; }

void update(GLFWwindow* window, GameState* state) {
    auto controls = Controls::read(window);

    if (controls.exit) {
        glfwSetWindowShouldClose(window, true);
        return;
    }

    if (controls.pause) {
        if (!state->pause_prev_pressed) {
            state->paused = !state->paused;
        }
    }
    state->pause_prev_pressed = controls.pause;

    bool skip_frame =
        state->paused && !(controls.next_frame && !state->next_prev_pressed);
    state->next_prev_pressed = controls.next_frame;

    if (skip_frame)
        return;

    // camera.move(
    //     0, // glfwGetKey(window, GLFW_KEY_W),
    //     0, // glfwGetKey(window, GLFW_KEY_S),
    //     0, // glfwGetKey(window, GLFW_KEY_A),
    //     0, // glfwGetKey(window, GLFW_KEY_D),
    //     glfwGetKey(window, GLFW_KEY_E),
    //     glfwGetKey(window, GLFW_KEY_Q)
    // );

    // car.roll += camera_horizontal * 0.02f;
    // car.pitch -= camera_vertical * 0.02f;
    // camera_horizontal = 0;
    // camera_vertical = 0;

    car.move(
        controls.vertical, controls.horizontal, controls.jump, controls.boost
    );

    // car.position.y = 1.0f;
    if (glfwGetKey(window, GLFW_KEY_R)) {
        car.orientation = glm::quat();
    }

    auto on_floor = car.position.y <= 0.0f;

    if (on_floor) {
        auto yaw = car.yaw();
        auto angle = min_abs(
            min_abs(yaw - camera.yaw, 2.0f * pi + yaw - camera.yaw),
            -2.0f * pi + yaw - camera.yaw
        );
        camera.yaw += angle * 0.125f;
    }

    camera.yaw += controls.camera_horizontal * 30;

    while (camera.yaw > pi)
        camera.yaw -= 2.0f * pi;
    while (camera.yaw < -pi)
        camera.yaw += 2.0f * pi;

    glm::vec3 front = {
        sin(camera.yaw),
        0,
        cos(camera.yaw),
    };

    auto offset = controls.camera_vertical == 0
                      ? glm::normalize(glm::vec3(0, 0.5, 0) + front)
                  : controls.camera_vertical > 0 ? glm::vec3(0, 1, 0)
                                                 : front;

    camera.position = car.position + glm::vec3(0, 0.25, 0) + offset;

    offset = controls.camera_vertical > 0 ? glm::vec3(0, 0.5, 0) - front * 0.1f
                                          : glm::vec3(0, 0.5, 0) - front * 0.5f;

    camera.target = car.position + offset;

    camera.yaw -= controls.camera_horizontal * 30;

    for (auto& particle : state->particles) {
        particle.lifetime -= 1;
    }
    std::erase_if(state->particles, [](auto x) { return x.lifetime == 0; });
    state->particles.push_back({ car.position, glm::vec3(0.1), 20 });

    // camera.rotate(
    //     glfwGetKey(window, GLFW_KEY_LEFT),
    //     glfwGetKey(window, GLFW_KEY_RIGHT),
    //     glfwGetKey(window, GLFW_KEY_UP),
    //     glfwGetKey(window, GLFW_KEY_DOWN)
    // );
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    window_width = width;
    window_height = height;
}
