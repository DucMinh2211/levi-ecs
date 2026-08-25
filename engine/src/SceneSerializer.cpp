#include "Levi/SceneSerializer.h"

#include "Levi/Components.h"
#include "Levi/ScriptComponent.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <variant>

namespace Levi {
    namespace {

        struct ScriptRecord {
            std::string schema;
            std::map<std::string, ScriptValue> values;
        };

        struct EntityRecord {
            int parent = -1;
            std::string name;
            std::optional<Position2D> position;
            std::optional<Scale2D> scale;
            std::optional<Rotation2D> rotation;
            std::optional<Sprite2D> sprite;
            std::optional<AABBCollider2D> aabb;
            std::optional<CircleCollider2D> circle;
            std::optional<RigidBody2D> rigidBody;
            std::optional<Camera2D> camera;
            std::vector<ScriptRecord> scripts;
        };

        using Scene = std::vector<EntityRecord>;

        bool isSceneEntity(flecs::entity entity) {
            if (!entity || !entity.is_alive()) return false;
            if (flecs::strip_generation(entity.id()) < EcsFirstUserEntityId) return false;
            const std::string path = entity.path().c_str();
            if (path.rfind("::flecs", 0) == 0) return false;
            if (entity.has(flecs::Module) || entity.has<flecs::Component>() || entity.has(flecs::System)) return false;
            if (entity.has<ScriptComponentSchemaTag>()) return false;
            const std::string name = entity.name().c_str();
            if (!name.empty() && (name[0] == '$' || name == "World" || name == "Query" || name == "Observer"))
                return false;
            return name.empty() || ScriptComponentRegistry::getInstance().getSchema(name) == nullptr;
        }

        void captureEntity(flecs::entity entity, int parent, Scene& scene) {
            EntityRecord record;
            record.parent = parent;
            record.name = entity.name().c_str();
            if (const auto* value = entity.get<Position2D>()) record.position = *value;
            if (const auto* value = entity.get<Scale2D>()) record.scale = *value;
            if (const auto* value = entity.get<Rotation2D>()) record.rotation = *value;
            if (const auto* value = entity.get<Sprite2D>()) record.sprite = *value;
            if (const auto* value = entity.get<AABBCollider2D>()) record.aabb = *value;
            if (const auto* value = entity.get<CircleCollider2D>()) record.circle = *value;
            if (const auto* value = entity.get<RigidBody2D>()) record.rigidBody = *value;
            if (const auto* value = entity.get<Camera2D>()) record.camera = *value;

            const auto scriptId = entity.world().component<ScriptComponent>().id();
            entity.each(scriptId, flecs::Wildcard, [&record, &entity](flecs::id id) {
                if (const auto* value = entity.get<ScriptComponent>(id.second())) {
                    ScriptRecord script;
                    script.schema = id.second().name().c_str();
                    for (const auto& [name, field] : value->values) script.values[name] = field;
                    record.scripts.push_back(std::move(script));
                }
            });

            const int index = static_cast<int>(scene.size());
            scene.push_back(std::move(record));
            entity.children([index, &scene](flecs::entity child) {
                if (isSceneEntity(child)) captureEntity(child, index, scene);
            });
        }

        Scene captureScene(flecs::world& world) {
            Scene scene;
            auto roots = world.query_builder().without(flecs::ChildOf, flecs::Wildcard).build();
            roots.each([&scene](flecs::entity entity) {
                if (isSceneEntity(entity)) captureEntity(entity, -1, scene);
            });
            return scene;
        }

