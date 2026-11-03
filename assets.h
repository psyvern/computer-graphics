#include "types.h"

namespace assets {
    namespace shader {
        const char VERTEX[] = {
#embed "vertex.glsl"
        };

        const char VERTEX_PBR[] = {
#embed "vertex_pbr.glsl"
        };

        const char VERTEX_GUI[] = {
#embed "vertex_gui.glsl"
        };

        const char FRAGMENT[] = {
#embed "fragment.glsl"
        };

        const char FRAGMENT_1[] = {
#embed "fragment1.glsl"
        };

        const char FRAGMENT_PBR[] = {
#embed "fragment_pbr.glsl"
        };

        const char FRAGMENT_GUI[] = {
#embed "fragment_gui.glsl"
        };
    }

    namespace texture {
        u8 WHITE[] = { 0xFF, 0xFF, 0xFF, 0xFF };

        u8 WHITE_FAKE[] = { 0xFF,
            0xFF,
            0xFF,
            0xFF,
            0xFF,
            0x00,
            0x00,
            0xFF,
            0x00,
            0xFF,
            0x00,
            0xFF,
            0x00,
            0x00,
            0xFF,
            0xFF };
    }

    namespace mesh {
        float TRIANGLE[] = {
            0.45f,
            -0.5f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,

            0.0f,
            0.5f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,

            -0.45f,
            -0.5f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
        };

        float PLANE[] = {
            -0.5f,
            -0.5f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            0.0f,

            0.5f,
            -0.5f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,

            0.5f,
            0.5f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,

            -0.5f,
            0.5f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
        };

        float PLANE_2[] = {
            0.5f,
            -0.5f,
            0.0f,
            1.0f,
            0.0f,

            0.5f,
            0.5f,
            0.0f,
            1.0f,
            1.0f,

            -0.5f,
            0.5f,
            0.0f,
            0.0f,
            1.0f,

            -0.5f,
            -0.5f,
            0.0f,
            0.0f,
            0.0f,
        };

        float CUBE[] = {
            1.0f,
            1.0f,
            -1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,

            1.0f,
            -1.0f,
            -1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            0.0f,

            1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,

            1.0f,
            -1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,

            -1.0f,
            1.0f,
            -1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,

            -1.0f,
            -1.0f,
            -1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,

            -1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
            0.0f,

            -1.0f,
            -1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            1.0f,
            0.0f,
        };
    }

    namespace indices {
        u32 PLANE[] = { 0, 1, 2, 0, 2, 3 };

        u32 CUBE[] = {
            0,
            1,
            2,
            1,
            2,
            3,
            4,
            5,
            6,
            5,
            6,
            7,
            0,
            2,
            4,
            2,
            4,
            6,
            1,
            3,
            5,
            3,
            5,
            7,
            2,
            3,
            6,
            3,
            6,
            7,
            0,
            1,
            4,
            1,
            4,
            5,
        };
    }
}
