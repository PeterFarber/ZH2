#include "Hockey/Physics/PhysicsComponents.hpp"

#include <cstddef>

#include "Hockey/ECS/ComponentMetadata.hpp"
#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/ComponentSerializer.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/YAML.hpp"
#include "Hockey/Physics/PhysicsValidation.hpp"

namespace Hockey {

const char* RigidBodyTypeToString(RigidBodyType type) {
    switch (type) {
    case RigidBodyType::Static:
        return "Static";
    case RigidBodyType::Kinematic:
        return "Kinematic";
    case RigidBodyType::Dynamic:
        return "Dynamic";
    }
    return "Static";
}

bool RigidBodyTypeFromString(std::string_view text, RigidBodyType& out) {
    if (text == "Static") {
        out = RigidBodyType::Static;
        return true;
    }
    if (text == "Kinematic") {
        out = RigidBodyType::Kinematic;
        return true;
    }
    if (text == "Dynamic") {
        out = RigidBodyType::Dynamic;
        return true;
    }
    return false;
}

namespace {

// ------------------------------------------------------------------ YAML ----

void SerializePhysics(YAML::Emitter& out, Entity entity) {
    if (entity.HasComponent<RigidBodyComponent>()) {
        const auto& rb = entity.GetComponent<RigidBodyComponent>();
        out << YAML::Key << "RigidBodyComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Type" << YAML::Value << RigidBodyTypeToString(rb.type);
        out << YAML::Key << "Mass" << YAML::Value << rb.mass;
        out << YAML::Key << "UseGravity" << YAML::Value << rb.useGravity;
        out << YAML::Key << "AllowSleeping" << YAML::Value << rb.allowSleeping;
        out << YAML::Key << "LockTranslationX" << YAML::Value << rb.lockTranslationX;
        out << YAML::Key << "LockTranslationY" << YAML::Value << rb.lockTranslationY;
        out << YAML::Key << "LockTranslationZ" << YAML::Value << rb.lockTranslationZ;
        out << YAML::Key << "LockRotationX" << YAML::Value << rb.lockRotationX;
        out << YAML::Key << "LockRotationY" << YAML::Value << rb.lockRotationY;
        out << YAML::Key << "LockRotationZ" << YAML::Value << rb.lockRotationZ;
        out << YAML::Key << "LinearDamping" << YAML::Value << rb.linearDamping;
        out << YAML::Key << "AngularDamping" << YAML::Value << rb.angularDamping;
        out << YAML::Key << "InitialLinearVelocity" << YAML::Value << rb.initialLinearVelocity;
        out << YAML::Key << "InitialAngularVelocity" << YAML::Value << rb.initialAngularVelocity;
        out << YAML::Key << "Layer" << YAML::Value << PhysicsLayerToString(rb.layer);
        out << YAML::Key << "Material" << YAML::Value << rb.materialName;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<BoxColliderComponent>()) {
        const auto& c = entity.GetComponent<BoxColliderComponent>();
        out << YAML::Key << "BoxColliderComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "HalfExtents" << YAML::Value << c.halfExtents;
        out << YAML::Key << "Offset" << YAML::Value << c.offset;
        out << YAML::Key << "Rotation" << YAML::Value << c.rotation;
        out << YAML::Key << "IsTrigger" << YAML::Value << c.isTrigger;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<SphereColliderComponent>()) {
        const auto& c = entity.GetComponent<SphereColliderComponent>();
        out << YAML::Key << "SphereColliderComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Radius" << YAML::Value << c.radius;
        out << YAML::Key << "Offset" << YAML::Value << c.offset;
        out << YAML::Key << "IsTrigger" << YAML::Value << c.isTrigger;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<CapsuleColliderComponent>()) {
        const auto& c = entity.GetComponent<CapsuleColliderComponent>();
        out << YAML::Key << "CapsuleColliderComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Radius" << YAML::Value << c.radius;
        out << YAML::Key << "HalfHeight" << YAML::Value << c.halfHeight;
        out << YAML::Key << "Offset" << YAML::Value << c.offset;
        out << YAML::Key << "Rotation" << YAML::Value << c.rotation;
        out << YAML::Key << "IsTrigger" << YAML::Value << c.isTrigger;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<CylinderColliderComponent>()) {
        const auto& c = entity.GetComponent<CylinderColliderComponent>();
        out << YAML::Key << "CylinderColliderComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Radius" << YAML::Value << c.radius;
        out << YAML::Key << "HalfHeight" << YAML::Value << c.halfHeight;
        out << YAML::Key << "Offset" << YAML::Value << c.offset;
        out << YAML::Key << "Rotation" << YAML::Value << c.rotation;
        out << YAML::Key << "IsTrigger" << YAML::Value << c.isTrigger;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<MeshColliderComponent>()) {
        const auto& c = entity.GetComponent<MeshColliderComponent>();
        out << YAML::Key << "MeshColliderComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "MeshAsset" << YAML::Value << c.meshAsset;
        out << YAML::Key << "Convex" << YAML::Value << c.convex;
        out << YAML::Key << "IsTrigger" << YAML::Value << c.isTrigger;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<TriggerComponent>()) {
        const auto& c = entity.GetComponent<TriggerComponent>();
        out << YAML::Key << "TriggerComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Tag" << YAML::Value << c.tag;
        out << YAML::Key << "DetectPlayers" << YAML::Value << c.detectPlayers;
        out << YAML::Key << "DetectGoalies" << YAML::Value << c.detectGoalies;
        out << YAML::Key << "DetectPuck" << YAML::Value << c.detectPuck;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<CharacterControllerComponent>()) {
        const auto& c = entity.GetComponent<CharacterControllerComponent>();
        out << YAML::Key << "CharacterControllerComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Radius" << YAML::Value << c.radius;
        out << YAML::Key << "Height" << YAML::Value << c.height;
        out << YAML::Key << "StepHeight" << YAML::Value << c.stepHeight;
        out << YAML::Key << "MaxSlopeDegrees" << YAML::Value << c.maxSlopeDegrees;
        out << YAML::EndMap;
    }

    if (entity.HasComponent<PhysicsMaterialComponent>()) {
        const auto& c = entity.GetComponent<PhysicsMaterialComponent>();
        out << YAML::Key << "PhysicsMaterialComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Material" << YAML::Value << c.materialName;
        out << YAML::EndMap;
    }
}

void DeserializePhysics(Entity entity, const YAML::Node& node) {
    if (const auto n = node["RigidBodyComponent"]) {
        RigidBodyComponent rb;
        if (n["Type"]) {
            RigidBodyTypeFromString(n["Type"].as<std::string>(), rb.type);
        }
        if (n["Mass"]) {
            rb.mass = n["Mass"].as<float>();
        }
        if (n["UseGravity"]) {
            rb.useGravity = n["UseGravity"].as<bool>();
        }
        if (n["AllowSleeping"]) {
            rb.allowSleeping = n["AllowSleeping"].as<bool>();
        }
        if (n["LockTranslationX"]) {
            rb.lockTranslationX = n["LockTranslationX"].as<bool>();
        }
        if (n["LockTranslationY"]) {
            rb.lockTranslationY = n["LockTranslationY"].as<bool>();
        }
        if (n["LockTranslationZ"]) {
            rb.lockTranslationZ = n["LockTranslationZ"].as<bool>();
        }
        if (n["LockRotationX"]) {
            rb.lockRotationX = n["LockRotationX"].as<bool>();
        }
        if (n["LockRotationY"]) {
            rb.lockRotationY = n["LockRotationY"].as<bool>();
        }
        if (n["LockRotationZ"]) {
            rb.lockRotationZ = n["LockRotationZ"].as<bool>();
        }
        if (n["LinearDamping"]) {
            rb.linearDamping = n["LinearDamping"].as<float>();
        }
        if (n["AngularDamping"]) {
            rb.angularDamping = n["AngularDamping"].as<float>();
        }
        ReadVec3(n["InitialLinearVelocity"], rb.initialLinearVelocity);
        ReadVec3(n["InitialAngularVelocity"], rb.initialAngularVelocity);
        if (n["Layer"]) {
            PhysicsLayerFromString(n["Layer"].as<std::string>(), rb.layer);
        }
        if (n["Material"]) {
            rb.materialName = n["Material"].as<std::string>();
        }
        entity.AddOrReplaceComponent<RigidBodyComponent>(rb);
    }

    if (const auto n = node["BoxColliderComponent"]) {
        BoxColliderComponent c;
        ReadVec3(n["HalfExtents"], c.halfExtents);
        ReadVec3(n["Offset"], c.offset);
        ReadVec3(n["Rotation"], c.rotation);
        if (n["IsTrigger"]) {
            c.isTrigger = n["IsTrigger"].as<bool>();
        }
        entity.AddOrReplaceComponent<BoxColliderComponent>(c);
    }

    if (const auto n = node["SphereColliderComponent"]) {
        SphereColliderComponent c;
        if (n["Radius"]) {
            c.radius = n["Radius"].as<float>();
        }
        ReadVec3(n["Offset"], c.offset);
        if (n["IsTrigger"]) {
            c.isTrigger = n["IsTrigger"].as<bool>();
        }
        entity.AddOrReplaceComponent<SphereColliderComponent>(c);
    }

    if (const auto n = node["CapsuleColliderComponent"]) {
        CapsuleColliderComponent c;
        if (n["Radius"]) {
            c.radius = n["Radius"].as<float>();
        }
        if (n["HalfHeight"]) {
            c.halfHeight = n["HalfHeight"].as<float>();
        }
        ReadVec3(n["Offset"], c.offset);
        ReadVec3(n["Rotation"], c.rotation);
        if (n["IsTrigger"]) {
            c.isTrigger = n["IsTrigger"].as<bool>();
        }
        entity.AddOrReplaceComponent<CapsuleColliderComponent>(c);
    }

    if (const auto n = node["CylinderColliderComponent"]) {
        CylinderColliderComponent c;
        if (n["Radius"]) {
            c.radius = n["Radius"].as<float>();
        }
        if (n["HalfHeight"]) {
            c.halfHeight = n["HalfHeight"].as<float>();
        }
        ReadVec3(n["Offset"], c.offset);
        ReadVec3(n["Rotation"], c.rotation);
        if (n["IsTrigger"]) {
            c.isTrigger = n["IsTrigger"].as<bool>();
        }
        entity.AddOrReplaceComponent<CylinderColliderComponent>(c);
    }

    if (const auto n = node["MeshColliderComponent"]) {
        MeshColliderComponent c;
        if (n["MeshAsset"]) {
            c.meshAsset = n["MeshAsset"].as<std::uint64_t>();
        }
        if (n["Convex"]) {
            c.convex = n["Convex"].as<bool>();
        }
        if (n["IsTrigger"]) {
            c.isTrigger = n["IsTrigger"].as<bool>();
        }
        entity.AddOrReplaceComponent<MeshColliderComponent>(c);
    }

    if (const auto n = node["TriggerComponent"]) {
        TriggerComponent c;
        if (n["Tag"]) {
            c.tag = n["Tag"].as<std::string>();
        }
        if (n["DetectPlayers"]) {
            c.detectPlayers = n["DetectPlayers"].as<bool>();
        }
        if (n["DetectGoalies"]) {
            c.detectGoalies = n["DetectGoalies"].as<bool>();
        }
        if (n["DetectPuck"]) {
            c.detectPuck = n["DetectPuck"].as<bool>();
        }
        entity.AddOrReplaceComponent<TriggerComponent>(c);
    }

    if (const auto n = node["CharacterControllerComponent"]) {
        CharacterControllerComponent c;
        if (n["Radius"]) {
            c.radius = n["Radius"].as<float>();
        }
        if (n["Height"]) {
            c.height = n["Height"].as<float>();
        }
        if (n["StepHeight"]) {
            c.stepHeight = n["StepHeight"].as<float>();
        }
        if (n["MaxSlopeDegrees"]) {
            c.maxSlopeDegrees = n["MaxSlopeDegrees"].as<float>();
        }
        entity.AddOrReplaceComponent<CharacterControllerComponent>(c);
    }

    if (const auto n = node["PhysicsMaterialComponent"]) {
        PhysicsMaterialComponent c;
        if (n["Material"]) {
            c.materialName = n["Material"].as<std::string>();
        }
        entity.AddOrReplaceComponent<PhysicsMaterialComponent>(c);
    }
}

// -------------------------------------------------------------- metadata ----

FieldMetadata MakeField(std::string name, FieldType type, std::size_t offset, std::string displayName = {}) {
    FieldMetadata field;
    field.name = std::move(name);
    field.displayName = displayName.empty() ? field.name : std::move(displayName);
    field.type = type;
    field.offset = offset;
    return field;
}

FieldMetadata MakeRigidBodyTypeField(std::size_t offset) {
    FieldMetadata field = MakeField("Type", FieldType::Enum, offset);
    field.enumNames = {"Static", "Kinematic", "Dynamic"};
    field.enumValues = {static_cast<int>(RigidBodyType::Static), static_cast<int>(RigidBodyType::Kinematic),
                        static_cast<int>(RigidBodyType::Dynamic)};
    return field;
}

FieldMetadata MakeLayerField(std::string name, std::size_t offset) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Enum, offset);
    field.enumNames = {"Static", "Player", "Goalie", "Puck", "Stick", "Rink", "Goal", "Trigger", "Sensor", "Editor"};
    for (int i = 0; i < static_cast<int>(kPhysicsLayerCount); ++i) {
        field.enumValues.push_back(i);
    }
    return field;
}