        void clearScene(flecs::world& world) {
            std::vector<flecs::entity_t> entities;
            auto roots = world.query_builder().without(flecs::ChildOf, flecs::Wildcard).build();
            roots.each([&entities](flecs::entity entity) {
                if (isSceneEntity(entity)) entities.push_back(entity.id());
            });
            auto children = world.query_builder().with(flecs::ChildOf, flecs::Wildcard).build();
            children.each([&entities](flecs::entity entity) {
                if (isSceneEntity(entity)) entities.push_back(entity.id());
            });
            for (auto it = entities.rbegin(); it != entities.rend(); ++it) {
                const auto id = *it;
                auto entity = world.entity(id);
                if (entity.is_alive()) entity.destruct();
            }
        }

        void restoreScene(flecs::world& world, const Scene& scene) {
            const bool deferred = world.is_deferred();
            if (deferred) world.defer_suspend();
            clearScene(world);

            std::vector<flecs::entity> entities;
            entities.reserve(scene.size());
            for (const auto& record : scene) {
                auto entity = world.entity();
                if (record.parent >= 0 && static_cast<std::size_t>(record.parent) < entities.size()) {
                    entity.child_of(entities[record.parent]);
                }
                if (!record.name.empty()) entity.set_name(record.name.c_str());
                if (record.position) entity.set<Position2D>(*record.position);
                if (record.scale) entity.set<Scale2D>(*record.scale);
                if (record.rotation) entity.set<Rotation2D>(*record.rotation);
                if (record.sprite) entity.set<Sprite2D>(*record.sprite);
                if (record.aabb) entity.set<AABBCollider2D>(*record.aabb);
                if (record.circle) entity.set<CircleCollider2D>(*record.circle);
                if (record.rigidBody) entity.set<RigidBody2D>(*record.rigidBody);
                if (record.camera) {
                    Camera2D camera = *record.camera;
                    camera.shakeIntensity = 0.0f;
                    camera.shakeDuration = 0.0f;
                    camera.shakeRemaining = 0.0f;
                    camera.shakeClock = 0.0f;
                    camera.shakeOffset = {};
                    camera.shakeRotation = 0.0f;
                    entity.set<Camera2D>(camera);
                }
                for (const auto& script : record.scripts) {
                    auto schema = world.entity(script.schema.c_str());
                    ScriptComponent component;
                    component.schemaName = script.schema;
                    for (const auto& [name, value] : script.values) component.values[name] = value;
                    entity.set<ScriptComponent>(schema, component);
                }
                entities.push_back(entity);
            }
            if (deferred) world.defer_resume();
        }

        std::string escapeJson(const std::string& value) {
            std::ostringstream out;
            for (unsigned char c : value) {
                switch (c) {
                case '\\':
                    out << "\\\\";
                    break;
                case '"':
                    out << "\\\"";
                    break;
                case '\n':
                    out << "\\n";
                    break;
                case '\r':
                    out << "\\r";
                    break;
                case '\t':
                    out << "\\t";
                    break;
                default:
                    if (c < 0x20)
                        out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c)
                            << std::dec;
                    else
                        out << static_cast<char>(c);
                }
            }
            return out.str();
        }

        struct Json {
            using Object = std::map<std::string, Json>;
            using Array = std::vector<Json>;
            std::variant<std::nullptr_t, bool, double, std::string, Array, Object> value;

            const Object& object() const { return std::get<Object>(value); }
            const Array& array() const { return std::get<Array>(value); }
            const std::string& string() const { return std::get<std::string>(value); }
            double number() const { return std::get<double>(value); }
            bool boolean() const { return std::get<bool>(value); }
            bool isNull() const { return std::holds_alternative<std::nullptr_t>(value); }
            const Json& at(const std::string& key) const { return object().at(key); }
        };

        class JsonParser {
        public:
            explicit JsonParser(const std::string& source) : source_(source) {}
            Json parse() {
                Json result = parseValue();
                whitespace();
                if (position_ != source_.size()) fail("unexpected trailing data");
                return result;
            }

