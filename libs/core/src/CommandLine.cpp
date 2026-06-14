#include "Hockey/Core/CommandLine.hpp"
#include <algorithm>
#include <cctype>
namespace Hockey {
namespace {
bool IsFlag(const std::string& token) { return token.rfind("--", 0) == 0; }

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}
}
CommandLine::CommandLine(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        m_Args.emplace_back(argv[i]);
    }
    for (size_t i = 0; i < m_Args.size(); ++i) {
        const std::string& token = m_Args[i];
        if (!IsFlag(token)) {
            continue;
        }
        if (i + 1 < m_Args.size() && !IsFlag(m_Args[i + 1])) {
            m_Values[token] = m_Args[i + 1];
            ++i;
        } else {
            m_Values[token] = "true";
        }
    }
}
bool CommandLine::Has(std::string_view flag) const {
    return m_Values.find(std::string(flag)) != m_Values.end();
}
std::string CommandLine::GetString(std::string_view flag, std::string defaultValue) const {
    auto it = m_Values.find(std::string(flag));
    return it != m_Values.end() ? it->second : defaultValue;
}
int CommandLine::GetInt(std::string_view flag, int defaultValue) const {
    auto it = m_Values.find(std::string(flag));
    if (it == m_Values.end()) {
        return defaultValue;
    }
    try {
        return std::stoi(it->second);
    } catch (...) {
        return defaultValue;
    }
}
double CommandLine::GetDouble(std::string_view flag, double defaultValue) const {
    auto it = m_Values.find(std::string(flag));
    if (it == m_Values.end()) {
        return defaultValue;
    }
    try {
        return std::stod(it->second);
    } catch (...) {
        return defaultValue;
    }
}
bool CommandLine::GetBool(std::string_view flag, bool defaultValue) const {
    auto it = m_Values.find(std::string(flag));
    if (it == m_Values.end()) {
        return defaultValue;
    }
    const std::string value = ToLower(it->second);
    return value == "true" || value == "1" || value == "yes" || value == "on";
}
const std::vector<std::string>& CommandLine::Args() const { return m_Args; }
}
