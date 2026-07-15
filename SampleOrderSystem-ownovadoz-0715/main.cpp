#include <iostream>
#include "SampleClerk/SampleModel.h"
#include "SampleClerk/SampleController.h"
#include "SampleClerk/SampleView.h"

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

    sampleclerk::SampleModel sampleModel("samples.dat");
    sampleModel.load();
    sampleclerk::SampleController sampleController(sampleModel);
    sampleclerk::SampleView sampleView(sampleController);

    while (true) {
        std::cout << "\n반도체 시료 생산주문관리 시스템\n";
        std::cout << "[1] 시료 등록  [2] 시료 조회  [3] 시료 검색  [0] 종료\n";
        std::cout << "선택 > ";
        int choice = 0;
        if (!(std::cin >> choice)) break;
        std::cin.ignore();

        if (choice == 1) {
            sampleView.showRegisterScreen(std::cin, std::cout);
        } else if (choice == 2) {
            sampleView.showListScreen(std::cout);
        } else if (choice == 3) {
            sampleView.showSearchScreen(std::cin, std::cout);
        } else if (choice == 0) {
            break;
        }
    }

    sampleModel.save();
    return 0;
}
#endif
