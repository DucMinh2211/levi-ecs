#include <iostream>

// This is intended to be a DLL/Shared Library for hot-reloading
// Using Levi ECS Engine

void OnUpdate(float dt) {
    // Logic goes here
    // std::cout << "Demo Project: Updating... (" << dt << "s)" << std::endl;
}

void OnInit() {
    std::cout << "Demo Project: Initialized." << std::endl;
}
