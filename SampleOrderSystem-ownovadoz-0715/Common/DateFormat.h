#pragma once
#include <string>
#include <ctime>
#include <iomanip>
#include <sstream>
#include "Time.h"

namespace common {

inline std::string formatDateYYYYMMDD(TimePoint tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tmStruct{};
    gmtime_s(&tmStruct, &t);
    std::ostringstream oss;
    oss << std::put_time(&tmStruct, "%Y%m%d");
    return oss.str();
}

} // namespace common
