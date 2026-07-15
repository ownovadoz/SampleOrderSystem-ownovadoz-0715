#include <iostream>

#ifdef _DEBUG
#include <gmock/gmock.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
#else

int main() {
    std::cout << "반도체 시료 생산주문관리 시스템\n";
    return 0;
}
#endif
