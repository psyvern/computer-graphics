#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "types.h"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

template <glm::length_t L> struct Vertex {
    glm::vec<L, float, glm::defaultp> position;
    glm::vec<L, float, glm::defaultp> normal;
};

float plane_point_distance(Vertex<3> plane, glm::vec3 point) {
    return glm::dot(plane.normal, point - plane.position);
}

template <glm::length_t L> class Bezier {
  public:
    std::vector<glm::vec<L, float, glm::defaultp>> points;

    glm::vec<L, float, glm::defaultp> value(float t) {
        return value_internal(&this->points, t);
    }

    // Samples a value on the bezier curve, returning its position and its
    // derivative
    Vertex<L> sample(float t) { return sample_internal(&this->points, t); }

    float closest_point(glm::vec3 position, size_t scans = 25, float ε = 1e-4) {
        bool loop = this->loop();

        size_t index;
        float min = INFINITY;

        for (int i = 0; i < scans + (loop ? 0 : 1); i++) {
            auto d = glm::distance2(position, this->value(((float) i) / scans));

            if (d < min) {
                index = i;
                min = d;
            }
        }

        min = plane_point_distance(
            this->sample(((float) index) / scans), position
        );

        size_t index_1, index_2;

        if (loop) {
            if (min > 0.0f) {
                index_1 = index;
                index_2 = index + 1;
            } else if (min < 0.0f) {
                if (index == 0) {
                    index_1 = scans - 1;
                    index_2 = scans;
                } else {
                    index_1 = index - 1;
                    index_2 = index;
                }
            }
        } else {
            if (min > 0.0f) {
                if (index == scans) {
                    index_1 = index;
                    index_2 = index;
                } else {
                    index_1 = index;
                    index_2 = index + 1;
                }
            } else {
                if (index == 0) {
                    index_1 = index;
                    index_2 = index;
                } else {
                    index_1 = index - 1;
                    index_2 = index;
                }
            }
        }

        auto m = (float) index_1 / scans;
        auto n = (float) index_2 / scans;

        while (true) {
            float k = (m + n) / 2;
            if (n - m < ε)
                return k;

            auto d1 = glm::distance2(position, this->value(m));
            auto d2 = glm::distance2(position, this->value(n));

            // auto d1 = fabs(plane_point_distance(this->sample(m), position));
            // auto d2 = fabs(plane_point_distance(this->sample(n), position));

            if (d1 < d2) {
                n = k;
            } else {
                m = k;
            }
        }
    }

    bool loop() { return points.front() == points.back(); }

  private:
    static glm::vec<L, float, glm::defaultp> value_internal(
        std::vector<glm::vec<L, float, glm::defaultp>>* points,
        float t
    ) {
        if (points->size() == 1)
            return points->front();

        std::vector<glm::vec<L, float, glm::defaultp>> next = {};
        next.reserve(points->size() - 1);

        for (int i = 1; i < points->size(); i++) {
            auto a = points->at(i - 1);
            auto b = points->at(i);

            next.push_back((b - a) * t + a);
        }

        return value_internal(&next, t);
    }

    static Vertex<L> sample_internal(
        std::vector<glm::vec<L, float, glm::defaultp>>* points,
        float t
    ) {
        if (points->size() == 2) {
            auto a = points->at(0);
            auto b = points->at(1);
            return { (b - a) * t + a, glm::normalize(b - a) };
        }

        std::vector<glm::vec<L, float, glm::defaultp>> next = {};
        next.reserve(points->size() - 1);

        for (int i = 1; i < points->size(); i++) {
            auto a = points->at(i - 1);
            auto b = points->at(i);

            next.push_back((b - a) * t + a);
        }

        return sample_internal(&next, t);
    }
};
