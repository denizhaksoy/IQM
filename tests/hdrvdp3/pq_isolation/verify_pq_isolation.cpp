#include <fstream>
#include <IQM/hdrvdp3/pq_single.h>
#include <IQM/hdrvdp3/pq_display_single_gpu.h>
#include <vulkan_instance.h>

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace IQM::HDRVDP3;

void compare(
    const char* name,
    const std::vector<float>& cpu,
    const std::vector<float>& gpu)
{
    if (cpu.size() != gpu.size())
        throw std::runtime_error("Tensor size mismatch");

    uint64_t mismatches = 0;
    uint64_t nonfinite = 0;
    uint32_t maxUlp = 0;
    double maxAbs = 0;
    double maxRel = 0;
    int first = -1;

    for (size_t i = 0; i < cpu.size(); ++i) {
        const float a = cpu[i];
        const float b = gpu[i];

        if (!std::isfinite(a) || !std::isfinite(b)) {
            ++nonfinite;
            continue;
        }

        if (a < 0 || b < 0)
            throw std::runtime_error("Negative PQ luminance");

        const auto ua = std::bit_cast<uint32_t>(a);
        const auto ub = std::bit_cast<uint32_t>(b);

        if (ua != ub) {
            ++mismatches;
            if (first < 0)
                first = static_cast<int>(i);
        }

        maxUlp = std::max(
            maxUlp, ua > ub ? ua - ub : ub - ua);

        const double diff =
            std::abs(double(a) - double(b));

        maxAbs = std::max(maxAbs, diff);

        if (a != 0)
            maxRel = std::max(
                maxRel, diff / std::abs(double(a)));
    }

    std::cout << "\nCASE=" << name
              << "\nCOUNT=" << cpu.size()
              << "\nBITWISE_MISMATCHES=" << mismatches
              << "\nMAX_ULP=" << maxUlp
              << "\nMAX_ABS=" << maxAbs
              << "\nMAX_REL=" << maxRel
              << "\nNONFINITE=" << nonfinite
              << "\nFIRST_DIFFERENT_CODE=" << first
              << "\n";

    if (first >= 0) {
        std::cout << "FIRST_CPU=" << cpu[first]
                  << "\nFIRST_GPU=" << gpu[first]
                  << "\n";
    }

    std::cout << "BITWISE_PARITY="
              << (mismatches == 0 && nonfinite == 0
                  ? "PASS" : "FAIL")
              << "\n";
}

