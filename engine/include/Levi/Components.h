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

    // Rotation angle of a 2D entity (unit: degrees)
    struct Rotation2D {
        float angle = 0.0f;
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
