#pragma once

#include <filesystem>
#include <string>

#include "Hockey/Core/Result.hpp"
#include "Hockey/Gameplay/Tuning/GameplayTuning.hpp"

namespace Hockey {

class TuningSerializer {
public:
    static std::string Serialize(const GameplayTuning& tuning);
    static bool Deserialize(const std::string& text, GameplayTuning& outTuning);
    static Status Save(const std::filesystem::path& path, const GameplayTuning& tuning);
    static Result<GameplayTuning> Load(const std::filesystem::path& path);
};

}
