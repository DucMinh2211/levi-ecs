# 📄 Levi-ECS Engine - Documentation

## 1. Giới thiệu dự án
...
* **Công nghệ cốt lõi:**
    * **C++20**
    * **SDL3:** Xử lý Windowing, Input và 2D Rendering hiện đại.
    * **Dear ImGui (Docking):** Xây dựng giao diện Editor chuyên nghiệp.
    * **Flecs (v4):** Hệ thống ECS mạnh mẽ và linh hoạt.
    * **cr.h:** Cơ chế Hot-reloading cho C++.

## 2. Tính năng chính (2D Focus)
...
* **Rendering:** 
    * **SDL_Renderer:** Sử dụng Hardware Acceleration để vẽ 2D mượt mà.
    * **2D Sprite:** Hiển thị hình ảnh thông qua SDL_Texture.
    * **Animation:** Hệ thống Sprite Animation dựa trên khung hình (Frame-based).
    * **Lighting 2D:** Hỗ trợ các hiệu ứng ánh sáng cơ bản (Point Light, Global Ambient).
    * **Particle System:** Hiệu ứng hạt cho khói, lửa, nổ.
* **Vật lý 2D:** Tích hợp Box2D cho va chạm (Colliders), trọng lực và lực đẩy.
* **Tilemap Editor:** Công cụ vẽ map theo ô lưới trực tiếp trong Editor.
* **Camera 2D:** Hệ thống camera hỗ trợ Zoom, Smooth Follow và Screen Shake.
* **Input System:** Quản lý phím bấm, chuột và tay cầm (Controller) linh hoạt.
* **Âm thanh:** Hỗ trợ phát nhạc nền (BGM) và hiệu ứng âm thanh (SFX).
* **Quản lý Tài nguyên (Asset Browser):** Import và quản lý Texture, Audio, Font trực quan.
* **Quản lý Màn chơi (Scene Management):** Lưu và nạp (Save/Load) toàn bộ trạng thái thế giới game qua định dạng JSON hoặc YAML.

## 3. Giao diện Editor (GUI)
Editor được xây dựng trên nền tảng **Dear ImGui**, cung cấp các công cụ trực quan để quản lý thế giới game:
* **Scene Hierarchy:** Hiển thị danh sách các Entity hiện có trong ECS World.
* **Inspector:** Chỉnh sửa thuộc tính của các Component (Position, Velocity, Sprite, v.v.) trong thời gian thực.
* **System Monitor:** Theo dõi hiệu năng và trạng thái hoạt động của các System.
* **Console:** Hiển thị log từ Engine và Logic game (Hot-reloaded logic).

## 4. Cấu trúc thư mục & Kiến trúc
Dự án được phân tách rõ ràng giữa **Core Engine** và **Editor Application** để tối ưu hóa việc đóng gói và phát hành game sau này:

| Thư mục | Thành phần | Nhiệm vụ | Đầu ra (Output) |
| :--- | :--- | :--- | :--- |
| **`engine/`** | **Core Library** | Chứa logic vận hành game: Rendering (SDL3), ECS (Flecs), Vật lý, Toán học. | `LeviEngine.lib` (Static Library) |
| **`editor/`** | **Studio App** | Chứa giao diện điều khiển (Dear ImGui), Scene Hierarchy, Inspector, quản lý Project. | `LeviEditor.exe` (Executable) |
| **`projects/`** | **User Game** | Chứa code logic và tài nguyên của người dùng (được nạp vào qua Hot-reloading). | `game_logic.dll` (Shared Library) |

### Tại sao cần tách Engine và Editor?
1. **Tính độc lập:** Code giao diện (UI) của Editor không làm ảnh hưởng đến hiệu năng cốt lõi của Engine.
2. **Đóng gói (Shipping):** Khi phát hành game cho người chơi, chúng ta chỉ cần đóng gói `engine/` và code của người dùng, loại bỏ hoàn toàn phần `editor/` cồng kềnh.
3. **Mở rộng:** Dễ dàng bảo trì và thêm tính năng mới cho công cụ mà không sợ phá vỡ logic game.

---

## 5. Hướng dẫn phát triển
### Build hệ thống
Sử dụng script `scripts/build.ps1` để biên dịch toàn bộ dự án.
```powershell
.\scripts\build.ps1
```
### Quy trình Hot-reloading
1. Chỉnh sửa code trong `projects/demo-project/src/`.
2. Biên dịch code logic thành `game_logic.dll`.
3. Engine sẽ tự động phát hiện thay đổi và nạp lại logic mới mà không mất dữ liệu ECS.
