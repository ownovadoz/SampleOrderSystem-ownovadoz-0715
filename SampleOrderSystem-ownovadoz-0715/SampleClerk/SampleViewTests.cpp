#include "SampleView.h"
#include <gtest/gtest.h>
#include <sstream>

using namespace sampleclerk;
using namespace common;

TEST(SampleClerkTest, SampleView_ListScreen_ShowsEmptyMessage) {
    SampleModel model("view_test_empty.dat");
    SampleController controller(model);
    SampleView view(controller);
    std::ostringstream out;
    view.showListScreen(out);
    EXPECT_NE(out.str().find("등록된 시료가 없습니다"), std::string::npos);
}

TEST(SampleClerkTest, SampleView_RegisterScreen_SuccessShowsSample) {
    SampleModel model("view_test_register.dat");
    SampleController controller(model);
    SampleView view(controller);
    std::istringstream in("S-001\n실리콘 웨이퍼\n30\n0.92\n");
    std::ostringstream out;
    view.showRegisterScreen(in, out);
    EXPECT_NE(out.str().find("등록 완료"), std::string::npos);
    EXPECT_NE(out.str().find("실리콘 웨이퍼"), std::string::npos);
}

TEST(SampleClerkTest, SampleView_SearchScreen_NoResults) {
    SampleModel model("view_test_search.dat");
    SampleController controller(model);
    SampleView view(controller);
    std::istringstream in("존재하지않는이름\n");
    std::ostringstream out;
    view.showSearchScreen(in, out);
    EXPECT_NE(out.str().find("검색 결과 없음"), std::string::npos);
}
