#include "IQM/hdrvdp3/jnd.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>

int main() {
    const auto lut = IQM::HDRVDP3::create_jnd_lut();

    std::ifstream ref("C:/temp/ref_jnd_lut.txt");
    if (!ref) {
        std::cerr << "Cannot open Octave reference\n";
        return 1;
    }

    double maxCone = 0.0, maxRod = 0.0;
    double sumCone = 0.0, sumRod = 0.0;
    std::size_t count = 0;

    double logL, cone, rod;

    while (ref >> logL >> cone >> rod) {
        if (count >= IQM::HDRVDP3::JND_LUT_SIZE) {
            std::cerr << "Too many reference samples\n";
            return 1;
        }

        const double ec = std::abs(lut.cone[count] - cone);
        const double er = std::abs(lut.rod[count] - rod);

        maxCone = std::max(maxCone, ec);
        maxRod = std::max(maxRod, er);

        sumCone += ec * ec;
        sumRod += er * er;

        ++count;
    }

    if (count != IQM::HDRVDP3::JND_LUT_SIZE) {
        std::cerr << "Incorrect reference sample count\n";
        return 1;
    }

    std::cout << std::setprecision(12);
    std::cout << "Samples: " << count << "\n";
    std::cout << "Cone MaxAbsErr: " << maxCone << "\n";
    std::cout << "Cone RMSE: " << std::sqrt(sumCone / count) << "\n";
    std::cout << "Rod MaxAbsErr: " << maxRod << "\n";
    std::cout << "Rod RMSE: " << std::sqrt(sumRod / count) << "\n";
    std::cout << "Cone last: " << lut.cone.back() << "\n";
    std::cout << "Rod last: " << lut.rod.back() << "\n";

    const double testLuminances[] = {
    0.001, 0.01, 0.1, 1.0, 10.0, 100.0, 1000.0
};

std::cout << "\nPhotoreceptor JND responses:\n";

for (double luminance : testLuminances) {
    const double response =
        IQM::HDRVDP3::luminance_to_jnd_response(lut, luminance);

    std::cout << std::setprecision(15)
              << luminance << " "
              << response << "\n";
}

    const double tolerance = 1e-5;
    const bool passed = maxCone < tolerance && maxRod < tolerance;

    std::cout << (passed ? "JND LUT TEST PASSED\n"
                         : "JND LUT TEST FAILED\n");

    return passed ? 0 : 1;
}
