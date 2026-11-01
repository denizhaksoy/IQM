#include <IQM/hdrvdp3/csf.h>

#include <array>
#include <cmath>
#include <iomanip>
#include <iostream>

int main() {
    constexpr std::array<double, 7> rho = {
        0.5, 1.0, 2.0, 4.0, 8.0, 16.0, 30.0
    };

    constexpr std::array<double, 7> luminance = {
        0.001, 0.01, 0.1, 1.0, 10.0, 100.0, 1000.0
    };

    constexpr std::array<double, 7> octave = {
        1.034656203300,
        1.100803634793,
        1.118907577586,
        0.909309548611,
        0.731235033409,
        0.392809414293,
        0.168436527844
    };

    double max_error = 0.0;

    std::cout << std::fixed << std::setprecision(12);

    for (std::size_t i = 0; i < rho.size(); ++i) {
        const double result = IQM::HDRVDP3::compute_ncsf(
            rho[i], luminance[i]
        );

        const double error = std::abs(result - octave[i]);
        max_error = std::max(max_error, error);

        std::cout << "rho=" << rho[i]
                  << " lum=" << luminance[i]
                  << " C++=" << result
                  << " Octave=" << octave[i]
                  << " Error=" << error << '\n';
    }

    std::cout << "MaxAbsErr=" << max_error << '\n';

    std::cout << "\nSPYR Band Frequencies (PPD=30)\n";

constexpr std::array<double, 5> expected_freqs = {
    15.0, 7.5, 3.75, 1.875, 0.9375
};

for (int band = 1; band <= 5; ++band) {
    const double frequency =
        IQM::HDRVDP3::compute_band_frequency(band, 30.0);

    const double error =
        std::abs(frequency - expected_freqs[band - 1]);

    max_error = std::max(max_error, error);

    std::cout << "Band " << band
              << " Frequency=" << frequency
              << " Expected=" << expected_freqs[band - 1]
              << " Error=" << error << '\n';
}

    std::cout << "\nCSF LUT Test (PPD=30)\n";

for (int band = 1; band <= 5; ++band) {
    const double frequency =
        IQM::HDRVDP3::compute_band_frequency(band, 30.0);

    const auto lut = IQM::HDRVDP3::create_csf_lut(frequency);

    for (double lum : {0.001, 0.1, 1.0, 10.0, 100.0}) {
        const double sampled =
            IQM::HDRVDP3::sample_csf_lut(lut, lum);

        const double direct =
            IQM::HDRVDP3::compute_ncsf(frequency, lum);

        const double error = std::abs(sampled - direct);

        std::cout << "Band=" << band
                  << " Lum=" << lum
                  << " LUT=" << sampled
                  << " Direct=" << direct
                  << " Error=" << error << '\n';
    }
}

    return max_error < 1e-9 ? 0 : 1;
}
