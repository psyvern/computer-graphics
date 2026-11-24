#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "types.h"
#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

template <glm::length_t L> struct Vertex {
    glm::vec<L, float, glm::defaultp> position;
    glm::vec<L, float, glm::defaultp> normal;
};

template <glm::length_t L> class Bezier {
  public:
    std::vector<glm::vec<L, float, glm::defaultp>> points;

    glm::vec<L, float, glm::defaultp> value(float t) {
        return value_internal(&this->points, t);
    }

    // Samples a value on the bezier curve, returning its position and its
    // derivative
    Vertex<L> sample(float t) { return sample_internal(&this->points, t); }

    float closestPoint(glm::vec3 position, u32 scans = 25, float ε = 1e-4) {
        int mindex;
        float min = INFINITY;
        for (int i = 0; i <= scans; i++) {
            auto d2 =
                glm::distance2(position, this->value(((float) i) / scans));
            if (d2 < min) {
                min = d2;
                mindex = i;
            }
        }
        auto m = fmax(((float) mindex - 1.0f) / (float) scans, 0.0f);
        auto n = fmin(((float) mindex + 1.0f) / (float) scans, 1.0f);

        float k;
        float d;
        while ((n - m) > ε) {
            k = (n + m) / 2;
            auto d1 = glm::distance2(position, this->value(k - ε));
            auto d2 = glm::distance2(position, this->value(k + ε));

            if (d1 < d2) {
                n = k;
                d = d1;
            } else {
                m = k;
                d = d2;
            }
        }

        return k;
    }

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