        private:
            Json parseValue() {
                whitespace();
                if (position_ >= source_.size()) fail("unexpected end of input");
                const char c = source_[position_];
                if (c == '{') return Json{parseObject()};
                if (c == '[') return Json{parseArray()};
                if (c == '"') return Json{parseString()};
                if (c == 't') {
                    literal("true");
                    return Json{true};
                }
                if (c == 'f') {
                    literal("false");
                    return Json{false};
                }
                if (c == 'n') {
                    literal("null");
                    return Json{nullptr};
                }
                return Json{parseNumber()};
            }

            Json::Object parseObject() {
                Json::Object result;
                expect('{');
                whitespace();
                if (take('}')) return result;
                while (true) {
                    whitespace();
                    const std::string key = parseString();
                    expect(':');
                    result[key] = parseValue();
                    whitespace();
                    if (take('}')) break;
                    expect(',');
                }
                return result;
            }

            Json::Array parseArray() {
                Json::Array result;
                expect('[');
                whitespace();
                if (take(']')) return result;
                while (true) {
                    result.push_back(parseValue());
                    whitespace();
                    if (take(']')) break;
                    expect(',');
                }
                return result;
            }

            std::string parseString() {
                expect('"');
                std::string result;
                while (position_ < source_.size()) {
                    char c = source_[position_++];
                    if (c == '"') return result;
                    if (c != '\\') {
                        result += c;
                        continue;
                    }
                    if (position_ >= source_.size()) fail("invalid string escape");
                    c = source_[position_++];
                    switch (c) {
                    case '"':
                        result += '"';
                        break;
                    case '\\':
                        result += '\\';
                        break;
                    case '/':
                        result += '/';
                        break;
                    case 'b':
                        result += '\b';
                        break;
                    case 'f':
                        result += '\f';
                        break;
                    case 'n':
                        result += '\n';
                        break;
                    case 'r':
                        result += '\r';
                        break;
                    case 't':
                        result += '\t';
                        break;
                    default:
                        fail("unsupported string escape");
                    }
                }
                fail("unterminated string");
            }

            double parseNumber() {
                const std::size_t start = position_;
                while (position_ < source_.size()) {
                    const char c = source_[position_];
                    if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E')
                        ++position_;
                    else
                        break;
                }
                if (start == position_) fail("expected value");
                return std::stod(source_.substr(start, position_ - start));
            }

            void whitespace() {
                while (position_ < source_.size() && std::isspace(static_cast<unsigned char>(source_[position_])))
                    ++position_;
            }
            bool take(char expected) {
                whitespace();
                if (position_ < source_.size() && source_[position_] == expected) {
                    ++position_;
                    return true;
                }
                return false;
            }
            void expect(char expected) {
                if (!take(expected)) fail(std::string("expected '") + expected + "'");
            }
            void literal(const char* text) {
                const auto size = std::strlen(text);
                if (source_.compare(position_, size, text) != 0) fail("invalid literal");
                position_ += size;
            }
            [[noreturn]] void fail(const std::string& message) const {
                throw std::runtime_error(message + " at byte " + std::to_string(position_));
            }

            const std::string& source_;
            std::size_t position_ = 0;
        };

        void writeVector(std::ostream& out, const Vector2& value) {
            out << '[' << value.x << ',' << value.y << ']';
        }

