#pragma once
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace Hockey {
class CommandLine {
public:
    CommandLine() = default;
    CommandLine(int argc, char** argv);

    bool Has(std::string_view flag) const;

    std::string GetString(std::string_view flag, std::string defaultValue = "") const;
    int GetInt(std::string_view flag, int defaultValue = 0) const;
    double GetDouble(std::string_view flag, double defaultValue = 0.0) const;
    bool GetBool(std::string_view flag, bool defaultValue = false) const;

    const std::vector<std::string>& Args() const;

private:
    std::vector<std::string> m_Args;
    std::unordered_map<std::string, std::string> m_Values;
};
}
