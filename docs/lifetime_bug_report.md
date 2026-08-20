# Báo cáo: xử lý shutdown crash và ổn định hot reload

## 1. Sự cố ban đầu

Crash xuất hiện do vòng đời của ba nhóm tài nguyên không đồng bộ:

- `sol::environment` và `sol::protected_function` còn tham chiếu tới `lua_State`.
- `SystemManager` vẫn giữ callback Lua sau khi VM bắt đầu bị hủy.
- Script cleanup tiếp tục truy cập hoặc hủy Flecs entity khi `flecs::world` không còn hợp lệ.

Khi Lua GC hoặc destructor của Sol2 chạy sai thứ tự, callback có thể cố giải phóng reference trên một `lua_State` đã chết. Theo chiều ngược lại, callback `onShutdown` hoặc entity cleanup có thể truy cập một Flecs world đã bị hủy.

## 2. Thứ tự shutdown mới

Thứ tự teardown hiện tại:

```text
onStop/onShutdown
        ↓
Hủy entity do script tạo
        ↓
Xóa callback trong SystemManager
        ↓
Giải phóng protected_function và environment
        ↓
Xóa các Lua global table
        ↓
Lua collect_garbage + reset lua_State
        ↓
Ngắt con trỏ Flecs world
        ↓
Hủy Flecs world sau cùng
```

Implementation nằm tại [LuaScriptManager.cpp](C:/Users/khach/projects/levi-ecs/engine/src/LuaScriptManager.cpp:324).

Các điểm quan trọng:

- Nếu runtime còn hoạt động, `stopRuntime()` luôn được gọi trước.
- `SystemManager::clear()` giải phóng tất cả callback trước khi reset Lua VM.
- `onInit`, `onUpdate`, `onShutdown` và `sol::environment` được gán `nil` trước Lua GC.
- Lua global `ECS`, `Input`, `Physics` được gỡ bỏ.
- `world_` chỉ được đặt `nullptr` sau khi entity cleanup hoàn tất.
- `initialized_` giúp shutdown có tính idempotent; destructor gọi lại sẽ không cleanup lần hai.

## 3. Đảm bảo Flecs world sống lâu hơn Lua

Trong `EngineCore`, member được khai báo theo thứ tự:

```cpp
flecs::world world_;
AssetManager assetManager_;
LuaScriptManager luaScriptManager_;
```

C++ hủy member theo thứ tự ngược, vì vậy `LuaScriptManager` bị hủy trước và `world_` bị hủy cuối cùng: [Engine.h](C:/Users/khach/projects/levi-ecs/engine/include/Levi/Engine.h:72).

Ngoài ra, `EngineCore::shutdown()` chủ động unload project trước khi giải phóng ImGui, renderer, viewport texture và SDL: [Engine.cpp](C:/Users/khach/projects/levi-ecs/engine/src/Engine.cpp:241).

## 4. Lifecycle Play/Stop

Lifecycle Lua được tách khỏi quá trình chỉ load script:

- Load project: tạo VM, nạp script, đăng ký schema và systems.
- Play: chạy `onInit`, sau đó `onPlay`.
- Stop: chạy `onStop`, `onShutdown`, hủy entity runtime và restore snapshot.
- Edit mode không tự chạy `onInit`.

Phần triển khai nằm tại:

- [startRuntime](C:/Users/khach/projects/levi-ecs/engine/src/LuaScriptManager.cpp:479)
- [stopRuntime](C:/Users/khach/projects/levi-ecs/engine/src/LuaScriptManager.cpp:492)

Mỗi script có danh sách `createdEntities`. `ECS.createEntity()` tự ghi lại ID, nhờ đó Stop và hot reload chỉ cleanup entity thuộc script tương ứng.

Các thao tác lifecycle còn suspend Flecs deferral khi cần, tránh để lệnh destruct nằm trong hàng đợi trong lúc scene đang được restore.

## 5. Ổn định hot reload

Mỗi frame, editor kiểm tra timestamp script trước khi chạy simulation: [Engine.cpp](C:/Users/khach/projects/levi-ecs/engine/src/Engine.cpp:194).

Khi một script thay đổi:

1. Nếu đang Playing, gọi `onShutdown` của environment cũ.
2. Hủy toàn bộ entity do script đó tạo.
3. Xóa danh sách entity cũ.
4. Tạo environment mới và nạp lại file.
5. Chỉ chạy lại `onInit` nếu runtime đang hoạt động.
6. System trùng tên được cập nhật callback thay vì thêm system mới.

Nhờ đó không còn callback trỏ vào environment cũ, entity nhân đôi hoặc Lua system chạy nhiều lần sau mỗi lần save.

## 6. Ổn định viewport

Hot reload không tái tạo:

- SDL renderer.
- Viewport texture.
- Flecs world.
- Render module hoặc AssetManager.

Nó chỉ thay environment và callback của script liên quan. Vì vậy viewport tiếp tục render bằng cùng texture/render pipeline.

Lua update và Lua systems chạy bên trong `world_.defer_begin()/defer_end()`, tránh thay đổi archetype trong khi Flecs đang iterate query.

## 7. Ổn định Scene Hierarchy và Inspector

Hierarchy không cache danh sách entity qua nhiều frame. Mỗi frame nó query lại world và kiểm tra entity còn sống: [SceneHierarchy.cpp](C:/Users/khach/projects/levi-ecs/editor/src/SceneHierarchy.cpp:11).

Các cải tiến liên quan:

- Root query dùng `without(ChildOf, Wildcard)` để child không bị hiển thị lặp.
- Child cũng được kiểm tra `is_alive`.
- Flecs modules, systems, components và Lua schema metadata bị lọc khỏi hierarchy.
- `Health`, `SpinData` được đánh dấu bằng `ScriptComponentSchemaTag`, nên không còn hiện như scene entity.
- Inspector kiểm tra entity hợp lệ trước khi đọc component: [Inspector.cpp](C:/Users/khach/projects/levi-ecs/editor/src/Inspector.cpp:73).
- Sau Stop hoặc Load Scene, selection được reset để không giữ handle của entity đã bị recreate.

## 8. Xác minh

Test lifecycle chạy hai chu kỳ liên tiếp trên cùng Flecs world và kiểm tra:

- Runtime entity được tạo khi Play.
- Runtime entity biến mất sau Stop.
- Không còn Lua callback trong `SystemManager`.
- Không còn script schema sau shutdown.
- Lua manager trở về trạng thái chưa khởi tạo.

Test nằm tại [LuaScriptManagerLifecycleTest.cpp](C:/Users/khach/projects/levi-ecs/tests/LuaScriptManagerLifecycleTest.cpp:19).

Kết quả hiện tại:

- `LeviEditor` build thành công.
- **5/5 test pass**.
- Chưa có automated GUI/GPU test; độ ổn định viewport/hierarchy hiện được bảo vệ bằng lifecycle test, serializer test và các kiểm tra entity hợp lệ trong editor.
