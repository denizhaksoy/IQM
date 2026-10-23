
#include "IQM/hdrvdp3/jnd.h"

#include <fstream>
#include <iomanip>
#include <iostream>

int main() {
    using namespace IQM::HDRVDP3;

    const JNDLUT lut = create_jnd_lut();

    std::ofstream output("jnd_cpp.csv");
    if (!output) {
        std::cerr << "Cannot create jnd_cpp.csv\n";
        return 1;
    }

    output << std::setprecision(17);

    for (std::size_t i = 0; i < JND_LUT_SIZE; ++i) {
        output << lut.log_luminance[i] << ","
               << lut.cone[i] << ","
               << lut.rod[i] << "\n";
    }

    output.close();

    const std::size_t indices[] = {
        0, 256, 512, 1024, 1536, 2047
    };

    std::cout << std::fixed << std::setprecision(12);

    for (auto i : indices) {
        std::cout << i << "  "
                  << lut.log_luminance[i] << "  "
                  << lut.cone[i] << "  "
                  << lut.rod[i] << "\n";
    }

    return 0;
}
