#pragma once
#include <istream>
#include <ostream>
#include <string>

namespace common {

// 화면 제목을 한 줄로 보여준다. 이 화면이 입력을 받는 도중 q로 취소할 수 있으면 cancellable=true를
// 넘긴다 (개별 입력 프롬프트마다 취소 안내를 반복하지 않고 화면당 한 번만 보여주기 위함).
void printScreenHeader(std::ostream& out, const std::string& title, bool cancellable = true);

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

struct DoubleResult {
    bool ok;
    bool cancelled;
    double value;
};

DoubleResult readDouble(std::istream& in);

} // namespace common
