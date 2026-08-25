#pragma once

#include "Math.h"
#include <cstdint>
#include <string>

namespace Levi {

    // --- 2D Components ---

    // Entity position in 2D space
    struct Position2D : public Vector2 {
        using Vector2::Vector2; // Inherit constructors
    };

    // Scale (multiplier) of a 2D entity
    struct Scale2D : public Vector2 {
        Scale2D() : Vector2(1.0f, 1.0f) {} // Default is 1.0
        using Vector2::Vector2;
    };

    // Pivot point type
    enum class PivotType {
        Percent, // 0.0 to 1.0 (0.5 is center)
        Pixel    // Absolute pixel offset
    };

    // Rotation angle and pivot of a 2D entity (unit: degrees)
    struct Rotation2D {
        float angle = 0.0f;
        Vector2 pivot = {0.5f, 0.5f};
        PivotType pivotType = PivotType::Percent;
    };

    // Component for displaying 2D images
    struct Sprite2D {
        std::string texturePath;
        Vector2 size = {64.0f, 64.0f}; // Frame size when rendering

        // You can add more info later like:
        // SDL_Texture* texture = nullptr; (Loaded by AssetManager)
        // int layer = 0; (Render depth)
    };

    struct AABBCollider2D {
        Vector2 size = {64.0f, 64.0f};
        Vector2 offset = {0.0f, 0.0f};
    };

    struct CircleCollider2D {
        float radius = 32.0f;
        Vector2 offset = {0.0f, 0.0f};
    };

    enum class RigidBodyType2D { Static, Kinematic, Dynamic };

    struct RigidBody2D {
        RigidBodyType2D type = RigidBodyType2D::Dynamic;
        Vector2 linearVelocity = {0.0f, 0.0f}; // Engine units (pixels) per second.
        float angularVelocity = 0.0f;          // Degrees per second.
        float gravityScale = 1.0f;
        float linearDamping = 0.0f;
        float angularDamping = 0.0f;
        float density = 1.0f;
        float friction = 0.6f;
        float restitution = 0.0f;
        bool fixedRotation = false;
        bool bullet = false;
        bool enabled = true;
        bool sensor = false;
        std::uint32_t categoryBits = 1;
        std::uint32_t maskBits = 0xFFFFFFFFu;
    };

    struct Camera2D {
        float zoom = 1.0f;
        bool active = true;
        float maxShakeOffset = 12.0f;
        float maxShakeRotation = 2.0f;

        // Runtime-only shake state. Scene persistence intentionally stores
        // only the configuration above.
        float shakeIntensity = 0.0f;
        float shakeDuration = 0.0f;
        float shakeRemaining = 0.0f;
        float shakeClock = 0.0f;
        Vector2 shakeOffset = {0.0f, 0.0f};
        float shakeRotation = 0.0f;
    };

    // --- (Future suggestions) ---
    // struct Position3D : public Vector3 { ... };
} // namespace Levi
