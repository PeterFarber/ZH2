#include "Hockey/Core/Config.hpp"
#include <fstream>
#include <system_error>
#include <utility>
#include <vector>
namespace Hockey {
namespace {
std::vector<std::string> SplitKey(std::string_view key) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t dot = key.find('.', start);
        if (dot == std::string_view::npos) {
            parts.emplace_back(key.substr(start));
            break;
        }
        parts.emplace_back(key.substr(start, dot - start));
        start = dot + 1;
    }
    return parts;
}

template <typename T>
void SetValue(toml::table& root, std::string_view key, T&& value) {
    const std::vector<std::string> parts = SplitKey(key);
    toml::table* current = &root;
    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        toml::node* node = current->get(parts[i]);
        if (node == nullptr || !node->is_table()) {
            current->insert_or_assign(parts[i], toml::table{});
            node = current->get(parts[i]);
        }
        current = node->as_table();
    }
    current->insert_or_assign(parts.back(), std::forward<T>(value));
}
}
Status Config::Load(const std::filesystem::path& path) {
    try {
        m_Table = toml::parse_file(path.string());
        return Status::Ok();
    } catch (const toml::parse_error& error) {
        return Status::Fail("failed to parse '" + path.string() + "': " +
                            std::string(error.description()));
    }
}
Status Config::Save(const std::filesystem::path& path) const {
    if (path.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(path.parent_path(), ec);
    }
    std::ofstream stream(path, std::ios::trunc);
    if (!stream.is_open()) {
        return Status::Fail("cannot open config for write: " + path.string());
    }
    stream << m_Table;
    if (!stream.good()) {
        return Status::Fail("failed while writing config: " + path.string());
    }
    return Status::Ok();
}
bool Config::Has(std::string_view key) const {
    return m_Table.at_path(key).node() != nullptr;
}
std::string Config::GetString(std::string_view key, std::string defaultValue) const {
    if (auto value = m_Table.at_path(key).value<std::string>()) {
        return *value;
    }
    return defaultValue;
}
int Config::GetInt(std::string_view key, int defaultValue) const {
    if (auto value = m_Table.at_path(key).value<int64_t>()) {
        return static_cast<int>(*value);
    }
    return defaultValue;
}
double Config::GetDouble(std::string_view key, double defaultValue) const {
    const auto node = m_Table.at_path(key);
    if (auto value = node.value<double>()) {
        return *value;
    }
    if (auto value = node.value<int64_t>()) {
        return static_cast<double>(*value);
    }
    return defaultValue;
}
bool Config::GetBool(std::string_view key, bool defaultValue) const {
    if (auto value = m_Table.at_path(key).value<bool>()) {
        return *value;
    }
    return defaultValue;
}
void Config::SetString(std::string_view key, std::string value) {
    SetValue(m_Table, key, std::move(value));
}
void Config::SetInt(std::string_view key, int value) {
    SetValue(m_Table, key, static_cast<int64_t>(value));
}
void Config::SetDouble(std::string_view key, double value) {
    SetValue(m_Table, key, value);
}
void Config::SetBool(std::string_view key, bool value) {
    SetValue(m_Table, key, value);
}
}
