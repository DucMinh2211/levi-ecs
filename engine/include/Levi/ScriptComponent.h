#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <variant>

namespace Levi {

    enum class ScriptFieldType {
        Float,
        Int,
        String,
        Bool,
        AssetPath,
        Unknown
    };

    struct ScriptField {
        std::string name;
        ScriptFieldType type;
    };

    struct ScriptComponentSchema {
        std::string name;
        std::vector<ScriptField> fields;
    };

    using ScriptValue = std::variant<float, int, std::string, bool>;

    // This is the component that actually sits on the Flecs/EnTT entity
    struct ScriptComponent {
        std::string schemaName;
        std::unordered_map<std::string, ScriptValue> values;
    };

    // Marks Flecs entities that only serve as pair targets for dynamic Lua
    // component schemas. These are editor metadata, not scene entities.
    struct ScriptComponentSchemaTag {};

    class ScriptComponentRegistry {
    public:
        static ScriptComponentRegistry& getInstance() {
            static ScriptComponentRegistry instance;
            return instance;
        }

        void registerSchema(const ScriptComponentSchema& schema) {
            schemas_[schema.name] = schema;
        }

        const ScriptComponentSchema* getSchema(const std::string& name) const {
            auto it = schemas_.find(name);
            if (it != schemas_.end()) return &it->second;
            return nullptr;
        }

        const std::unordered_map<std::string, ScriptComponentSchema>& getSchemas() const {
            return schemas_;
        }

        void clear() {
            schemas_.clear();
        }

    private:
        ScriptComponentRegistry() = default;
        std::unordered_map<std::string, ScriptComponentSchema> schemas_;
    };

}