        std::string sceneToJson(const Scene& scene) {
            std::ostringstream out;
            out << std::setprecision(std::numeric_limits<float>::max_digits10);
            out << "{\n  \"format\": \"levi.scene\",\n  \"version\": 1,\n  \"entities\": [\n";
            for (std::size_t i = 0; i < scene.size(); ++i) {
                const auto& entity = scene[i];
                out << "    {\"parent\":" << entity.parent << ",\"name\":\"" << escapeJson(entity.name)
                    << "\",\"components\":{";
                bool first = true;
                auto key = [&](const char* name) {
                    if (!first) out << ',';
                    first = false;
                    out << '"' << name << "\":";
                };
                if (entity.position) {
                    key("Position2D");
                    writeVector(out, *entity.position);
                }
                if (entity.scale) {
                    key("Scale2D");
                    writeVector(out, *entity.scale);
                }
                if (entity.rotation) {
                    key("Rotation2D");
                    out << '[' << entity.rotation->angle << ',';
                    writeVector(out, entity.rotation->pivot);
                    out << ',' << static_cast<int>(entity.rotation->pivotType) << ']';
                }
                if (entity.sprite) {
                    key("Sprite2D");
                    out << "{\"texture\":\"" << escapeJson(entity.sprite->texturePath) << "\",\"size\":";
                    writeVector(out, entity.sprite->size);
                    out << '}';
                }
                if (entity.aabb) {
                    key("AABBCollider2D");
                    out << "{\"size\":";
                    writeVector(out, entity.aabb->size);
                    out << ",\"offset\":";
                    writeVector(out, entity.aabb->offset);
                    out << '}';
                }
                if (entity.circle) {
                    key("CircleCollider2D");
                    out << "{\"radius\":" << entity.circle->radius << ",\"offset\":";
                    writeVector(out, entity.circle->offset);
                    out << '}';
                }
                if (entity.rigidBody) {
                    key("RigidBody2D");
                    out << "{\"type\":" << static_cast<int>(entity.rigidBody->type) << ",\"linearVelocity\":";
                    writeVector(out, entity.rigidBody->linearVelocity);
                    out << ",\"angularVelocity\":" << entity.rigidBody->angularVelocity
                        << ",\"gravityScale\":" << entity.rigidBody->gravityScale
                        << ",\"linearDamping\":" << entity.rigidBody->linearDamping
                        << ",\"angularDamping\":" << entity.rigidBody->angularDamping
                        << ",\"density\":" << entity.rigidBody->density
                        << ",\"friction\":" << entity.rigidBody->friction
                        << ",\"restitution\":" << entity.rigidBody->restitution
                        << ",\"fixedRotation\":" << (entity.rigidBody->fixedRotation ? "true" : "false")
                        << ",\"bullet\":" << (entity.rigidBody->bullet ? "true" : "false")
                        << ",\"enabled\":" << (entity.rigidBody->enabled ? "true" : "false")
                        << ",\"sensor\":" << (entity.rigidBody->sensor ? "true" : "false")
                        << ",\"categoryBits\":" << entity.rigidBody->categoryBits
                        << ",\"maskBits\":" << entity.rigidBody->maskBits << '}';
                }
                if (entity.camera) {
                    key("Camera2D");
                    out << "{\"zoom\":" << entity.camera->zoom
                        << ",\"active\":" << (entity.camera->active ? "true" : "false")
                        << ",\"maxShakeOffset\":" << entity.camera->maxShakeOffset
                        << ",\"maxShakeRotation\":" << entity.camera->maxShakeRotation << '}';
                }
                out << "},\"scripts\":[";
                for (std::size_t s = 0; s < entity.scripts.size(); ++s) {
                    const auto& script = entity.scripts[s];
                    if (s) out << ',';
                    out << "{\"schema\":\"" << escapeJson(script.schema) << "\",\"values\":{";
                    bool firstValue = true;
                    for (const auto& [name, value] : script.values) {
                        if (!firstValue) out << ',';
                        firstValue = false;
                        out << '"' << escapeJson(name) << "\":{";
                        std::visit(
                            [&out](const auto& item) {
                                using T = std::decay_t<decltype(item)>;
                                if constexpr (std::is_same_v<T, float>)
                                    out << "\"type\":\"float\",\"value\":" << item;
                                else if constexpr (std::is_same_v<T, int>)
                                    out << "\"type\":\"int\",\"value\":" << item;
                                else if constexpr (std::is_same_v<T, bool>)
                                    out << "\"type\":\"bool\",\"value\":" << (item ? "true" : "false");
                                else
                                    out << "\"type\":\"string\",\"value\":\"" << escapeJson(item) << '"';
                            },
                            value);
                        out << '}';
                    }
                    out << "}}";
                }
                out << "]}" << (i + 1 < scene.size() ? "," : "") << '\n';
            }
            out << "  ]\n}\n";
            return out.str();
        }

