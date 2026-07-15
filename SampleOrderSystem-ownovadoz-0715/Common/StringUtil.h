#pragma once
#include <string>
#include <vector>

namespace common {

inline std::vector<std::string> splitString(const std::string& s, char delimiter) {
    std::vector<std::string> result;
    std::string current;
    for (char c : s) {
        if (c == delimiter) {
            result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    result.push_back(current);
    return result;
}

} // namespace common
