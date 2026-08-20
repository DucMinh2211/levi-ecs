#pragma once

#include <array>
#include <string>

namespace Levi {

    class Input {
    public:
        static Input& instance();

        void update();
        bool isKeyDown(const std::string& key) const;
        bool isKeyPressed(const std::string& key) const;
        bool isKeyReleased(const std::string& key) const;
        bool isMouseButtonDown(int button) const;
        bool isMouseButtonPressed(int button) const;
        float mouseX() const { return mouseX_; }
        float mouseY() const { return mouseY_; }

    private:
        std::array<bool, 512> keys_{};
        std::array<bool, 512> previousKeys_{};
        unsigned int mouseButtons_ = 0;
        unsigned int previousMouseButtons_ = 0;
        float mouseX_ = 0.0f;
        float mouseY_ = 0.0f;
    };

}
