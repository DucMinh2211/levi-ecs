#include "Levi/AssetManager.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <iostream>

namespace Levi {

    AssetManager::~AssetManager() {
        clear();
    }

    void AssetManager::init(SDL_Renderer* renderer) {
        renderer_ = renderer;
    }

    SDL_Texture* AssetManager::loadTexture(const std::string& path) {
        // If texture already exists, return it
        if (textures_.find(path) != textures_.end()) {
            return textures_[path];
        }

        if (!renderer_) {
            std::cerr << "[AssetManager] Renderer not initialized." << std::endl;
            return nullptr;
        }

        // Load surface using SDL_image
        SDL_Surface* surface = IMG_Load(path.c_str());
        if (!surface) {
            std::cerr << "[AssetManager] Failed to load image: " << path << " - " << SDL_GetError() << std::endl;
            return nullptr;
        }

        // Create texture from surface
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
        SDL_DestroySurface(surface); // Surface is no longer needed after texture creation

        if (!texture) {
            std::cerr << "[AssetManager] Failed to create texture: " << path << " - " << SDL_GetError() << std::endl;
            return nullptr;
        }

        // Cache the texture
        textures_[path] = texture;
        std::cout << "[AssetManager] Texture loaded: " << path << std::endl;
        
        return texture;
    }

    void AssetManager::unloadTexture(const std::string& path) {
        auto it = textures_.find(path);
        if (it != textures_.end()) {
            SDL_DestroyTexture(it->second);
            textures_.erase(it);
            std::cout << "[AssetManager] Texture unloaded: " << path << std::endl;
        }
    }

    void AssetManager::clear() {
        for (auto& pair : textures_) {
            SDL_DestroyTexture(pair.second);
        }
        textures_.clear();
        std::cout << "[AssetManager] All textures cleared." << std::endl;
    }

}
