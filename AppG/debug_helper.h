#pragma once

#ifdef DEBUG
#define DEB_BLOCK
#define DEB_FRIEND friend class ::DebugHelper;
#include <cstdio>
#include <format>
template<typename... Args>
void DebugMessageImpl(Args&&... args) {
    // Sfrutta std::format per convertire e unire tutto in una stringa sola
    // C++20 permette di formattare tipi base direttamente così
    ((std::fwrite(std::format("{}", std::forward<Args>(args)).data(), 1, std::format("{}", args).size(), stdout)), ...);
    std::putchar('\n');
}
#define DebugMessage(...) DebugMessageImpl(__VA_ARGS__)
#else 
#define DEB_BLOCK if constexpr (true) return;
#define DEB_FRIEND
#define DebugMessage(...) (void(0))
#endif // DEBUG


class DebugHelper {

};