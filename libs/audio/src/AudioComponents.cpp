#include "Hockey/Audio/AudioComponents.hpp"

#include <cstddef>
#include <string>
#include <utility>

#include "Hockey/ECS/ComponentRegistry.hpp"
#include "Hockey/ECS/ComponentSerializer.hpp"
#include "Hockey/ECS/Entity.hpp"
#include "Hockey/ECS/YAML.hpp"

namespace Hockey {
namespace {

FieldMetadata MakeField(std::string name, FieldType type, std::size_t offset, std::string displayName = {}) {
    FieldMetadata field;
    field.name = std::move(name);
    field.displayName = displayName.empty() ? field.name : std::move(displayName);
    field.type = type;
    field.offset = offset;
    return field;
}

FieldMetadata MakeFloatRangeField(std::string name, std::size_t offset, float minValue, float maxValue,
                                  float speed = 0.1f, std::string displayName = {}) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Float, offset, std::move(displayName));
    field.minFloat = minValue;
    field.maxFloat = maxValue;
    field.speed = speed;
    return field;
}

FieldMetadata MakeBusField(std::string name, std::size_t offset) {
    FieldMetadata field = MakeField(std::move(name), FieldType::Enum, offset);
    field.enumNames = {"Master", "Music", "SFX", "Ambience", "UI", "Crowd"};
    field.enumValues = {static_cast<int>(AudioBusType::Master), static_cast<int>(AudioBusType::Music),
                        static_cast<int>(AudioBusType::SFX),     static_cast<int>(AudioBusType::Ambience),
                        static_cast<int>(AudioBusType::UI),      static_cast<int>(AudioBusType::Crowd)};
    return field;
}

void SerializeAudio(YAML::Emitter& out, Entity entity) {
    if (entity.HasComponent<AudioListenerComponent>()) {
        const AudioListenerComponent& c = entity.GetComponent<AudioListenerComponent>();
        out << YAML::Key << "AudioListenerComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "Primary" << YAML::Value << c.primary;
        out << YAML::EndMap;
    }
    if (entity.HasComponent<AudioSourceComponent>()) {
        const AudioSourceComponent& c = entity.GetComponent<AudioSourceComponent>();
        out << YAML::Key << "AudioSourceComponent" << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "ClipAsset" << YAML::Value << c.clipAsset;
        out << YAML::Key << "Bus" << YAML::Value << AudioBusTypeToString(c.bus);
        out << YAML::Key << "PlayOnStart" << YAML::Value << c.playOnStart;
        out << YAML::Key << "Loop" << YAML::Value << c.loop;
        out << YAML::Key << "Spatial" << YAML::Value << c.spatial;
        out << YAML::Key << "Volume" << YAML::Value << c.volume;
        out << YAML::Key << "Pitch" << YAML::Value << c.pitch;
        out << YAML::Key << "MinDistance" << YAML::Value << c.minDistance;
        out << YAML::Key << "MaxDistance" << YAML::Value << c.maxDistance;
        out << YAML::EndMap;
    }
}

void DeserializeAudio(Entity entity, const YAML::Node& node) {
    if (const YAML::Node n = node["AudioListenerComponent"]) {
        AudioListenerComponent c;
        if (n["Primary"]) {
            c.primary = n["Primary"].as<bool>();
        }
        entity.AddOrReplaceComponent<AudioListenerComponent>(c);
    }
    if (const YAML::Node n = node["AudioSourceComponent"]) {
        AudioSourceComponent c;
        if (n["ClipAsset"]) {
            c.clipAsset = n["ClipAsset"].as<std::uint64_t>();
        }
        if (n["Bus"]) {
            AudioBusTypeFromString(n["Bus"].as<std::string>(), c.bus);
        }
        if (n["PlayOnStart"]) {
            c.playOnStart = n["PlayOnStart"].as<bool>();
        }
        if (n["Loop"]) {
            c.loop = n["Loop"].as<bool>();
        }
        if (n["Spatial"]) {
            c.spatial = n["Spatial"].as<bool>();
        }
        if (n["Volume"]) {
            c.volume = n["Volume"].as<float>();
        }
        if (n["Pitch"]) {
            c.pitch = n["Pitch"].as<float>();
        }
        if (n["MinDistance"]) {
            c.minDistance = n["MinDistance"].as<float>();
        }
        if (n["MaxDistance"]) {
            c.maxDistance = n["MaxDistance"].as<float>();
        }
        entity.AddOrReplaceComponent<AudioSourceComponent>(c);
    }
}

void RegisterMetadata() {
    ComponentRegistry& registry = ComponentRegistry::Get();
    if (registry.FindByName(ComponentTraits<AudioSourceComponent>::Name) != nullptr) {
        return;
    }

    ComponentMetadata listener;
    listener.name = ComponentTraits<AudioListenerComponent>::Name;
    listener.displayName = "Audio Listener";
    listener.category = "Audio";
    listener.fields.push_back(MakeField("Primary", FieldType::Bool, offsetof(AudioListenerComponent, primary)));
    registry.RegisterComponent<AudioListenerComponent>(std::move(listener));

    ComponentMetadata source;
    source.name = ComponentTraits<AudioSourceComponent>::Name;
    source.displayName = "Audio Source";
    source.category = "Audio";
    FieldMetadata clip = MakeField("ClipAsset", FieldType::AssetRef, offsetof(AudioSourceComponent, clipAsset));
    clip.assetTypeName = "Audio";
    source.fields.push_back(clip);
    source.fields.push_back(MakeBusField("Bus", offsetof(AudioSourceComponent, bus)));
    source.fields.push_back(MakeField("PlayOnStart", FieldType::Bool, offsetof(AudioSourceComponent, playOnStart)));
    source.fields.push_back(MakeField("Loop", FieldType::Bool, offsetof(AudioSourceComponent, loop)));
    source.fields.push_back(MakeField("Spatial", FieldType::Bool, offsetof(AudioSourceComponent, spatial)));
    source.fields.push_back(MakeFloatRangeField("Volume", offsetof(AudioSourceComponent, volume), 0.0f, 1.0f, 0.01f));
    source.fields.push_back(MakeFloatRangeField("Pitch", offsetof(AudioSourceComponent, pitch), 0.25f, 4.0f, 0.01f));
    source.fields.push_back(
        MakeFloatRangeField("MinDistance", offsetof(AudioSourceComponent, minDistance), 0.0f, 500.0f));
    source.fields.push_back(
        MakeFloatRangeField("MaxDistance", offsetof(AudioSourceComponent, maxDistance), 0.0f, 500.0f));
    registry.RegisterComponent<AudioSourceComponent>(std::move(source));
}

} // namespace

void RegisterAudioComponents() {
    RegisterMetadata();

    static bool s_SerializationRegistered = false;
    if (!s_SerializationRegistered) {
        ComponentSerializer::RegisterExternal(SerializeAudio, DeserializeAudio);
        s_SerializationRegistered = true;
    }
}

} // namespace Hockey
