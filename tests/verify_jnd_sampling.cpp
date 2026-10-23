
#include "IQM/hdrvdp3/jnd.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>

int main() {
    using namespace IQM::HDRVDP3;

    const JNDLUT lut = create_jnd_lut();

    constexpr std::array<double, 15> luminances = {
        1e-8, 1e-5, 1e-4, 1e-3, 0.01,
        0.1, 0.5, 1.0, 2.0, 10.0,
        50.0, 100.0, 1000.0, 1e5, 1e8
    };

    std::ofstream out("jnd_sampling_cpp.csv");

    if (!out) {
        std::cerr << "Cannot create output file\n";
        return 1;
    }

    out << std::setprecision(17);

    for (double luminance : luminances) {
        const double cone =
            sample_jnd(lut, luminance, false);

        const double rod =
            sample_jnd(lut, luminance, true);

        out << luminance << ","
            << cone << ","
            << rod << "\n";
    }

    std::cout << "Exported "
              << luminances.size()
              << " JND sampling results\n";

    return 0;
}