        Vector2 readVector(const Json& json) {
            const auto& values = json.array();
            if (values.size() != 2) throw std::runtime_error("expected Vector2 array");
            return {static_cast<float>(values[0].number()), static_cast<float>(values[1].number())};
        }

        Scene sceneFromJson(const std::string& source) {
            const Json root = JsonParser(source).parse();
            if (root.at("format").string() != "levi.scene" || static_cast<int>(root.at("version").number()) != 1) {
                throw std::runtime_error("unsupported scene format or version");
            }
            Scene scene;
            for (const auto& entityJson : root.at("entities").array()) {
                EntityRecord entity;
                entity.parent = static_cast<int>(entityJson.at("parent").number());
                entity.name = entityJson.at("name").string();
                const auto& components = entityJson.at("components").object();
                if (auto it = components.find("Position2D"); it != components.end()) {
                    auto v = readVector(it->second);
                    entity.position = Position2D{v.x, v.y};
                }
                if (auto it = components.find("Scale2D"); it != components.end()) {
                    auto v = readVector(it->second);
                    entity.scale = Scale2D{v.x, v.y};
                }
                if (auto it = components.find("Rotation2D"); it != components.end()) {
                    const auto& a = it->second.array();
                    entity.rotation = Rotation2D{static_cast<float>(a.at(0).number()), readVector(a.at(1)),
                                                 static_cast<PivotType>(static_cast<int>(a.at(2).number()))};
                }
                if (auto it = components.find("Sprite2D"); it != components.end()) {
                    entity.sprite = Sprite2D{it->second.at("texture").string(), readVector(it->second.at("size"))};
                }
                if (auto it = components.find("AABBCollider2D"); it != components.end()) {
                    entity.aabb =
                        AABBCollider2D{readVector(it->second.at("size")), readVector(it->second.at("offset"))};
                }
                if (auto it = components.find("CircleCollider2D"); it != components.end()) {
                    entity.circle = CircleCollider2D{static_cast<float>(it->second.at("radius").number()),
                                                     readVector(it->second.at("offset"))};
                }
                if (auto it = components.find("RigidBody2D"); it != components.end()) {
                    RigidBody2D body;
                    body.type = static_cast<RigidBodyType2D>(static_cast<int>(it->second.at("type").number()));
                    body.linearVelocity = readVector(it->second.at("linearVelocity"));
                    body.angularVelocity = static_cast<float>(it->second.at("angularVelocity").number());
                    body.gravityScale = static_cast<float>(it->second.at("gravityScale").number());
                    body.linearDamping = static_cast<float>(it->second.at("linearDamping").number());
                    body.angularDamping = static_cast<float>(it->second.at("angularDamping").number());
                    body.density = static_cast<float>(it->second.at("density").number());
                    body.friction = static_cast<float>(it->second.at("friction").number());
                    body.restitution = static_cast<float>(it->second.at("restitution").number());
                    body.fixedRotation = it->second.at("fixedRotation").boolean();
                    body.bullet = it->second.at("bullet").boolean();
                    body.enabled = it->second.at("enabled").boolean();
                    body.sensor = it->second.at("sensor").boolean();
                    body.categoryBits = static_cast<std::uint32_t>(it->second.at("categoryBits").number());
                    body.maskBits = static_cast<std::uint32_t>(it->second.at("maskBits").number());
                    entity.rigidBody = body;
                }
                if (auto it = components.find("Camera2D"); it != components.end()) {
                    Camera2D camera;
                    camera.zoom = static_cast<float>(it->second.at("zoom").number());
                    camera.active = it->second.at("active").boolean();
                    camera.maxShakeOffset = static_cast<float>(it->second.at("maxShakeOffset").number());
                    camera.maxShakeRotation = static_cast<float>(it->second.at("maxShakeRotation").number());
                    entity.camera = camera;
                }
                for (const auto& scriptJson : entityJson.at("scripts").array()) {
                    ScriptRecord script;
                    script.schema = scriptJson.at("schema").string();
                    for (const auto& [name, field] : scriptJson.at("values").object()) {
                        const std::string type = field.at("type").string();
                        if (type == "float")
                            script.values[name] = static_cast<float>(field.at("value").number());
                        else if (type == "int")
                            script.values[name] = static_cast<int>(field.at("value").number());
                        else if (type == "bool")
                            script.values[name] = field.at("value").boolean();
                        else if (type == "string")
                            script.values[name] = field.at("value").string();
                        else
                            throw std::runtime_error("unsupported script value type: " + type);
                    }
                    entity.scripts.push_back(std::move(script));
                }
                scene.push_back(std::move(entity));
            }
            return scene;
        }

