#pragma once

#ifdef DEBUG
#define DEB_BLOCK
#define DEB_FRIEND friend class ::DebugHelper;
#include <format>      
#include <string_view> 
#include <type_traits> 
#include <cstdio>
template<typename... Args>
void DebugMessageImpl(Args&&... args) {
    auto format_and_write = [](const auto& arg) {
        // Se l'argomento è convertibile a const char* (come FormatBuffer o stringhe raw)
        if constexpr (std::is_convertible_v<decltype(arg), const char*>) {
            const char* str = arg;
            std::fwrite(str, 1, std::char_traits<char>::length(str), stdout);
        }
        // Se l'argomento ha direttamente un metodo .str() (es. Vec o Mat passati direttamente)
        else if constexpr (requires { arg.str(); }) {
            const char* str = arg.str();
            std::fwrite(str, 1, std::char_traits<char>::length(str), stdout);
        }
        // Altrimenti usa std::format standard (per int, float, ecc.)
        else {
            auto formatted = std::format("{}", arg);
            std::fwrite(formatted.data(), 1, formatted.size(), stdout);
        }
        };

    (format_and_write(args), ...);
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