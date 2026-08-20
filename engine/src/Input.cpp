#include "Levi/Input.h"

#include <SDL3/SDL.h>

#include <algorithm>

namespace Levi {

    Input& Input::instance() {
        static Input input;
        return input;
    }

    void Input::update() {
        previousKeys_ = keys_;
        int count = 0;
        const bool* state = SDL_GetKeyboardState(&count);
        keys_.fill(false);
        const int copyCount = std::min(count, static_cast<int>(keys_.size()));
        for (int i = 0; i < copyCount; ++i) keys_[i] = state[i];

        previousMouseButtons_ = mouseButtons_;
        mouseButtons_ = static_cast<unsigned int>(SDL_GetMouseState(&mouseX_, &mouseY_));
    }

    static SDL_Scancode scancodeFor(const std::string& key) {
        return SDL_GetScancodeFromName(key.c_str());
    }

    bool Input::isKeyDown(const std::string& key) const {
        const auto code = scancodeFor(key);
        return code > SDL_SCANCODE_UNKNOWN && static_cast<std::size_t>(code) < keys_.size() && keys_[code];
    }

    bool Input::isKeyPressed(const std::string& key) const {
        const auto code = scancodeFor(key);
        return code > SDL_SCANCODE_UNKNOWN && static_cast<std::size_t>(code) < keys_.size()
            && keys_[code] && !previousKeys_[code];
    }

    bool Input::isKeyReleased(const std::string& key) const {
        const auto code = scancodeFor(key);
        return code > SDL_SCANCODE_UNKNOWN && static_cast<std::size_t>(code) < keys_.size()
            && !keys_[code] && previousKeys_[code];
    }

    static unsigned int mouseMask(int button) {
        return button > 0 ? SDL_BUTTON_MASK(button) : 0;
    }

    bool Input::isMouseButtonDown(int button) const {
        return (mouseButtons_ & mouseMask(button)) != 0;
    }

    bool Input::isMouseButtonPressed(int button) const {
        const auto mask = mouseMask(button);
        return (mouseButtons_ & mask) != 0 && (previousMouseButtons_ & mask) == 0;
    }

}
