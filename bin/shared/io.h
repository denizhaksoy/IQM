/*
 * Image Quality Metrics
 * Petr Volf - 2025
 */

#ifndef IQM_IO_H
#define IQM_IO_H

#include "stb_image.h"
#include "stb_image_write.h"

#include <stdexcept>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <optional>
#include <cstdint>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace IQM::Bin {
    struct Match {
        std::string testPath;
        std::string refPath;
        std::optional<std::string> outPath;
    };

    struct InputImage {
        unsigned width;
        unsigned height;
        std::vector<unsigned char> data;
    };

    inline InputImage load_image(const std::string &filename) {
        // force all images to always open in RGBA format to prevent issues with separate RGB and RGBA loading
        int x, y, channels;
        unsigned char* data = stbi_load(filename.c_str(), &x, &y, &channels, 4);
        if (data == nullptr) {
            const auto err = stbi_failure_reason();
            const auto msg = std::string("Failed to load image '" + filename + "', reason: " + err);
            throw std::runtime_error(msg);
        }

        std::vector<unsigned char> dataVec(x * y * 4);
        memcpy(dataVec.data(), data, x * y * 4 * sizeof(char));

        stbi_image_free(data);

        return InputImage{
            .width = static_cast<unsigned>(x),
            .height = static_cast<unsigned>(y),
            .data = std::move(dataVec)
        };
    }

    inline std::vector<unsigned char> convertFloatToChar(const std::vector<float>& data) {
        std::vector<unsigned char> result(data.size());

        for (unsigned long i = 0; i < data.size(); ++i) {
            result[i] = static_cast<unsigned char>(std::clamp(data[i], 0.0f, 1.0f) * 255.0f);
        }

        return result;
    }

    inline void save_char_image(const std::string &filename, const std::vector<unsigned char> &imageData, unsigned int width, unsigned int height) {
        auto saveResult = stbi_write_png(filename.c_str(), width, height, 1, imageData.data(), width * sizeof(unsigned char));
        if (saveResult == 0) {
            throw std::runtime_error("Failed to save output image");
        }
    }

    inline void save_float_image(const std::string &filename, const std::vector<float> &imageData, unsigned int width, unsigned int height) {
        const auto converted = convertFloatToChar(imageData);

        auto saveResult = stbi_write_png(filename.c_str(), width, height, 1, converted.data(), width * sizeof(unsigned char));
        if (saveResult == 0) {
            throw std::runtime_error("Failed to save output image");
        }
    }

    inline void save_color_image(const std::string &filename, const std::vector<unsigned char> &imageData, unsigned int width, unsigned int height) {
        auto saveResult = stbi_write_png(filename.c_str(), width, height, 4, imageData.data(), 4 * width * sizeof(unsigned char));
        if (saveResult == 0) {
            throw std::runtime_error("Failed to save output image");
        }
    }

    inline std::vector<float> load_model(const std::string &filename) {
    namespace fs = std::filesystem;

    fs::path executablePath;

#ifdef _WIN32
    std::vector<wchar_t> pathBuffer(32768);
    DWORD length = GetModuleFileNameW(
        nullptr,
        pathBuffer.data(),
        static_cast<DWORD>(pathBuffer.size())
    );

    if (length == 0 || length >= pathBuffer.size()) {
        throw std::runtime_error("Failed to determine executable path");
    }

    executablePath = fs::path(std::wstring(pathBuffer.data(), length));
#else
    executablePath = fs::canonical("/proc/self/exe");
#endif

    fs::path modelPath = executablePath.parent_path() / filename;

    std::ifstream input(modelPath, std::ios::binary);

    if (!input.is_open()) {
        throw std::runtime_error(
            "Failed to open model file: " + modelPath.string()
        );
    }

    input.seekg(0, std::ios::end);
    const auto fileSize = input.tellg();

    if (fileSize < 0 ||
        fileSize % static_cast<std::streamoff>(sizeof(float)) != 0) {
        throw std::runtime_error("Invalid model file size");
    }

    const auto count = static_cast<std::size_t>(
        fileSize / static_cast<std::streamoff>(sizeof(float))
    );

    input.seekg(0, std::ios::beg);

    std::vector<float> buffer(count);

    if (!buffer.empty()) {
        input.read(
            reinterpret_cast<char*>(buffer.data()),
            static_cast<std::streamsize>(fileSize)
        );

        if (!input) {
            throw std::runtime_error("Failed to read model file");
        }
    }

    return buffer;
}
}

#endif //IQM_IO_H
