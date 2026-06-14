#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>
#include "Hockey/Core/Result.hpp"
namespace Hockey {
class Config {
public:
    Status Load(const std::filesystem::path& path);
    Status Save(const std::filesystem::path& path) const;

    bool Has(std::string_view key) const;

    std::string GetString(std::string_view key, std::string defaultValue = "") const;
    int GetInt(std::string_view key, int defaultValue = 0) const;
    double GetDouble(std::string_view key, double defaultValue = 0.0) const;
    bool GetBool(std::string_view key, bool defaultValue = false) const;

    void SetString(std::string_view key, std::string value);
    void SetInt(std::string_view key, int value);
    void SetDouble(std::string_view key, double value);
    void SetBool(std::string_view key, bool value);

private:
    toml::table m_Table;
};
}
