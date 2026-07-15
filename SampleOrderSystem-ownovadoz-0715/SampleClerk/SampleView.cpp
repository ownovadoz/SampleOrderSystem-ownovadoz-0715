#include "SampleView.h"

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
    out << "시료 ID > ";
    std::string id;
    std::getline(in, id);

    out << "이름 > ";
    std::string name;
    std::getline(in, name);

    out << "평균 생산시간(분) > ";
    int minutes = 0;
    in >> minutes;

    out << "수율 > ";
    double yield = 0.0;
    in >> yield;
    in.ignore();

    auto result = controller_.registerSample(id, name, common::Duration(minutes * 60), yield);
    if (result.ok) {
        out << "등록 완료\n";
        printTable({*controller_.getSample(id)}, out);
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
    out << "검색어 > ";
    std::string keyword;
    std::getline(in, keyword);

    auto results = controller_.searchSamples(keyword);
    if (results.empty()) {
        out << "검색 결과 없음\n";
        return;
    }
    printTable(results, out);
}

} // namespace sampleclerk
