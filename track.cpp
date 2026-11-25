#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "bezier.cpp"
#include "glm_format.cpp"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <print>
#include <ranges>

using glm::vec2, glm::vec3, glm::vec4, glm::quat, glm::mat4;

struct FindVertexResult {
    float t;
    float s;
    float distance;
    vec3 position;
    vec3 up;
};

struct Track {
    Bezier<3> path;
    Bezier<2> shape;
    size_t path_resolution;
    size_t shape_resolution;

    std::vector<std::vector<Vertex<3>>> rows;

    static Track build(
        Bezier<3> path,
        Bezier<2> shape,
        size_t path_resolution,
        size_t shape_resolution
    ) {
        auto loop = path.loop();
        std::vector<Vertex<3>> shape_points = {};
        std::vector<std::vector<Vertex<3>>> rows = {};

        for (size_t i = 0; i <= shape_resolution; i++) {
            auto [point, point_der] =
                shape.sample(((float) i) / (float) shape_resolution);

            shape_points.push_back(
                {
                    vec3(point, 0),
                    glm::normalize(
                        glm::cross(vec3(point_der, 0), vec3(0, 0, -1))
                    ),
                }
            );
        }

        for (size_t i = 0; i < path_resolution + (loop ? 0 : 1); i++) {
            auto [origin, forward] =
                path.sample(((float) i) / (float) path_resolution);

            std::vector<Vertex<3>> rs = {};
            for (auto point : shape_points) {
                auto t = glm::translate(mat4(1), origin);
                auto r = glm::toMat4(
                    glm::rotation(vec3(0, 0, -1), glm::normalize(forward))
                );

                auto new_point = vec3(t * r * vec4(point.position, 1));
                auto new_norm = vec3(r * vec4(point.normal, 1));
                std::println("{:.03f}, {:.03f}", new_point, new_norm);
                rs.push_back({ new_point, new_norm });
            }
            rows.push_back(rs);

            std::println();
        }

        return { path, shape, path_resolution, shape_resolution, rows };
    }

    std::tuple<std::vector<float>, std::vector<u32>> mesh() {
        auto path_loop = path.loop();
        auto shape_loop = shape.loop();

        std::vector<u32> indices = {};
        std::vector<float> data = {};
        int counter = 0;
        for (auto row : rows) {
            for (auto [i, vertex] : row | std::views::enumerate) {
                vec3 color;
                switch (counter) {
                case 0: {
                    color = vec3(1, 1, 1);
                    break;
                }
                case 1: {
                    color = vec3(1, 0, 0);
                    break;
                }
                case 2: {
                    color = vec3(1, 1, 0);
                    break;
                }
                case 3: {
                    color = vec3(0, 1, 0);
                    break;
                }
                case 4: {
                    color = vec3(0, 1, 1);
                    break;
                }
                case 5: {
                    color = vec3(0, 0, 1);
                    break;
                }
                case 6: {
                    color = vec3(1, 0, 1);
                    break;
                }
                }

                data.push_back(vertex.position.x);
                data.push_back(vertex.position.y);
                data.push_back(vertex.position.z);
                // auto middle = i == shape_resolution / 2;
                // data.push_back(middle ? 1.0f : 0.25f);
                // data.push_back(middle ? 1.0f : 0.25f);
                // data.push_back(middle ? 1.0f : 0.25f);
                data.push_back(color.x);
                data.push_back(color.y);
                data.push_back(color.z);
                data.push_back(0.0f);
                data.push_back(0.0f);
                // data.push_back(vertex.normal.x);
                // data.push_back(vertex.normal.y);
                // data.push_back(vertex.normal.z);
                data.push_back(0.0f);
                data.push_back(1.0f);
                data.push_back(0.0f);

                counter += 1;
                counter %= 7;
            }
        }

        for (auto i = 1; i < path_resolution + (path_loop ? 0 : 1); i++) {
            for (auto j = 1; j < shape_resolution + (shape_loop ? 0 : 1); j++) {
                auto x1 = (i - 1) * (shape_resolution + 1);
                auto x2 = i * (shape_resolution + 1);
                auto y1 = j - 1;
                auto y2 = j;
                indices.push_back(x1 + y1);
                indices.push_back(x1 + y2);
                indices.push_back(x2 + y2);
                indices.push_back(x1 + y1);
                indices.push_back(x2 + y2);
                indices.push_back(x2 + y1);
            }

            if (shape_loop) {
                auto x1 = (i - 1) * (shape_resolution + 1);
                auto x2 = i * (shape_resolution + 1);
                auto y1 = shape_resolution - 1;
                auto y2 = 0;
                indices.push_back(x1 + y1);
                indices.push_back(x1 + y2);
                indices.push_back(x2 + y2);
                indices.push_back(x1 + y1);
                indices.push_back(x2 + y2);
                indices.push_back(x2 + y1);
            }
        }

        if (path_loop) {
            for (auto j = 1; j <= shape_resolution + (shape_loop ? 0 : 1);
                j++) {
                auto x1 = (path_resolution - 1) * (shape_resolution + 1);
                auto x2 = 0 * (shape_resolution + 1);
                auto y1 = j - 1;
                auto y2 = j;
                indices.push_back(x1 + y1);
                indices.push_back(x1 + y2);
                indices.push_back(x2 + y2);
                indices.push_back(x1 + y1);
                indices.push_back(x2 + y2);
                indices.push_back(x2 + y1);
            }

            if (shape_loop) {
                auto x1 = (path_resolution - 1) * (shape_resolution + 1);
                auto x2 = 0 * (shape_resolution + 1);
                auto y1 = shape_resolution - 1;
                auto y2 = 0;
                indices.push_back(x1 + y1);
                indices.push_back(x1 + y2);
                indices.push_back(x2 + y2);
                indices.push_back(x1 + y1);
                indices.push_back(x2 + y2);
                indices.push_back(x2 + y1);
            }
        }

        return { data, indices };
    }

    FindVertexResult find_vertex(vec3 position) {
        auto t = path.closest_point(position, 64);
        auto [t_position, t_normal] = path.sample(t);

        auto r = glm::toMat4(glm::rotation(t_normal, vec3(0, 0, -1)));

        auto original_position = position;
        position =
            vec3(r * glm::translate(mat4(1), -t_position) * vec4(position, 1));

        Bezier<3> shape = {
            this->shape.points |
            std::views::transform([](auto in) { return vec3(in, 0); }) |
            std::ranges::to<std::vector>()
        };

        auto s = shape.closest_point(position, 64);

        auto [s_position, s_normal] = shape.sample(s);

        auto t_1 = glm::translate(mat4(1), t_position);
        auto t_2 = glm::translate(mat4(1), s_position);
        auto r_1 = glm::rotation(vec3(0, 0, -1), t_normal);
        auto r_2 = glm::rotation(vec3(1, 0, 0), s_normal);

        auto transform = t_1 * glm::toMat4(r_1) * t_2 * glm::toMat4(r_2);
        // Fix for when the normal should be straight down
        auto up = (1.0f + s_normal.x < glm::epsilon<float>())
                      ? (r_1 * vec3(0, -1, 0))
                      : (r_1 * r_2 * vec3(0, 1, 0));

        auto final_position = vec3(transform * vec4(0, 0, 0, 1));

        auto distance = glm::dot(up, original_position - final_position);

        return { t, s, distance, final_position, up };
    }
};
