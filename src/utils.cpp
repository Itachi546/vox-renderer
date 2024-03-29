#include "utils.h"

#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox/ogt_vox.h"

#include <fstream>
#include <assert.h>
#include <iostream>

namespace utils {
    std::optional<std::string> ReadFile(const char *filename) {
        std::ifstream inFile(filename);
        if (!inFile)
            return {};
        return std::string{
            (std::istreambuf_iterator<char>(inFile)),
            std::istreambuf_iterator<char>()};
    }
} // namespace utils
