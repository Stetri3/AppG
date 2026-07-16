#pragma once
#include <type_traits>
#include <cstdint>
#include <array>
#include <charconv>
#include <concepts>

// Forward declarations delle tue strutture
template <uint32_t dim, typename T>
struct Vec;

template <uint32_t Rows, uint32_t Cols, typename T>
struct Mat;

template <uint32_t dim, typename T>
struct MatQ;

namespace math::utils {

    // Helper per verificare se un tipo ha un metodo .str()
    template <typename T>
    concept HasStrMethod = requires(const T & x) {
        { x.str() } -> std::convertible_to<const char*>;
    };

    // 1. Caso base: Tipi aritmetici primitivi (float, double, int, ecc.)
    template <typename T, size_t BufSize>
    constexpr char* format_element(char* ptr, char* const end, const T& val)
        requires std::is_arithmetic_v<T>
    {
        auto [p, ec] = std::to_chars(ptr, end, val);
        return (ec == std::errc{}) ? p : ptr;
    }

    // 2. Caso ricorsivo: Tipi complessi che implementano .str() (altri Vec, Mat, ecc.)
    template <typename T, size_t BufSize>
    constexpr char* format_element(char* ptr, char* const end, const T& val)
        requires (!std::is_arithmetic_v<T>&& HasStrMethod<T>)
    {
        const char* inner_str = val.str();
        while (ptr < end && *inner_str != '\0') {
            *ptr++ = *inner_str++;
        }
        return ptr;
    }

    // 3. Fallback per tipi non stampabili
    template <typename T, size_t BufSize>
    constexpr char* format_element(char* ptr, char* const end, const T& val)
        requires (!std::is_arithmetic_v<T> && !HasStrMethod<T>)
    {
        if (ptr + 1 < end) {
            *ptr++ = '?';
        }
        return ptr;
    }

    // --- FUNZIONE DI FORMATTAZIONE PER VEC ---
    template <uint32_t dim, typename T>
    struct VecFormatBuffer {
        // Allocazione statica basata su sizeof(T) * 8 + overhead minimo
        std::array<char, dim * sizeof(T) * 8 + 32> data{};
        constexpr operator const char* () const noexcept { return data.data(); }
    };

    template <uint32_t dim, typename T>
    constexpr auto to_string(const ::Vec<dim, T>& vec) noexcept {
        VecFormatBuffer<dim, T> buf;
        char* ptr = buf.data.data();
        char* const end = ptr + buf.data.size() - 1;

        *ptr++ = '[';
        for (size_t i = 0; i < dim; ++i) {
            if (i > 0) {
                if (ptr + 2 < end) {
                    *ptr++ = ',';
                    *ptr++ = ' ';
                }
            }
            ptr = format_element<T, buf.data.size()>(ptr, end, vec.raw[i]);
        }

        if (ptr < end) {
            *ptr++ = ']';
        }
        *ptr = '\0';
        return buf;
    }

    // --- FUNZIONE DI FORMATTAZIONE PER MAT / MATQ ---
    template <uint32_t Rows, uint32_t Cols, typename T>
    struct MatFormatBuffer {
        // Allocazione basata sull'intera griglia di elementi
        std::array<char, Rows* Cols * sizeof(T) * 8 + Rows * 4 + 32> data{};
        constexpr operator const char* () const noexcept { return data.data(); }
    };

    template <uint32_t Rows, uint32_t Cols, typename T>
    constexpr auto to_string(const ::Mat<Rows, Cols, T>& mat) noexcept {
        MatFormatBuffer<Rows, Cols, T> buf;
        char* ptr = buf.data.data();
        char* const end = ptr + buf.data.size() - 1;

        *ptr++ = '[';
        for (uint32_t r = 0; r < Rows; ++r) {
            if (r > 0) {
                if (ptr + 2 < end) {
                    *ptr++ = ';';
                    *ptr++ = ' ';
                }
            }
            *ptr++ = '[';
            for (uint32_t c = 0; c < Cols; ++c) {
                if (c > 0) {
                    if (ptr + 2 < end) {
                        *ptr++ = ',';
                        *ptr++ = ' ';
                    }
                }
                ptr = format_element<T, buf.data.size()>(ptr, end, mat.raw[r * Cols + c]);
            }
            if (ptr < end) {
                *ptr++ = ']';
            }
        }
        if (ptr < end) {
            *ptr++ = ']';
        }
        *ptr = '\0';
        return buf;
    }
}