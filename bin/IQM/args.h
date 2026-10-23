/*
 * Image Quality Metrics
 * Petr Volf - 2025
 */

#ifndef IQM_ARGS_H
#define IQM_ARGS_H

#include "../shared/methods.h"
#include <optional>
#include <unordered_map>

namespace IQM::Bin {
    struct HDRVDP3QualityArgs {
        std::string runtimeDirectory;
        std::optional<std::string> spectralDirectory;
        std::optional<std::string> exportDirectory;
        double pixelsPerDegree = 30;
        double observerAge = 24;
    };
    class Args {
    public:
        Args(unsigned argc, const char* argv[]);
        Method method;
        std::string inputPath;
        std::string refPath;
        std::optional<std::string> outputPath;
        std::unordered_map<std::string, std::string> options;
        std::optional<HDRVDP3QualityArgs> hdrvdp3Quality;
        bool colorize = false;
        bool verbose = false;
        bool printHelp = false;
        bool enableValidation = false;
    };
}

#endif //IQM_ARGS_H
