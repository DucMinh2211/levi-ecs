#pragma once

#include <string>
#include <unordered_map>

struct SDL_Renderer;
struct SDL_Texture;

namespace Levi {

    class AssetManager {
    public:
        AssetManager() = default;
        ~AssetManager();

        // Initializes the AssetManager with a renderer
        void init(SDL_Renderer* renderer);

        // Loads a texture from file. Returns existing one if already loaded.
        // Currently supports BMP (native SDL3).
        SDL_Texture* loadTexture(const std::string& path);

        // Removes a texture from memory
        void unloadTexture(const std::string& path);

        // Clears all loaded assets
        void clear();

    private:
        SDL_Renderer* renderer_ = nullptr;
        std::unordered_map<std::string, SDL_Texture*> textures_;
    };

}
