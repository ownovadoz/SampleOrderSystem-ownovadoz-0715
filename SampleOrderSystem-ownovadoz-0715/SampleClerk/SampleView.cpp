#include "SampleView.h"
#include "../Common/ConsoleInput.h"
#include <cmath>

namespace sampleclerk {

SampleView::SampleView(SampleController& controller) : controller_(controller) {}

void SampleView::printTable(const std::vector<common::Sample>& samples, std::ostream& out) {
    out << "ID\t이름\t평균생산시간(분)\t수율\t재고\n";
    for (const auto& sample : samples) {
        out << sample.id << '\t' << sample.name << '\t'
            << (sample.avgProductionTime.count() / 60) << '\t'
            << sample.yield << '\t' << sample.stock << '\n';
    }
}

void SampleView::showRegisterScreen(std::istream& in, std::ostream& out) {
    out << "시료 ID (취소: q) > ";
    auto idInput = common::readLine(in);
    if (!idInput.ok || idInput.cancelled) {
        out << "취소되었습니다\n";
        return;
    }

    out << "이름 (취소: q) > ";
    auto nameInput = common::readLine(in);
    if (!nameInput.ok || nameInput.cancelled) {
        out << "취소되었습니다\n";
        return;
    }

    out << "평균 생산시간(분, 소수 가능. 예: 1.5) (취소: q) > ";
    auto minutesInput = common::readDouble(in);
    if (!minutesInput.ok) {
        out << (minutesInput.cancelled ? "취소되었습니다\n" : "잘못된 입력입니다\n");
        return;
    }

    out << "수율 (취소: q) > ";
    auto yieldInput = common::readDouble(in);
    if (!yieldInput.ok) {
        out << (yieldInput.cancelled ? "취소되었습니다\n" : "잘못된 입력입니다\n");
        return;
    }

    long long seconds = std::llround(minutesInput.value * 60.0);
    auto result = controller_.registerSample(idInput.text, nameInput.text, common::Duration(seconds),
                                              yieldInput.value);
    if (result.ok) {
        out << "등록 완료\n";
        printTable({*controller_.getSample(idInput.text)}, out);
    } else {
        out << "등록 실패: " << result.error << "\n";
    }
}

void SampleView::showListScreen(std::ostream& out) {
    auto samples = controller_.listSamples();
    if (samples.empty()) {
        out << "등록된 시료가 없습니다\n";
        return;
    }
    printTable(samples, out);
}

void SampleView::showSearchScreen(std::istream& in, std::ostream& out) {
    out << "검색어 (취소: q) > ";
    auto keywordInput = common::readLine(in);
    if (!keywordInput.ok || keywordInput.cancelled) {
        out << "취소되었습니다\n";
        return;
    }

    auto results = controller_.searchSamples(keywordInput.text);
    if (results.empty()) {
        out << "검색 결과 없음\n";
        return;
    }
    printTable(results, out);
}

} // namespace sampleclerk
