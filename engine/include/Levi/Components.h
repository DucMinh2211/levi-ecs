#pragma once

#include <string>
#include "Math.h"

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
        Vector2 pivot = { 0.5f, 0.5f };
        PivotType pivotType = PivotType::Percent;
    };

    // Component for displaying 2D images
    struct Sprite2D {
        std::string texturePath;
        Vector2 size = { 64.0f, 64.0f }; // Frame size when rendering
        
        // You can add more info later like:
        // SDL_Texture* texture = nullptr; (Loaded by AssetManager)
        // int layer = 0; (Render depth)
    };

    // --- (Future suggestions) ---
    // struct Position3D : public Vector3 { ... };
}
