#include "Hockey/Core/Config.hpp"
#include <fstream>
#include <sstream>
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

void AssignNode(toml::table& table, const toml::key& key, const toml::node& node) {
    const std::string keyName{key.str()};

    if (const auto* tableValue = node.as_table()) {
        table.insert_or_assign(keyName, *tableValue);
    } else if (const auto* arrayValue = node.as_array()) {
        table.insert_or_assign(keyName, *arrayValue);
    } else if (const auto stringValue = node.value<std::string>()) {
        table.insert_or_assign(keyName, *stringValue);
    } else if (const auto integerValue = node.value<int64_t>()) {
        table.insert_or_assign(keyName, *integerValue);
    } else if (const auto doubleValue = node.value<double>()) {
        table.insert_or_assign(keyName, *doubleValue);
    } else if (const auto boolValue = node.value<bool>()) {
        table.insert_or_assign(keyName, *boolValue);
    } else if (const auto dateValue = node.value<toml::date>()) {
        table.insert_or_assign(keyName, *dateValue);
    } else if (const auto timeValue = node.value<toml::time>()) {
        table.insert_or_assign(keyName, *timeValue);
    } else if (const auto dateTimeValue = node.value<toml::date_time>()) {
        table.insert_or_assign(keyName, *dateTimeValue);
    }
}

void MergeTables(toml::table& base, const toml::table& overlay) {
    for (const auto& [key, overlayNode] : overlay) {
        if (const auto* overlayTable = overlayNode.as_table()) {
            if (toml::node* baseNode = base.get(key); baseNode != nullptr && baseNode->is_table()) {
                MergeTables(*baseNode->as_table(), *overlayTable);
                continue;
            }
        }

        AssignNode(base, key, overlayNode);
    }
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
Status Config::LoadString(std::string_view tomlText, std::string_view sourceName) {
    try {
        m_Table = toml::parse(tomlText, std::string(sourceName));
        return Status::Ok();
    } catch (const toml::parse_error& error) {
        return Status::Fail("failed to parse '" + std::string(sourceName) + "': " +
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
std::string Config::ToTomlString() const {
    std::ostringstream stream;
    stream << m_Table;
    return stream.str();
}
void Config::MergeFrom(const Config& overlay) {
    MergeTables(m_Table, overlay.m_Table);
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
