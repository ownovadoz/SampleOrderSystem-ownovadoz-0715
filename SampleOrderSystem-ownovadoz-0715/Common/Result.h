#pragma once
#include <string>
#include <utility>

namespace common {

struct Result {
    bool ok;
    std::string error;

    static Result success() { return Result{true, ""}; }
    static Result failure(std::string err) { return Result{false, std::move(err)}; }
};

} // namespace common
