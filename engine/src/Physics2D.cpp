#include "Levi/Physics2D.h"

#include <box2d/box2d.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Levi {
    namespace {

        constexpr float Pi = 3.14159265358979323846f;
        constexpr float DegreesToRadians = Pi / 180.0f;
        constexpr float RadiansToDegrees = 180.0f / Pi;
        constexpr float FixedTimeStep = 1.0f / 60.0f;
        constexpr int SolverSubSteps = 4;

        struct ContactPair {
            flecs::entity_t first = 0;
            flecs::entity_t second = 0;

            static ContactPair make(flecs::entity_t a, flecs::entity_t b) {
                return a < b ? ContactPair{a, b} : ContactPair{b, a};
            }

            bool operator==(const ContactPair&) const = default;
        };

        struct ContactPairHash {
            std::size_t operator()(const ContactPair& pair) const noexcept {
                const auto first = std::hash<flecs::entity_t>{}(pair.first);
                const auto second = std::hash<flecs::entity_t>{}(pair.second);
                return first ^ (second + 0x9e3779b9u + (first << 6u) + (first >> 2u));
            }
        };

        std::size_t hashCombine(std::size_t seed, std::uint64_t value) {
            return seed ^ (std::hash<std::uint64_t>{}(value) + 0x9e3779b9u + (seed << 6u) + (seed >> 2u));
        }

        std::size_t hashFloat(std::size_t seed, float value) {
            std::uint32_t bits = 0;
            static_assert(sizeof(bits) == sizeof(value));
            std::memcpy(&bits, &value, sizeof(value));
            return hashCombine(seed, bits);
        }

        std::size_t bodySignature(flecs::entity entity, const RigidBody2D& body) {
            std::size_t result = hashCombine(0, static_cast<std::uint64_t>(body.type));
            result = hashFloat(result, body.gravityScale);
            result = hashFloat(result, body.linearDamping);
            result = hashFloat(result, body.angularDamping);
            result = hashFloat(result, body.density);
            result = hashFloat(result, body.friction);
            result = hashFloat(result, body.restitution);
            result = hashCombine(result, body.fixedRotation);
            result = hashCombine(result, body.bullet);
            result = hashCombine(result, body.enabled);
            result = hashCombine(result, body.sensor);
            result = hashCombine(result, body.categoryBits);
            result = hashCombine(result, body.maskBits);

            if (const auto* box = entity.get<AABBCollider2D>()) {
                result = hashCombine(result, 0xAABB);
                result = hashFloat(result, box->size.x);
                result = hashFloat(result, box->size.y);
                result = hashFloat(result, box->offset.x);
                result = hashFloat(result, box->offset.y);
            }
            if (const auto* circle = entity.get<CircleCollider2D>()) {
                result = hashCombine(result, 0xC1CC1E);
                result = hashFloat(result, circle->radius);
                result = hashFloat(result, circle->offset.x);
                result = hashFloat(result, circle->offset.y);
            }
            return result;
        }

        struct BodyRecord {
            b2BodyId bodyId = b2_nullBodyId;
            std::vector<b2ShapeId> shapeIds;
            std::size_t signature = 0;
            Vector2 lastPosition;
            float lastAngle = 0.0f;
            Vector2 lastLinearVelocity;
            float lastAngularVelocity = 0.0f;
        };

        struct PhysicsRuntime {
            b2WorldId worldId = b2_nullWorldId;
            Vector2 gravity = {0.0f, 980.0f};
            float accumulator = 0.0f;
            std::unordered_map<flecs::entity_t, BodyRecord> bodies;
            std::unordered_map<std::uint64_t, flecs::entity_t> shapeOwners;
            std::unordered_map<ContactPair, int, ContactPairHash> contacts;
            std::unordered_set<ContactPair, ContactPairHash> beganContacts;
            std::unordered_set<ContactPair, ContactPairHash> endedContacts;

            PhysicsRuntime() { createWorld(); }
            ~PhysicsRuntime() { destroyWorld(); }

            void createWorld() {
                b2WorldDef definition = b2DefaultWorldDef();
                definition.gravity = {gravity.x / Physics2D::DefaultPixelsPerMeter,
                                      gravity.y / Physics2D::DefaultPixelsPerMeter};
                worldId = b2CreateWorld(&definition);
            }

            void destroyWorld() {
                if (b2World_IsValid(worldId)) b2DestroyWorld(worldId);
                worldId = b2_nullWorldId;
                bodies.clear();
                shapeOwners.clear();
                contacts.clear();
                beganContacts.clear();
                endedContacts.clear();
                accumulator = 0.0f;
            }
        };

        struct PhysicsRuntimeHolder {
            std::shared_ptr<PhysicsRuntime> value;
        };

        PhysicsRuntime& runtime(flecs::world& world) {
            auto* holder = world.get_mut<PhysicsRuntimeHolder>();
            if (!holder) {
                world.set<PhysicsRuntimeHolder>({std::make_shared<PhysicsRuntime>()});
                holder = world.get_mut<PhysicsRuntimeHolder>();
            }
            if (!holder->value) holder->value = std::make_shared<PhysicsRuntime>();
            return *holder->value;
        }

        b2BodyType toBox2DType(RigidBodyType2D type) {
            switch (type) {
            case RigidBodyType2D::Static:
                return b2_staticBody;
            case RigidBodyType2D::Kinematic:
                return b2_kinematicBody;
            case RigidBodyType2D::Dynamic:
                return b2_dynamicBody;
            }
            return b2_dynamicBody;
        }

        bool different(float first, float second) {
            return std::abs(first - second) > 0.0001f;
        }

        bool different(const Vector2& first, const Vector2& second) {
            return different(first.x, second.x) || different(first.y, second.y);
        }

        void removeContactsFor(PhysicsRuntime& state, flecs::entity_t entityId) {
            for (auto it = state.contacts.begin(); it != state.contacts.end();) {
                if (it->first.first == entityId || it->first.second == entityId) {
                    state.endedContacts.insert(it->first);
                    it = state.contacts.erase(it);
                } else {
                    ++it;
                }
            }
        }

        void destroyBody(PhysicsRuntime& state, flecs::entity_t entityId, BodyRecord& record) {
            removeContactsFor(state, entityId);
            for (const b2ShapeId shapeId : record.shapeIds) {
                state.shapeOwners.erase(b2StoreShapeId(shapeId));
            }
            if (b2Body_IsValid(record.bodyId)) b2DestroyBody(record.bodyId);
        }

        BodyRecord* createBody(PhysicsRuntime& state, flecs::entity entity) {
            const auto* position = entity.get<Position2D>();
            const auto* rigidBody = entity.get<RigidBody2D>();
            const auto* box = entity.get<AABBCollider2D>();
            const auto* circle = entity.get<CircleCollider2D>();
            if (!position || !rigidBody || (!box && !circle)) return nullptr;

            const auto old = state.bodies.find(entity.id());
            if (old != state.bodies.end()) {
                destroyBody(state, entity.id(), old->second);
                state.bodies.erase(old);
            }

            const float angle = entity.has<Rotation2D>() ? entity.get<Rotation2D>()->angle : 0.0f;
            b2BodyDef bodyDefinition = b2DefaultBodyDef();
            bodyDefinition.type = toBox2DType(rigidBody->type);
            bodyDefinition.position = {position->x / Physics2D::DefaultPixelsPerMeter,
                                       position->y / Physics2D::DefaultPixelsPerMeter};
            bodyDefinition.rotation = b2MakeRot(angle * DegreesToRadians);
            bodyDefinition.linearVelocity = {rigidBody->linearVelocity.x / Physics2D::DefaultPixelsPerMeter,
                                             rigidBody->linearVelocity.y / Physics2D::DefaultPixelsPerMeter};
            bodyDefinition.angularVelocity = rigidBody->angularVelocity * DegreesToRadians;
            bodyDefinition.gravityScale = rigidBody->gravityScale;
            bodyDefinition.linearDamping = std::max(0.0f, rigidBody->linearDamping);
            bodyDefinition.angularDamping = std::max(0.0f, rigidBody->angularDamping);
            bodyDefinition.fixedRotation = rigidBody->fixedRotation;
            bodyDefinition.isBullet = rigidBody->bullet;
            bodyDefinition.isEnabled = rigidBody->enabled;
            bodyDefinition.userData = reinterpret_cast<void*>(static_cast<std::uintptr_t>(entity.id()));

            BodyRecord record;
            record.bodyId = b2CreateBody(state.worldId, &bodyDefinition);
            record.signature = bodySignature(entity, *rigidBody);
            record.lastPosition = *position;
            record.lastAngle = angle;
            record.lastLinearVelocity = rigidBody->linearVelocity;
            record.lastAngularVelocity = rigidBody->angularVelocity;

            b2ShapeDef shapeDefinition = b2DefaultShapeDef();
            shapeDefinition.density = std::max(0.0f, rigidBody->density);
            shapeDefinition.material.friction = std::clamp(rigidBody->friction, 0.0f, 1.0f);
            shapeDefinition.material.restitution = std::clamp(rigidBody->restitution, 0.0f, 1.0f);
            shapeDefinition.isSensor = rigidBody->sensor;
            shapeDefinition.enableSensorEvents = true;
            shapeDefinition.enableContactEvents = true;
            shapeDefinition.filter.categoryBits = rigidBody->categoryBits;
            shapeDefinition.filter.maskBits = rigidBody->maskBits;

            if (box) {
                const float halfWidth =
                    std::max(std::abs(box->size.x) * 0.5f / Physics2D::DefaultPixelsPerMeter, 0.001f);
                const float halfHeight =
                    std::max(std::abs(box->size.y) * 0.5f / Physics2D::DefaultPixelsPerMeter, 0.001f);
                const b2Vec2 center = {box->offset.x / Physics2D::DefaultPixelsPerMeter,
                                       box->offset.y / Physics2D::DefaultPixelsPerMeter};
                const b2Polygon polygon = b2MakeOffsetBox(halfWidth, halfHeight, center, b2Rot_identity);
                record.shapeIds.push_back(b2CreatePolygonShape(record.bodyId, &shapeDefinition, &polygon));
            }
            if (circle) {
                b2Circle shape;
                shape.center = {circle->offset.x / Physics2D::DefaultPixelsPerMeter,
                                circle->offset.y / Physics2D::DefaultPixelsPerMeter};
                shape.radius = std::max(std::abs(circle->radius) / Physics2D::DefaultPixelsPerMeter, 0.001f);
                record.shapeIds.push_back(b2CreateCircleShape(record.bodyId, &shapeDefinition, &shape));
            }

            auto [inserted, _] = state.bodies.emplace(entity.id(), std::move(record));
            for (const b2ShapeId shapeId : inserted->second.shapeIds) {
                state.shapeOwners[b2StoreShapeId(shapeId)] = entity.id();
            }
            return &inserted->second;
        }

        BodyRecord* ensureBody(PhysicsRuntime& state, flecs::entity entity) {
            if (!entity.is_alive() || !entity.has<Position2D>() || !entity.has<RigidBody2D>() ||
                (!entity.has<AABBCollider2D>() && !entity.has<CircleCollider2D>()))
                return nullptr;
            auto found = state.bodies.find(entity.id());
            const auto* body = entity.get<RigidBody2D>();
            if (found == state.bodies.end() || found->second.signature != bodySignature(entity, *body) ||
                !b2Body_IsValid(found->second.bodyId)) {
                return createBody(state, entity);
            }
            return &found->second;
        }

        void recordBegin(PhysicsRuntime& state, b2ShapeId firstShape, b2ShapeId secondShape) {
            const auto first = state.shapeOwners.find(b2StoreShapeId(firstShape));
            const auto second = state.shapeOwners.find(b2StoreShapeId(secondShape));
            if (first == state.shapeOwners.end() || second == state.shapeOwners.end() ||
                first->second == second->second)
                return;
            const ContactPair pair = ContactPair::make(first->second, second->second);
            int& count = state.contacts[pair];
            if (count == 0) state.beganContacts.insert(pair);
            ++count;
        }

        void recordEnd(PhysicsRuntime& state, b2ShapeId firstShape, b2ShapeId secondShape) {
            const auto first = state.shapeOwners.find(b2StoreShapeId(firstShape));
            const auto second = state.shapeOwners.find(b2StoreShapeId(secondShape));
            if (first == state.shapeOwners.end() || second == state.shapeOwners.end() ||
                first->second == second->second)
                return;
            const ContactPair pair = ContactPair::make(first->second, second->second);
            const auto contact = state.contacts.find(pair);
            if (contact == state.contacts.end()) return;
            if (contact->second > 1) {
                --contact->second;
            } else {
                state.contacts.erase(contact);
                state.endedContacts.insert(pair);
            }
        }

        bool containsPair(const std::unordered_set<ContactPair, ContactPairHash>& contacts, flecs::entity_t first,
                          flecs::entity_t second) {
            return contacts.contains(ContactPair::make(first, second));
        }

        bool containsPair(const std::unordered_map<ContactPair, int, ContactPairHash>& contacts, flecs::entity_t first,
                          flecs::entity_t second) {
            return contacts.contains(ContactPair::make(first, second));
        }

    } // namespace

    void Physics2D::initialize(flecs::world& world) {
        (void)runtime(world);
    }

    void Physics2D::reset(flecs::world& world) {
        auto* holder = world.get_mut<PhysicsRuntimeHolder>();
        if (!holder) {
            initialize(world);
            return;
        }
        holder->value = std::make_shared<PhysicsRuntime>();
    }

    void Physics2D::step(flecs::world& world, float deltaTime) {
        PhysicsRuntime& state = runtime(world);
        state.beganContacts.clear();
        state.endedContacts.clear();

        for (auto it = state.bodies.begin(); it != state.bodies.end();) {
            auto entity = world.entity(it->first);
            if (!entity.is_alive() || !entity.has<Position2D>() || !entity.has<RigidBody2D>() ||
                (!entity.has<AABBCollider2D>() && !entity.has<CircleCollider2D>())) {
                destroyBody(state, it->first, it->second);
                it = state.bodies.erase(it);
            } else {
                ++it;
            }
        }

        auto bodies = world.query<Position2D, RigidBody2D>();
        bodies.each([&state](flecs::entity entity, Position2D& position, RigidBody2D& rigidBody) {
            BodyRecord* record = ensureBody(state, entity);
            if (!record) return;

            const float angle = entity.has<Rotation2D>() ? entity.get<Rotation2D>()->angle : record->lastAngle;
            if (different(position, record->lastPosition) || different(angle, record->lastAngle)) {
                b2Body_SetTransform(record->bodyId,
                                    {position.x / DefaultPixelsPerMeter, position.y / DefaultPixelsPerMeter},
                                    b2MakeRot(angle * DegreesToRadians));
                record->lastPosition = position;
                record->lastAngle = angle;
            }
            if (different(rigidBody.linearVelocity, record->lastLinearVelocity)) {
                b2Body_SetLinearVelocity(record->bodyId, {rigidBody.linearVelocity.x / DefaultPixelsPerMeter,
                                                          rigidBody.linearVelocity.y / DefaultPixelsPerMeter});
                record->lastLinearVelocity = rigidBody.linearVelocity;
            }
            if (different(rigidBody.angularVelocity, record->lastAngularVelocity)) {
                b2Body_SetAngularVelocity(record->bodyId, rigidBody.angularVelocity * DegreesToRadians);
                record->lastAngularVelocity = rigidBody.angularVelocity;
            }
        });

        state.accumulator += std::clamp(deltaTime, 0.0f, 0.25f);
        while (state.accumulator >= FixedTimeStep) {
            b2World_Step(state.worldId, FixedTimeStep, SolverSubSteps);
            state.accumulator -= FixedTimeStep;

            const b2ContactEvents contacts = b2World_GetContactEvents(state.worldId);
            for (int i = 0; i < contacts.beginCount; ++i) {
                recordBegin(state, contacts.beginEvents[i].shapeIdA, contacts.beginEvents[i].shapeIdB);
            }
            for (int i = 0; i < contacts.endCount; ++i) {
                recordEnd(state, contacts.endEvents[i].shapeIdA, contacts.endEvents[i].shapeIdB);
            }
            const b2SensorEvents sensors = b2World_GetSensorEvents(state.worldId);
            for (int i = 0; i < sensors.beginCount; ++i) {
                recordBegin(state, sensors.beginEvents[i].sensorShapeId, sensors.beginEvents[i].visitorShapeId);
            }
            for (int i = 0; i < sensors.endCount; ++i) {
                recordEnd(state, sensors.endEvents[i].sensorShapeId, sensors.endEvents[i].visitorShapeId);
            }
        }

        bodies.each([&state](flecs::entity entity, Position2D& position, RigidBody2D& rigidBody) {
            const auto found = state.bodies.find(entity.id());
            if (found == state.bodies.end() || !b2Body_IsValid(found->second.bodyId)) return;
            BodyRecord& record = found->second;
            const b2Vec2 bodyPosition = b2Body_GetPosition(record.bodyId);
            const float angle = b2Rot_GetAngle(b2Body_GetRotation(record.bodyId)) * RadiansToDegrees;
            const b2Vec2 velocity = b2Body_GetLinearVelocity(record.bodyId);

            position.x = bodyPosition.x * DefaultPixelsPerMeter;
            position.y = bodyPosition.y * DefaultPixelsPerMeter;
            if (auto* rotation = entity.get_mut<Rotation2D>()) rotation->angle = angle;
            rigidBody.linearVelocity = {velocity.x * DefaultPixelsPerMeter, velocity.y * DefaultPixelsPerMeter};
            rigidBody.angularVelocity = b2Body_GetAngularVelocity(record.bodyId) * RadiansToDegrees;
            record.lastPosition = position;
            record.lastAngle = angle;
            record.lastLinearVelocity = rigidBody.linearVelocity;
            record.lastAngularVelocity = rigidBody.angularVelocity;
        });
    }

    void Physics2D::setGravity(flecs::world& world, float x, float y) {
        PhysicsRuntime& state = runtime(world);
        state.gravity = {x, y};
        b2World_SetGravity(state.worldId, {x / DefaultPixelsPerMeter, y / DefaultPixelsPerMeter});
    }

    Vector2 Physics2D::getGravity(flecs::world& world) {
        return runtime(world).gravity;
    }

    bool Physics2D::setLinearVelocity(flecs::world& world, flecs::entity_t entityId, float x, float y) {
        auto entity = world.entity(entityId);
        auto* body = entity.is_alive() ? entity.get_mut<RigidBody2D>() : nullptr;
        if (!body) return false;
        body->linearVelocity = {x, y};
        PhysicsRuntime& state = runtime(world);
        if (BodyRecord* record = ensureBody(state, entity)) {
            b2Body_SetLinearVelocity(record->bodyId, {x / DefaultPixelsPerMeter, y / DefaultPixelsPerMeter});
            record->lastLinearVelocity = body->linearVelocity;
        }
        return true;
    }

    Vector2 Physics2D::getLinearVelocity(flecs::world& world, flecs::entity_t entityId) {
        auto entity = world.entity(entityId);
        if (entity.is_alive()) {
            if (const auto* body = entity.get<RigidBody2D>()) return body->linearVelocity;
        }
        return {};
    }

    bool Physics2D::applyForce(flecs::world& world, flecs::entity_t entityId, float x, float y) {
        PhysicsRuntime& state = runtime(world);
        if (BodyRecord* record = ensureBody(state, world.entity(entityId))) {
            b2Body_ApplyForceToCenter(record->bodyId, {x, y}, true);
            return true;
        }
        return false;
    }

    bool Physics2D::applyImpulse(flecs::world& world, flecs::entity_t entityId, float x, float y) {
        PhysicsRuntime& state = runtime(world);
        if (BodyRecord* record = ensureBody(state, world.entity(entityId))) {
            b2Body_ApplyLinearImpulseToCenter(record->bodyId, {x, y}, true);
            return true;
        }
        return false;
    }

    bool Physics2D::isTouching(flecs::world& world, flecs::entity_t first, flecs::entity_t second) {
        return containsPair(runtime(world).contacts, first, second);
    }

    bool Physics2D::beganContact(flecs::world& world, flecs::entity_t first, flecs::entity_t second) {
        return containsPair(runtime(world).beganContacts, first, second);
    }

    bool Physics2D::endedContact(flecs::world& world, flecs::entity_t first, flecs::entity_t second) {
        return containsPair(runtime(world).endedContacts, first, second);
    }

    bool Physics2D::overlapsAABB(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
        return std::abs(ax - bx) * 2.0f < (std::abs(aw) + std::abs(bw)) &&
               std::abs(ay - by) * 2.0f < (std::abs(ah) + std::abs(bh));
    }

    bool Physics2D::overlapsCircle(float ax, float ay, float ar, float bx, float by, float br) {
        const float dx = ax - bx;
        const float dy = ay - by;
        const float radius = std::abs(ar) + std::abs(br);
        return dx * dx + dy * dy < radius * radius;
    }

    bool Physics2D::overlaps(flecs::entity first, flecs::entity second) {
        if (!first.is_alive() || !second.is_alive()) return false;
        const auto* firstPosition = first.get<Position2D>();
        const auto* secondPosition = second.get<Position2D>();
        if (!firstPosition || !secondPosition) return false;

        if (const auto* firstCircle = first.get<CircleCollider2D>()) {
            if (const auto* secondCircle = second.get<CircleCollider2D>()) {
                return overlapsCircle(firstPosition->x + firstCircle->offset.x,
                                      firstPosition->y + firstCircle->offset.y, firstCircle->radius,
                                      secondPosition->x + secondCircle->offset.x,
                                      secondPosition->y + secondCircle->offset.y, secondCircle->radius);
            }
        }

        const auto* firstBox = first.get<AABBCollider2D>();
        const auto* secondBox = second.get<AABBCollider2D>();
        if (!firstBox || !secondBox) return false;
        return overlapsAABB(firstPosition->x + firstBox->offset.x, firstPosition->y + firstBox->offset.y,
                            firstBox->size.x, firstBox->size.y, secondPosition->x + secondBox->offset.x,
                            secondPosition->y + secondBox->offset.y, secondBox->size.x, secondBox->size.y);
    }

} // namespace Levi