        template <typename T> void binaryWrite(std::vector<unsigned char>& out, const T& value) {
            const auto* bytes = reinterpret_cast<const unsigned char*>(&value);
            out.insert(out.end(), bytes, bytes + sizeof(T));
        }

        void binaryWriteString(std::vector<unsigned char>& out, const std::string& value) {
            const auto size = static_cast<std::uint32_t>(value.size());
            binaryWrite(out, size);
            out.insert(out.end(), value.begin(), value.end());
        }

        class BinaryReader {
        public:
            explicit BinaryReader(const std::vector<unsigned char>& data) : data_(data) {}
            template <typename T> T read() {
                if (position_ + sizeof(T) > data_.size()) throw std::runtime_error("truncated binary scene");
                T result;
                std::memcpy(&result, data_.data() + position_, sizeof(T));
                position_ += sizeof(T);
                return result;
            }
            std::string string() {
                const auto size = read<std::uint32_t>();
                if (position_ + size > data_.size()) throw std::runtime_error("truncated binary string");
                std::string result(reinterpret_cast<const char*>(data_.data() + position_), size);
                position_ += size;
                return result;
            }

        private:
            const std::vector<unsigned char>& data_;
            std::size_t position_ = 0;
        };

        std::vector<unsigned char> sceneToBinary(const Scene& scene) {
            std::vector<unsigned char> out{'L', 'E', 'V', 'I', 'S', 'C', 'N', '1'};
            binaryWrite(out, static_cast<std::uint32_t>(scene.size()));
            for (const auto& entity : scene) {
                binaryWrite(out, static_cast<std::int32_t>(entity.parent));
                binaryWriteString(out, entity.name);
                std::uint8_t mask = (entity.position ? 1 : 0) | (entity.scale ? 2 : 0) | (entity.rotation ? 4 : 0) |
                                    (entity.sprite ? 8 : 0) | (entity.aabb ? 16 : 0) | (entity.circle ? 32 : 0) |
                                    (entity.camera ? 64 : 0) | (entity.rigidBody ? 128 : 0);
                binaryWrite(out, mask);
                if (entity.position) binaryWrite(out, *entity.position);
                if (entity.scale) binaryWrite(out, *entity.scale);
                if (entity.rotation) binaryWrite(out, *entity.rotation);
                if (entity.sprite) {
                    binaryWriteString(out, entity.sprite->texturePath);
                    binaryWrite(out, entity.sprite->size);
                }
                if (entity.aabb) binaryWrite(out, *entity.aabb);
                if (entity.circle) binaryWrite(out, *entity.circle);
                if (entity.camera) {
                    binaryWrite(out, entity.camera->zoom);
                    binaryWrite(out, static_cast<std::uint8_t>(entity.camera->active ? 1 : 0));
                    binaryWrite(out, entity.camera->maxShakeOffset);
                    binaryWrite(out, entity.camera->maxShakeRotation);
                }
                if (entity.rigidBody) {
                    binaryWrite(out, static_cast<std::uint8_t>(entity.rigidBody->type));
                    binaryWrite(out, entity.rigidBody->linearVelocity);
                    binaryWrite(out, entity.rigidBody->angularVelocity);
                    binaryWrite(out, entity.rigidBody->gravityScale);
                    binaryWrite(out, entity.rigidBody->linearDamping);
                    binaryWrite(out, entity.rigidBody->angularDamping);
                    binaryWrite(out, entity.rigidBody->density);
                    binaryWrite(out, entity.rigidBody->friction);
                    binaryWrite(out, entity.rigidBody->restitution);
                    binaryWrite(out, static_cast<std::uint8_t>(entity.rigidBody->fixedRotation));
                    binaryWrite(out, static_cast<std::uint8_t>(entity.rigidBody->bullet));
                    binaryWrite(out, static_cast<std::uint8_t>(entity.rigidBody->enabled));
                    binaryWrite(out, static_cast<std::uint8_t>(entity.rigidBody->sensor));
                    binaryWrite(out, entity.rigidBody->categoryBits);
                    binaryWrite(out, entity.rigidBody->maskBits);
                }
                binaryWrite(out, static_cast<std::uint32_t>(entity.scripts.size()));
                for (const auto& script : entity.scripts) {
                    binaryWriteString(out, script.schema);
                    binaryWrite(out, static_cast<std::uint32_t>(script.values.size()));
                    for (const auto& [name, value] : script.values) {
                        binaryWriteString(out, name);
                        binaryWrite(out, static_cast<std::uint8_t>(value.index()));
                        std::visit(
                            [&out](const auto& item) {
                                using T = std::decay_t<decltype(item)>;
                                if constexpr (std::is_same_v<T, std::string>)
                                    binaryWriteString(out, item);
                                else
                                    binaryWrite(out, item);
                            },
                            value);
                    }
                }
            }
            return out;
        }

