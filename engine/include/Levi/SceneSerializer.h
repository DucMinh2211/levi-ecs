#pragma once

#include <flecs.h>

#include <filesystem>
#include <string>
#include <vector>

namespace Levi {

    class SceneSerializer {
    public:
        static std::string toJson(flecs::world& world);
        static bool fromJson(flecs::world& world, const std::string& json, std::string* error = nullptr);
        static bool saveJson(flecs::world& world, const std::filesystem::path& path, std::string* error = nullptr);
        static bool loadJson(flecs::world& world, const std::filesystem::path& path, std::string* error = nullptr);

        static std::vector<unsigned char> toBinary(flecs::world& world);
        static bool fromBinary(flecs::world& world, const std::vector<unsigned char>& data,
                               std::string* error = nullptr);
        static bool saveBinary(flecs::world& world, const std::filesystem::path& path, std::string* error = nullptr);
        static bool loadBinary(flecs::world& world, const std::filesystem::path& path, std::string* error = nullptr);
    };

} // namespace Levi