void RegisterMetadata() {
    ComponentRegistry& registry = ComponentRegistry::Get();
    if (registry.FindByName("RigidBodyComponent") != nullptr) {
        return; // already registered
    }

    {
        ComponentMetadata md;
        md.name = "RigidBodyComponent";
        md.displayName = "Rigid Body";
        md.fields.push_back(MakeRigidBodyTypeField(offsetof(RigidBodyComponent, type)));
        md.fields.push_back(MakeField("Mass", FieldType::Float, offsetof(RigidBodyComponent, mass)));
        md.fields.push_back(MakeField("UseGravity", FieldType::Bool, offsetof(RigidBodyComponent, useGravity)));
        md.fields.push_back(MakeField("AllowSleeping", FieldType::Bool, offsetof(RigidBodyComponent, allowSleeping)));
        md.fields.push_back(
            MakeField("LockTranslationX", FieldType::Bool, offsetof(RigidBodyComponent, lockTranslationX)));
        md.fields.push_back(
            MakeField("LockTranslationY", FieldType::Bool, offsetof(RigidBodyComponent, lockTranslationY)));
        md.fields.push_back(
            MakeField("LockTranslationZ", FieldType::Bool, offsetof(RigidBodyComponent, lockTranslationZ)));
        md.fields.push_back(MakeField("LockRotationX", FieldType::Bool, offsetof(RigidBodyComponent, lockRotationX)));
        md.fields.push_back(MakeField("LockRotationY", FieldType::Bool, offsetof(RigidBodyComponent, lockRotationY)));
        md.fields.push_back(MakeField("LockRotationZ", FieldType::Bool, offsetof(RigidBodyComponent, lockRotationZ)));
        md.fields.push_back(MakeField("LinearDamping", FieldType::Float, offsetof(RigidBodyComponent, linearDamping)));
        md.fields.push_back(
            MakeField("AngularDamping", FieldType::Float, offsetof(RigidBodyComponent, angularDamping)));
        md.fields.push_back(
            MakeField("InitialLinearVelocity", FieldType::Vec3, offsetof(RigidBodyComponent, initialLinearVelocity)));
        md.fields.push_back(
            MakeField("InitialAngularVelocity", FieldType::Vec3, offsetof(RigidBodyComponent, initialAngularVelocity)));
        md.fields.push_back(MakeLayerField("Layer", offsetof(RigidBodyComponent, layer)));
        md.fields.push_back(MakeField("Material", FieldType::String, offsetof(RigidBodyComponent, materialName)));
        registry.RegisterComponent<RigidBodyComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "BoxColliderComponent";
        md.displayName = "Box Collider";
        md.fields.push_back(MakeField("HalfExtents", FieldType::Vec3, offsetof(BoxColliderComponent, halfExtents)));
        md.fields.push_back(MakeField("Offset", FieldType::Vec3, offsetof(BoxColliderComponent, offset)));
        md.fields.push_back(MakeField("Rotation", FieldType::Vec3, offsetof(BoxColliderComponent, rotation)));
        md.fields.push_back(MakeField("IsTrigger", FieldType::Bool, offsetof(BoxColliderComponent, isTrigger)));
        registry.RegisterComponent<BoxColliderComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "SphereColliderComponent";
        md.displayName = "Sphere Collider";
        md.fields.push_back(MakeField("Radius", FieldType::Float, offsetof(SphereColliderComponent, radius)));
        md.fields.push_back(MakeField("Offset", FieldType::Vec3, offsetof(SphereColliderComponent, offset)));
        md.fields.push_back(MakeField("IsTrigger", FieldType::Bool, offsetof(SphereColliderComponent, isTrigger)));
        registry.RegisterComponent<SphereColliderComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "CapsuleColliderComponent";
        md.displayName = "Capsule Collider";
        md.fields.push_back(MakeField("Radius", FieldType::Float, offsetof(CapsuleColliderComponent, radius)));
        md.fields.push_back(MakeField("HalfHeight", FieldType::Float, offsetof(CapsuleColliderComponent, halfHeight)));
        md.fields.push_back(MakeField("Offset", FieldType::Vec3, offsetof(CapsuleColliderComponent, offset)));
        md.fields.push_back(MakeField("Rotation", FieldType::Vec3, offsetof(CapsuleColliderComponent, rotation)));
        md.fields.push_back(MakeField("IsTrigger", FieldType::Bool, offsetof(CapsuleColliderComponent, isTrigger)));
        registry.RegisterComponent<CapsuleColliderComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "CylinderColliderComponent";
        md.displayName = "Cylinder Collider";
        md.fields.push_back(MakeField("Radius", FieldType::Float, offsetof(CylinderColliderComponent, radius)));
        md.fields.push_back(MakeField("HalfHeight", FieldType::Float, offsetof(CylinderColliderComponent, halfHeight)));
        md.fields.push_back(MakeField("Offset", FieldType::Vec3, offsetof(CylinderColliderComponent, offset)));
        md.fields.push_back(MakeField("Rotation", FieldType::Vec3, offsetof(CylinderColliderComponent, rotation)));
        md.fields.push_back(MakeField("IsTrigger", FieldType::Bool, offsetof(CylinderColliderComponent, isTrigger)));
        registry.RegisterComponent<CylinderColliderComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "MeshColliderComponent";
        md.displayName = "Mesh Collider";
        FieldMetadata meshAsset =
            MakeField("MeshAsset", FieldType::AssetRef, offsetof(MeshColliderComponent, meshAsset));
        meshAsset.assetTypeName = "Mesh";
        md.fields.push_back(meshAsset);
        md.fields.push_back(MakeField("Convex", FieldType::Bool, offsetof(MeshColliderComponent, convex)));
        md.fields.push_back(MakeField("IsTrigger", FieldType::Bool, offsetof(MeshColliderComponent, isTrigger)));
        registry.RegisterComponent<MeshColliderComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "TriggerComponent";
        md.displayName = "Trigger";
        md.fields.push_back(MakeField("Tag", FieldType::String, offsetof(TriggerComponent, tag)));
        md.fields.push_back(MakeField("DetectPlayers", FieldType::Bool, offsetof(TriggerComponent, detectPlayers)));
        md.fields.push_back(MakeField("DetectGoalies", FieldType::Bool, offsetof(TriggerComponent, detectGoalies)));
        md.fields.push_back(MakeField("DetectPuck", FieldType::Bool, offsetof(TriggerComponent, detectPuck)));
        registry.RegisterComponent<TriggerComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "CharacterControllerComponent";
        md.displayName = "Character Controller";
        md.fields.push_back(MakeField("Radius", FieldType::Float, offsetof(CharacterControllerComponent, radius)));
        md.fields.push_back(MakeField("Height", FieldType::Float, offsetof(CharacterControllerComponent, height)));
        md.fields.push_back(
            MakeField("StepHeight", FieldType::Float, offsetof(CharacterControllerComponent, stepHeight)));
        md.fields.push_back(
            MakeField("MaxSlopeDegrees", FieldType::Float, offsetof(CharacterControllerComponent, maxSlopeDegrees)));
        registry.RegisterComponent<CharacterControllerComponent>(std::move(md));
    }

    {
        ComponentMetadata md;
        md.name = "PhysicsMaterialComponent";
        md.displayName = "Physics Material";
        md.fields.push_back(MakeField("Material", FieldType::String, offsetof(PhysicsMaterialComponent, materialName)));
        registry.RegisterComponent<PhysicsMaterialComponent>(std::move(md));
    }
}

} // namespace

void RegisterPhysicsComponents() {
    RegisterMetadata();

    static bool s_SerializationRegistered = false;
    if (!s_SerializationRegistered) {
        ComponentSerializer::RegisterExternal(SerializePhysics, DeserializePhysics);
        s_SerializationRegistered = true;
    }

    RegisterPhysicsValidation();
}

} // namespace Hockey