        Scene sceneFromBinary(const std::vector<unsigned char>& data) {
            if (data.size() < 8 || std::memcmp(data.data(), "LEVISCN1", 8) != 0)
                throw std::runtime_error("invalid binary scene header");
            std::vector<unsigned char> payload(data.begin() + 8, data.end());
            BinaryReader reader(payload);
            Scene scene;
            const auto count = reader.read<std::uint32_t>();
            scene.reserve(count);
            for (std::uint32_t i = 0; i < count; ++i) {
                EntityRecord entity;
                entity.parent = reader.read<std::int32_t>();
                entity.name = reader.string();
                const auto mask = reader.read<std::uint8_t>();
                if (mask & 1) entity.position = reader.read<Position2D>();
                if (mask & 2) entity.scale = reader.read<Scale2D>();
                if (mask & 4) entity.rotation = reader.read<Rotation2D>();
                if (mask & 8) entity.sprite = Sprite2D{reader.string(), reader.read<Vector2>()};
                if (mask & 16) entity.aabb = reader.read<AABBCollider2D>();
                if (mask & 32) entity.circle = reader.read<CircleCollider2D>();
                if (mask & 64) {
                    Camera2D camera;
                    camera.zoom = reader.read<float>();
                    camera.active = reader.read<std::uint8_t>() != 0;
                    camera.maxShakeOffset = reader.read<float>();
                    camera.maxShakeRotation = reader.read<float>();
                    entity.camera = camera;
                }
                if (mask & 128) {
                    RigidBody2D body;
                    body.type = static_cast<RigidBodyType2D>(reader.read<std::uint8_t>());
                    body.linearVelocity = reader.read<Vector2>();
                    body.angularVelocity = reader.read<float>();
                    body.gravityScale = reader.read<float>();
                    body.linearDamping = reader.read<float>();
                    body.angularDamping = reader.read<float>();
                    body.density = reader.read<float>();
                    body.friction = reader.read<float>();
                    body.restitution = reader.read<float>();
                    body.fixedRotation = reader.read<std::uint8_t>() != 0;
                    body.bullet = reader.read<std::uint8_t>() != 0;
                    body.enabled = reader.read<std::uint8_t>() != 0;
                    body.sensor = reader.read<std::uint8_t>() != 0;
                    body.categoryBits = reader.read<std::uint32_t>();
                    body.maskBits = reader.read<std::uint32_t>();
                    entity.rigidBody = body;
                }
                const auto scriptCount = reader.read<std::uint32_t>();
                for (std::uint32_t s = 0; s < scriptCount; ++s) {
                    ScriptRecord script;
                    script.schema = reader.string();
                    const auto valueCount = reader.read<std::uint32_t>();
                    for (std::uint32_t v = 0; v < valueCount; ++v) {
                        const std::string name = reader.string();
                        switch (reader.read<std::uint8_t>()) {
                        case 0:
                            script.values[name] = reader.read<float>();
                            break;
                        case 1:
                            script.values[name] = reader.read<int>();
                            break;
                        case 2:
                            script.values[name] = reader.string();
                            break;
                        case 3:
                            script.values[name] = reader.read<bool>();
                            break;
                        default:
                            throw std::runtime_error("invalid binary script value");
                        }
                    }
                    entity.scripts.push_back(std::move(script));
                }
                scene.push_back(std::move(entity));
            }
            return scene;
        }

