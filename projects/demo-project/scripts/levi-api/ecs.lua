---@meta

ECS = {}

---Create a new entity
---@param name? string Optional entity name
---@return integer id Entity ID
function ECS.createEntity(name) end

---Delete an entity
---@param id integer Entity ID
function ECS.deleteEntity(id) end

-- --- Position ---

---Add Position2D component to entity
---@param id integer Entity ID
---@param x number X coordinate
---@param y number Y coordinate
function ECS.addPosition(id, x, y) end

---Set entity position
---@param id integer Entity ID
---@param x number X coordinate
---@param y number Y coordinate
function ECS.setPosition(id, x, y) end

---Get entity position
---@param id integer Entity ID
---@return Position2D|nil
function ECS.getPosition(id) end

-- --- Scale ---

---Add Scale2D component to entity
---@param id integer Entity ID
---@param x number Scale X
---@param y number Scale Y
function ECS.addScale(id, x, y) end

---Set entity scale
---@param id integer Entity ID
---@param x number Scale X
---@param y number Scale Y
function ECS.setScale(id, x, y) end

---Get entity scale
---@param id integer Entity ID
---@return Scale2D|nil
function ECS.getScale(id) end

-- --- Rotation ---

---Add Rotation2D component to entity
---@param id integer Entity ID
---@param angle number Rotation angle in degrees
function ECS.addRotation(id, angle) end

---Set entity rotation
---@param id integer Entity ID
---@param angle number Rotation angle in degrees
function ECS.setRotation(id, angle) end

---Set rotation pivot
---@param id integer Entity ID
---@param x number Pivot X
---@param y number Pivot Y
---@param type PivotType Pivot type (Percent or Pixel)
function ECS.setRotationPivot(id, x, y, type) end

---Get entity rotation
---@param id integer Entity ID
---@return Rotation2D|nil
function ECS.getRotation(id) end

-- --- Sprite ---

---Add Sprite2D component to entity
---@param id integer Entity ID
---@param path string Texture path (relative to project root)
---@param width number Sprite width
---@param height number Sprite height
function ECS.addSprite(id, path, width, height) end

---Get entity sprite
---@param id integer Entity ID
---@return Sprite2D|nil
function ECS.getSprite(id) end

-- --- Dynamic Abstraction API (Phase 1 & 2) ---

---Mark a string as an asset path for the Inspector
---@param path string
---@return table
function ECS.AssetPath(path) end

---Define a new component type with default values
---@param name string Component name
---@param defaultValues table Key-value pairs of default fields
function ECS.defineComponent(name, defaultValues) end

---Add a defined script component to an entity
---@param id integer Entity ID
---@param schemaName string Name of the defined component
function ECS.addComponent(id, schemaName) end

---Set a value in a script component
---@param id integer Entity ID
---@param schemaName string Component name
---@param fieldName string Field name
---@param value any Value to set
function ECS.setComponentValue(id, schemaName, fieldName, value) end

---Get a value from a script component
---@param id integer Entity ID
---@param schemaName string Component name
---@param fieldName string Field name
---@return any
function ECS.getComponentValue(id, schemaName, fieldName) end

---Register a new logic system
---@param name string System name
---@param query string[] List of required component names
---@param callback fun(entityId: integer) Function called for each matching entity
function ECS.registerSystem(name, query, callback) end


---@class Vector2
---@field x number
---@field y number

---@class Position2D : Vector2
---@class Scale2D : Vector2

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
