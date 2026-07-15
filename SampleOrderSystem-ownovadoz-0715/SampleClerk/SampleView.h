#pragma once
#include <iostream>
#include <vector>
#include "SampleController.h"

namespace sampleclerk {

class SampleView {
public:
    explicit SampleView(SampleController& controller);

    void showRegisterScreen(std::istream& in, std::ostream& out);
    void showListScreen(std::ostream& out);
    void showSearchScreen(std::istream& in, std::ostream& out);

private:
    void printTable(const std::vector<common::Sample>& samples, std::ostream& out);
    SampleController& controller_;
};

} // namespace sampleclerk