        bool setError(std::string* error, const std::string& value) {
            if (error) *error = value;
            return false;
        }
    } // namespace

    std::string SceneSerializer::toJson(flecs::world& world) {
        return sceneToJson(captureScene(world));
    }

    bool SceneSerializer::fromJson(flecs::world& world, const std::string& json, std::string* error) {
        try {
            restoreScene(world, sceneFromJson(json));
            return true;
        } catch (const std::exception& exception) {
            return setError(error, exception.what());
        }
    }

    bool SceneSerializer::saveJson(flecs::world& world, const std::filesystem::path& path, std::string* error) {
        try {
            if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
            std::ofstream file(path, std::ios::binary);
            if (!file) return setError(error, "cannot open scene for writing: " + path.string());
            file << toJson(world);
            return file.good() || setError(error, "failed writing scene: " + path.string());
        } catch (const std::exception& exception) {
            return setError(error, exception.what());
        }
    }

    bool SceneSerializer::loadJson(flecs::world& world, const std::filesystem::path& path, std::string* error) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return setError(error, "cannot open scene: " + path.string());
        std::ostringstream source;
        source << file.rdbuf();
        return fromJson(world, source.str(), error);
    }

    std::vector<unsigned char> SceneSerializer::toBinary(flecs::world& world) {
        return sceneToBinary(captureScene(world));
    }

    bool SceneSerializer::fromBinary(flecs::world& world, const std::vector<unsigned char>& data, std::string* error) {
        try {
            restoreScene(world, sceneFromBinary(data));
            return true;
        } catch (const std::exception& exception) {
            return setError(error, exception.what());
        }
    }

    bool SceneSerializer::saveBinary(flecs::world& world, const std::filesystem::path& path, std::string* error) {
        try {
            if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
            const auto data = toBinary(world);
            std::ofstream file(path, std::ios::binary);
            if (!file) return setError(error, "cannot open binary scene for writing: " + path.string());
            file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
            return file.good() || setError(error, "failed writing binary scene: " + path.string());
        } catch (const std::exception& exception) {
            return setError(error, exception.what());
        }
    }

    bool SceneSerializer::loadBinary(flecs::world& world, const std::filesystem::path& path, std::string* error) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return setError(error, "cannot open binary scene: " + path.string());
        std::vector<unsigned char> data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return fromBinary(world, data, error);
    }

} // namespace Levi
