#pragma once

#include <glm/glm.hpp>

template <glm::length_t L, typename T, glm::qualifier Q>
struct std::formatter<glm::vec<L, T, Q>> : std::formatter<T> {
    template <typename FormatContext>
    auto format(const glm::vec<L, T, Q>& v, FormatContext& ctx) const {
        auto out = ctx.out();
        *out++ = '(';
        for (glm::length_t i = 0; i < L; ++i) {
            if (i > 0) {
                *out++ = ',';
                *out++ = ' ';
            }
            ctx.advance_to(out);
            out = std::formatter<T>::format(v[i], ctx);
        }
        *out++ = ')';
        return out;
    }
};

template <typename T, glm::qualifier Q>
struct std::formatter<glm::qua<T, Q>> : std::formatter<T> {
    template <typename FormatContext>
    auto format(const glm::qua<T, Q>& v, FormatContext& ctx) const {
        auto out = ctx.out();
        *out++ = '(';
        for (glm::length_t i = 0; i < 4; ++i) {
            if (i > 0) {
                *out++ = ',';
                *out++ = ' ';
            }
            ctx.advance_to(out);
            out = std::formatter<T>::format(v[i], ctx);
        }
        *out++ = ')';
        return out;
    }
};
