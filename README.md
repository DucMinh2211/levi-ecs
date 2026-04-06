# 📄 Levi-ECS Engine

**Levi** is a high-performance 2D Game Engine focused on **Entity Component System (ECS)** architecture and **C++ Hot-reloading**. Built for modern game development workflows with a minimal feedback loop.

**Hot Reloading with Lua**

![Hot Reloading with Lua Demo](demo/levi-demo-gif.gif)
---

## 🚀 Key Features
*   **Engine Core:** Powered by **C++20** and **SDL3**.
*   **Architecture:** Data-oriented design using **Flecs (v4)** ECS.
*   **Hot-reloading:** Change game logic on the fly using **Lua scripting** (via **sol2**).
*   **2D Rendering:** Hardware-accelerated sprites, animations, and lighting.
*   **Studio Editor:** Integrated GUI built with **Dear ImGui (Docking)**.
*   **Cross-platform:** Support for Windows and Linux via **CMake**.

---
## Demo

https://github.com/user-attachments/assets/11788033-533e-47e3-a2be-a44198be4eff

---

## 🛠️ Build Instructions

### Prerequisites
*   **C++20 Compiler:** (MSVC 2022, GCC 11+, or Clang 13+)
*   **CMake:** version 3.20 or higher.
*   **Git:** To fetch third-party dependencies.

### 🪟 Windows (Visual Studio)
1.  Clone the repository:
    ```bash
    git clone https://github.com/your-username/levi.git
    cd levi
    ```
2.  **Using helper script:**
    ```powershell
    .\scripts\build.ps1
    ```
3.  **Using CMake manually:**
    ```bash
    mkdir build
    cmake -B build -S .
    cmake --build build --config Debug
    ```
4.  Run the editor from `bin/LeviEditor.exe`.

### 🐧 Linux (Ubuntu/Debian)
1.  Install dependencies (SDL3 requirements):
    ```bash
    sudo apt-get update
    sudo apt-get install build-essential git cmake libx11-dev libxext-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libwayland-dev libxkbcommon-dev
    ```
2.  Clone and Build:
    ```bash
    git clone https://github.com/your-username/levi.git
    cd levi
    mkdir build
    cmake -B build -S .
    cmake --build build --config Debug
    ```
3.  Run the editor:
    ```bash
    ./bin/LeviEditor
    ```

---

## 📂 Project Structure
*   **`engine/`**: Core engine logic (ECS, Rendering, Math, Physics, Lua integration).
*   **`editor/`**: Studio GUI and developer tools.
*   **`projects/`**: User game projects (Lua scripts with hot-reloading support).
*   **`bin/`**: Compiled executables and binaries.
*   **`lib/`**: Compiled static libraries.

---

## 📄 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
