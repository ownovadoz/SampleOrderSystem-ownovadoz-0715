#pragma once
#include <istream>
#include <string>

namespace common {

// 콘솔 입력 한 줄을 읽는다. operator>>는 파싱 실패 시 스트림을 실패 상태로 남겨 이후 모든 입력이
// 연쇄적으로 실패하는 문제가 있어(예: 잘못된 메뉴 번호 입력 시 프로그램이 그대로 종료됨), 이 프로젝트의
// 모든 사용자 입력은 getline 기반의 이 유틸리티를 거친다. "q"/"Q" 한 글자만 입력하면 취소로 간주한다.
struct LineResult {
    bool ok;        // 스트림에서 한 줄을 정상적으로 읽었으면 true (EOF/오류면 false)
    bool cancelled; // 입력이 "q" 또는 "Q" 하나였으면 true
    std::string text;
};

LineResult readLine(std::istream& in);

struct IntResult {
    bool ok;        // 정수로 정상 파싱됐으면 true
    bool cancelled; // 입력이 "q" 또는 "Q" 하나였으면 true (이때 ok는 false)
    int value;
};

IntResult readInt(std::istream& in);

} // namespace common
