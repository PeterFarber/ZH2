#pragma once

#include <filesystem>

#include "Hockey/Core/Result.hpp"
#include "Hockey/UI/ClientFlow.hpp"

namespace Hockey {

class ClientFlowSerializer {
public:
    static Result<ClientFlow> Load(const std::filesystem::path& path);
    static Status Save(const ClientFlow& flow, const std::filesystem::path& path);
};

} // namespace Hockey
