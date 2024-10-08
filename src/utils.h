#pragma once

#include <optional>
#include <string>
#include <fstream>

namespace utils {

    std::optional<std::string> ReadFile(const std::string &filename, std::ios::openmode mode = std::ios::in);
    inline std::string GetFileExtension(const std::string &filename) {
        return filename.substr(filename.find_last_of('.'));
    }

} // namespace utils
