#pragma once

// C++ HEADERS
#include <iostream>
#include <string>
#include <stdint.h>
#include <assert.h>
#include <math.h>

#include <vector>
#include <map>
#include <unordered_map>
#include <array>
#include <algorithm>
#include <list>
#include <fstream>
#include <queue>
#include <stack>

#include <chrono>
#include <sstream>
#include <numeric>
#include <random>
#include <mutex>
#include <thread>
#include <filesystem>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#ifdef _DEBUG
#define ASSERT(condition, message) (assert(condition &&message))
#define LOGE(err)                                   \
    {                                               \
        std::cerr << "ERROR::" << err << std::endl; \
        assert(0);                                  \
    }

#define LOG(msg)                                  \
    {                                             \
        std::cout << "LOG::" << msg << std::endl; \
    }

#define LOGW(msg)                                     \
    {                                                 \
        std::cout << "WARNING::" << msg << std::endl; \
    }
#else
#define ASSERT(condition, message)
#define LOGE(err)
#define LOG(msg)
#define LOGW(msg)
#endif
template <typename T>
inline bool HasFlag(T val, T flag) {
    return (val & flag) == flag;
}

// Godot String::hash()
inline uint32_t DJB2Hash(const std::string &message) {
    /* simple djb2 hashing */
    uint32_t hashv = 5381;
    for (auto c : message) {
        hashv = ((hashv << 3) + hashv) + c;
    }
    return hashv;
}
