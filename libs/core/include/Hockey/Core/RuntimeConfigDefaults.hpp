#pragma once

#include <string_view>

#include "Hockey/Core/Config.hpp"

namespace Hockey {

std::string_view DefaultRuntimeConfigToml();
Config MakeDefaultRuntimeConfig();

} // namespace Hockey