int main(int argc, char** argv)
{
    try {
        if (argc != 3)
            throw std::runtime_error(
                "Usage: verify-pq-isolation original.spv precise.spv");

        std::cout << std::setprecision(17);

        IQM::Bin::VulkanInstance instance(
            true, "NVIDIA RTX A500 Laptop GPU", true);

        std::cout << "DEVICE="
                  << instance.physicalDevice()
                         ->getProperties().deviceName.data()
                  << "\n";

        std::vector<float> encoded(65536);

        for (unsigned i = 0; i < 65536; ++i)
            encoded[i] = normalize_pq_u16_single(
                static_cast<uint16_t>(i));

        const PQDisplaySingle display{
            200.0, 0.005, 2000.0
        };

        const auto cpu =
            pq_to_display_single(encoded, display);

                if (cpu.empty() ||
            std::abs(double(cpu[0]) - 0.3183098861837907) > 1e-7)
            throw std::runtime_error("Invalid full PQ CPU reference");

        std::cout << "CPU_REFERENCE_CODE0=" << cpu[0] << "\n";
        std::cout << "CPU_REFERENCE_CODE65535=" << cpu.back() << "\n";

        const auto original =
            pq_to_display_single_gpu_readback(
                instance, argv[1], encoded, display);

        const auto precise =
            pq_to_display_single_gpu_readback(
                instance, argv[2], encoded, display);


        std::cout << "\n===== REMAINING PQ MISMATCHES =====\n";

        const float e1 = float(1.0 / 78.84375);
        const float e2 = float(1.0 / 0.1593017578125);

        size_t remaining = 0;

        for (size_t i = 0; i < cpu.size(); ++i) {

            if (std::bit_cast<uint32_t>(cpu[i]) ==
                std::bit_cast<uint32_t>(precise[i]))
                continue;

            ++remaining;

            const float x = encoded[i];

            const float t = std::pow(x, e1);

            const float t64 = float(
                std::pow(double(x), double(e1)));

            const float numerator =
                std::max(t - 0.8359375f, 0.0f);

            const float product = 18.6875f * t;

            const float denominator =
                18.8515625f - product;

            const float ratio =
                numerator / denominator;

            const float ratio64 = float(
                double(numerator) / double(denominator));

            const float power = std::pow(ratio, e2);

            const float power64 = float(
                std::pow(double(ratio), double(e2)));

            std::cout
                << "\nPQ_CODE=" << i
                << "\nENCODED=" << x
                << "\nCPU_FINAL=" << cpu[i]
                << "\nGPU_FINAL=" << precise[i]
                << "\nFINAL_ABS_ERROR="
                << std::abs(double(cpu[i]) -
                            double(precise[i]))
                << "\nPOW1_FLOAT=" << t
                << "\nPOW1_DOUBLE_ROUNDED=" << t64
                << "\nPOW1_MATCH="
                << (t == t64 ? "YES" : "NO")
                << "\nNUMERATOR=" << numerator
                << "\nPRODUCT=" << product
                << "\nDENOMINATOR=" << denominator
                << "\nRATIO_FLOAT=" << ratio
                << "\nRATIO_DOUBLE_ROUNDED=" << ratio64
                << "\nRATIO_MATCH="
                << (ratio == ratio64 ? "YES" : "NO")
                << "\nPOW2_FLOAT=" << power
                << "\nPOW2_DOUBLE_ROUNDED=" << power64
                << "\nPOW2_MATCH="
                << (power == power64 ? "YES" : "NO")
                << "\n";
        }

        std::cout
            << "\nREMAINING_MISMATCHES="
            << remaining << "\n";


        const char* officialPath =
            R"(C:\Users\DenizAksoy\OneDrive - Tieto\Desktop\IQM\validation\live_hdr_single_stages_20261101_run01\stages.f32)";

        std::vector<float> officialStages(65536 * 5);

        std::ifstream officialFile(
            officialPath, std::ios::binary);

        officialFile.read(
            reinterpret_cast<char*>(officialStages.data()),
            officialStages.size() * sizeof(float));

        if (!officialFile ||
            officialFile.peek() != std::char_traits<char>::eof())
            throw std::runtime_error(
                "Invalid official reference binary");

        for (float value : officialStages) {
            if (!std::isfinite(value))
                throw std::runtime_error(
                    "Nonfinite official reference value");
        }

        std::vector<float> officialUnit(
            officialStages.begin(),
            officialStages.begin() + 65536);

        std::vector<float> officialFinal(
            officialStages.begin() + 4 * 65536,
            officialStages.end());

        std::cout
            << "\nOFFICIAL_REFERENCE_LOADED=YES"
            << "\nOFFICIAL_REFERENCE_VALUES="
            << officialStages.size()
            << "\n";

        compare(
            "OFFICIAL_VS_CPU_NORMALIZATION",
            officialUnit, encoded);

        compare(
            "OFFICIAL_VS_CURRENT_CPU_FINAL",
            officialFinal, cpu);

        compare(
            "OFFICIAL_VS_GPU_FINAL",
            officialFinal, precise);

        for (size_t i = 0; i < officialFinal.size(); ++i) {
            if (std::bit_cast<uint32_t>(officialFinal[i]) !=
                std::bit_cast<uint32_t>(precise[i])) {

                std::cout
                    << "\nOFFICIAL_MISMATCH_CODE=" << i
                    << "\nOFFICIAL_FINAL=" << officialFinal[i]
                    << "\nCPU_FINAL=" << cpu[i]
                    << "\nGPU_FINAL=" << precise[i]
                    << "\n";
            }
        }

        compare("CPU_VS_GPU_ORIGINAL", cpu, original);
        compare("CPU_VS_GPU_PRECISE", cpu, precise);
        compare("GPU_ORIGINAL_VS_PRECISE", original, precise);

        std::cout << "\nPQ_ISOLATION_COMPLETE=YES\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return 1;
    }
}
