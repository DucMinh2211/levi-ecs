---@meta

---@class Vector2
---@field x number
---@field y number

---@class Position2D: Vector2
---@class Scale2D: Vector2

---@enum PivotType
PivotType = {
    Percent = 0,
    Pixel = 1
}

---@class Rotation2D
---@field angle number
---@field pivot Vector2
---@field pivotType PivotType

---@class Sprite2D
---@field texturePath string
---@field size Vector2

---@class AABBCollider2D
---@field size Vector2
---@field offset Vector2

---@class CircleCollider2D
---@field radius number
---@field offset Vector2

ECS = {}

---@param name? string
---@return integer id
function ECS.createEntity(name) end

---@param id integer
function ECS.deleteEntity(id) end

---@param id integer
---@param x number
---@param y number
function ECS.addPosition(id, x, y) end

---@param id integer
---@param x number
---@param y number
function ECS.setPosition(id, x, y) end

---@param id integer
---@return Position2D?
function ECS.getPosition(id) end

---@param id integer
---@param x number
---@param y number
function ECS.addScale(id, x, y) end

---@param id integer
---@param x number
---@param y number
function ECS.setScale(id, x, y) end

---@param id integer
---@return Scale2D?
function ECS.getScale(id) end

---@param id integer
---@param angle number
function ECS.addRotation(id, angle) end

---@param id integer
---@param angle number
function ECS.setRotation(id, angle) end

---@param id integer
---@param x number
---@param y number
---@param pivotType PivotType
function ECS.setRotationPivot(id, x, y, pivotType) end

---@param id integer
---@return Rotation2D?
function ECS.getRotation(id) end

---@param id integer
---@param path string
---@param width number
---@param height number
function ECS.addSprite(id, path, width, height) end

---@param id integer
---@return Sprite2D?
function ECS.getSprite(id) end

---@param id integer
---@param width number
---@param height number
function ECS.addAABBCollider(id, width, height) end

---@param id integer
---@param radius number
function ECS.addCircleCollider(id, radius) end

---@param id integer
---@param zoom? number
function ECS.addCamera(id, zoom) end

---@param id integer
---@return Camera2D?
function ECS.getCamera(id) end

---@param path string
---@return table
function ECS.AssetPath(path) end

---@param name string
---@param defaultValues table
function ECS.defineComponent(name, defaultValues) end

---@param id integer
---@param schemaName string
function ECS.addComponent(id, schemaName) end

---@param id integer
---@param schemaName string
---@param fieldName string
---@param value any
function ECS.setComponentValue(id, schemaName, fieldName, value) end

---@param id integer
---@param schemaName string
---@param fieldName string
---@return any
function ECS.getComponentValue(id, schemaName, fieldName) end

---@param name string
---@param query string[]
---@param callback fun(entityId: integer)
function ECS.registerSystem(name, query, callback) end

Input = {}

---@param key string
---@return boolean
function Input.isKeyDown(key) end

---@param key string
---@return boolean
function Input.isKeyPressed(key) end

---@param key string
---@return boolean
function Input.isKeyReleased(key) end

---@param button integer
---@return boolean
function Input.isMouseButtonDown(button) end

---@param button integer
---@return boolean
function Input.isMouseButtonPressed(button) end

---@return Vector2
function Input.getMousePosition() end

Physics = {}

---@param first integer
---@param second integer
---@return boolean
function Physics.overlaps(first, second) end

---@param ax number
---@param ay number
---@param aw number
---@param ah number
---@param bx number
---@param by number
---@param bw number
---@param bh number
---@return boolean
function Physics.overlapsAABB(ax, ay, aw, ah, bx, by, bw, bh) end

---@param ax number
---@param ay number
---@param ar number
---@param bx number
---@param by number
---@param br number
---@return boolean
function Physics.overlapsCircle(ax, ay, ar, bx, by, br) end

Camera = {}

---@return integer?
function Camera.getActive() end

---@param id integer
---@return boolean
function Camera.setActive(id) end

---@param deltaX number
---@param deltaY number
---@return boolean
function Camera.move(deltaX, deltaY) end

---@param x number
---@param y number
---@return boolean
function Camera.setPosition(x, y) end

---@param zoom number
---@return boolean
function Camera.setZoom(zoom) end

---@return number
function Camera.getZoom() end

---@param intensity number
---@param duration number
---@return boolean
function Camera.shake(intensity, duration) end

---@class Camera2D
---@field zoom number
---@field active boolean
---@field maxShakeOffset number
---@field maxShakeRotation number

Scene = {}

---Queue a scene switch at the end of the current simulation frame.
---@param path string Project-relative .levscene.json or .levscene.bin path
---@return boolean queued
function Scene.load(path) end

---Queue a reload of the active scene.
---@return boolean queued
function Scene.reload() end

---Return the active scene path, relative to the project when possible.
---@return string
function Scene.current() end
