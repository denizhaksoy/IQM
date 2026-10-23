/*
 * Image Quality Metrics
 * Petr Volf - 2025
 */

#include "args.h"
#include <cstring>
#include <stdexcept>
#include <cmath>
#include <unordered_set>

IQM::Bin::Args::Args(const unsigned argc, const char *argv[]) {
    bool parsedMethod = false;
    bool parsedInput = false;
    bool parsedReference = false;
    std::unordered_set<std::string> hdrOptions;

    unsigned i = 1;
    while (i < argc)  {
        const std::string key(argv[i]);
        if(key.rfind("--hdrvdp3-",0)==0) {
            if(key!="--hdrvdp3-runtime"&&key!="--hdrvdp3-spectral-data"&&key!="--hdrvdp3-export"&&key!="--hdrvdp3-ppd"&&key!="--hdrvdp3-age")
                throw std::runtime_error("Unknown HDRVDP3 option: "+key);
            if(!hdrOptions.insert(key).second)throw std::runtime_error("Duplicate option: "+key);
            if(i+1>=argc||!argv[i+1][0]||std::string(argv[i+1]).rfind("--",0)==0)
                throw std::runtime_error("Missing value for "+key);
            if(!hdrvdp3Quality)hdrvdp3Quality.emplace();
            const std::string value(argv[i+1]);
            if(key=="--hdrvdp3-runtime")hdrvdp3Quality->runtimeDirectory=value;
            else if(key=="--hdrvdp3-spectral-data")hdrvdp3Quality->spectralDirectory=value;
            else if(key=="--hdrvdp3-export")hdrvdp3Quality->exportDirectory=value;
            else {
                double number;size_t consumed=0;
                try{number=std::stod(value,&consumed);}catch(const std::exception&){throw std::runtime_error("Invalid number for "+key);}
                if(consumed!=value.size()||!std::isfinite(number))throw std::runtime_error("Invalid number for "+key);
                if(key=="--hdrvdp3-ppd")hdrvdp3Quality->pixelsPerDegree=number;
                else hdrvdp3Quality->observerAge=number;
            }
            i+=2;continue;
        }
        if (strcmp(argv[i], "--method") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing method argument");
            }
            if (strcmp(argv[i + 1], "SSIM") == 0) {
                this->method = Method::SSIM;
            } else if (strcmp(argv[i + 1], "CW_SSIM_CPU") == 0) {
                this->method = Method::CW_SSIM_CPU;
            } else if (strcmp(argv[i + 1], "SVD") == 0) {
                this->method = Method::SVD;
            } else if (strcmp(argv[i + 1], "FSIM") == 0) {
                this->method = Method::FSIM;
            } else if (strcmp(argv[i + 1], "FLIP") == 0) {
                this->method = Method::FLIP;
            } else if (strcmp(argv[i + 1], "PSNR") == 0) {
                this->method = Method::PSNR;
            } else if (strcmp(argv[i + 1], "LPIPS") == 0) {
                this->method = Method::LPIPS;
            } else if (strcmp(argv[i + 1], "HDRVDP3") == 0) {
                this->method = Method::HDRVDP3;
            } else if (strcmp(argv[i + 1], "LOCAL_ADAPT") == 0) {
                this->method = Method::LOCAL_ADAPT;
            } else {
                throw std::runtime_error("Unknown method");
            }
            parsedMethod = true;
            i+=2;
            continue;
        }

        if (strcmp(argv[i], "--input") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing input argument");
            }

            this->inputPath = std::string(argv[i + 1]);
            parsedInput = true;
            i+=2;
            continue;
        }

        if (strcmp(argv[i], "--ref") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing reference argument");
            }

            this->refPath = std::string(argv[i + 1]);
            parsedReference = true;
            i+=2;
            continue;
        }

        if (strcmp(argv[i], "--output") == 0) {
            if (i + 1 >= argc) {
                throw std::runtime_error("Missing output argument");
            }

            this->outputPath = std::string(argv[i + 1]);
            i+=2;
            continue;
        }

        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            this->verbose = true;
            i += 1;
            continue;
        }
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            this->printHelp = true;
            return;
        }
        if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--colorize") == 0) {
            this->colorize = true;
            i += 1;
            continue;
        }
        if (strcmp(argv[i], "--enable-validation") == 0) {
            this->enableValidation = true;
            i += 1;
            continue;
        }

        if (i + 1 >= argc) {
            throw std::runtime_error("Missing value argument");
        }
        this->options.emplace(std::string(argv[i]), std::string(argv[i + 1]));
        i+=2;
    }

    if (!parsedMethod) {
        throw std::runtime_error("missing method");
    }
    if(hdrvdp3Quality&&method!=Method::HDRVDP3)throw std::runtime_error("HDRVDP3 options require --method HDRVDP3");
    if(method==Method::HDRVDP3&&(parsedInput||parsedReference||hdrvdp3Quality)) {
        if(!parsedInput||!parsedReference||inputPath.empty()||refPath.empty())throw std::runtime_error("HDRVDP3 luminance input requires both --input and --ref PFM files");
        if(!hdrvdp3Quality||hdrvdp3Quality->runtimeDirectory.empty())throw std::runtime_error("HDRVDP3 luminance input requires --hdrvdp3-runtime");
        if(outputPath)throw std::runtime_error("HDRVDP3 quality uses --hdrvdp3-export, not --output");
        const auto& quality=*hdrvdp3Quality;
        if(quality.pixelsPerDegree!=30&&quality.pixelsPerDegree!=60)throw std::runtime_error("Unvalidated HDRVDP3 pixels per degree");
        if(quality.observerAge!=24&&quality.observerAge!=60&&quality.observerAge!=60.001)throw std::runtime_error("Unvalidated HDRVDP3 observer age");
        if(quality.observerAge!=24&&!quality.spectralDirectory)throw std::runtime_error("Nondefault HDRVDP3 age requires --hdrvdp3-spectral-data");
    }
    // LOCAL_ADAPT and HDRVDP3 use built-in test input and reference, so input/ref are optional
    if (this->method != Method::LOCAL_ADAPT && this->method != Method::HDRVDP3) {
        if (!parsedInput) {
            throw std::runtime_error("missing input");
        }
        if (!parsedReference) {
            throw std::runtime_error("missing reference");
        }
    }
}

