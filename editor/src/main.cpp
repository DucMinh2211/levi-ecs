#include <iostream>
#include "Levi/Engine.h"

int main(int argc, char* argv[]) {
    std::cout << "--- Levi Studio Editor ---" << std::endl;
    
    Levi::EngineConfig config;
    config.WindowTitle = "Levi Editor v0.1.0";
    config.WindowWidth = 1600;
    config.WindowHeight = 900;

    Levi::EngineCore engine;
    if (engine.Init(config)) {
        engine.Run();
    }
    
    engine.Shutdown();

    return 0;
}
