#include "ConsoleInput.h"
#include <cctype>
#include <exception>

namespace common {

namespace {

std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

bool isCancelText(const std::string& text) {
    return text == "q" || text == "Q";
}

} // namespace

LineResult readLine(std::istream& in) {
    std::string raw;
    if (!std::getline(in, raw)) {
        return LineResult{false, false, ""};
    }
    std::string trimmed = trim(raw);
    return LineResult{true, isCancelText(trimmed), trimmed};
}

IntResult readInt(std::istream& in) {
    auto line = readLine(in);
    if (!line.ok) {
        return IntResult{false, false, 0};
    }
    if (line.cancelled) {
        return IntResult{false, true, 0};
    }
    try {
        size_t consumed = 0;
        int value = std::stoi(line.text, &consumed);
        if (consumed != line.text.size()) {
            return IntResult{false, false, 0};
        }
        return IntResult{true, false, value};
    } catch (const std::exception&) {
        return IntResult{false, false, 0};
    }
}

DoubleResult readDouble(std::istream& in) {
    auto line = readLine(in);
    if (!line.ok) {
        return DoubleResult{false, false, 0.0};
    }
    if (line.cancelled) {
        return DoubleResult{false, true, 0.0};
    }
    try {
        size_t consumed = 0;
        double value = std::stod(line.text, &consumed);
        if (consumed != line.text.size()) {
            return DoubleResult{false, false, 0.0};
        }
        return DoubleResult{true, false, value};
    } catch (const std::exception&) {
        return DoubleResult{false, false, 0.0};
    }
}

} // namespace common
