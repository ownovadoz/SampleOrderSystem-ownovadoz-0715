#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

void setupConsoleEncoding() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

} // namespace

#ifdef _DEBUG
#include <gmock/gmock.h>

int main(int argc, char** argv) {
    setupConsoleEncoding();
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
#else

int main() {
    setupConsoleEncoding();
    std::cout << "반도체 시료 생산주문관리 시스템\n";
    return 0;
}
#endif
