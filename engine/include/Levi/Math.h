#pragma once

namespace Levi {
    // Basic mathematical data type for 2D
    struct Vector2 {
        float x = 0.0f;
        float y = 0.0f;

        Vector2() = default;
        Vector2(float _x, float _y) : x(_x), y(_y) {}
    };

    // Basic mathematical data type for 3D
    struct Vector3 {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;

        Vector3() = default;
        Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    };
}
