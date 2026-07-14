#pragma once
#include <cstdint>
#include <array>
#include <span>
#include <type_traits>
#include <memory.h>
#include <concepts>
#include <utility>
#include <cmath>
#include <limits>
#include <algorithm>

template <uint32_t dim, typename T>
struct Vec;
template <uint32_t Rows, uint32_t Cols, typename T>
struct Mat;

namespace math::cond {

    // Tratto base per identificare Vec e Mat
    template <typename T>
    struct is_algebraic : std::false_type {};

    template <uint32_t dim, typename T>
    struct is_algebraic<Vec<dim, T>> : std::true_type {};

    template <uint32_t Rows, uint32_t Cols, typename T>
    struct is_algebraic<Mat<Rows, Cols, T>> : std::true_type {};

    // Il concetto finale
    template <typename T>
    concept AlgebraicType = is_algebraic<T>::value;

    //Struct per ottenere il metadato _depth
    template <typename T, typename = void>
    struct get_depth {
        enum { value = 0 };
    };

    // Se il tipo ha l'enum '_depth', estrai il valore dell'enum
    template <AlgebraicType T>
    struct get_depth<T, void> {
        enum { value = T::_depth };
    };

    template <typename T>
    inline constexpr uint32_t _depth_v = get_depth<T>::value;
}

namespace co = math::cond;

template <uint32_t dim = 3, typename T = float>
struct Vec {
    std::array<T, dim> raw;
    enum { _depth = co::_depth_v<T> + 1};


    //COSTRUTTORI:

    //Zeri
    constexpr Vec() : raw{} {}

    //Da array generica (dim DEVE essere uguale a size)
    constexpr Vec(std::span<const T> source) {
        std::copy(source.begin(), source.end(), raw.begin());
    }

    //Da array generica con dim non per forza uguale
    //Il vec viene riempito partendo dallo 0, i restanti sono impostati a 0 (higher) o tagliati (lower)
    //diff scrive la differenza tra size, source_size - dim
    constexpr Vec(std::span<const T> source, int32_t& diff) : raw{} {
        const std::size_t src_size = source.size();
        diff = static_cast<int32_t>(src_size) - static_cast<int32_t>(dim);
        const std::size_t copy_size = std::min(dim, src_size);

        if constexpr (std::is_trivially_copyable_v<T>) {
            ::memcpy(raw.data(), source.data(), copy_size * sizeof(T));
        }
        else {
            const T* src_ptr = source.data();
            for (size_t i = 0; i < copy_size; ++i) {
                raw[i] = src_ptr[i];
            }
        }
    }

    // Da initializer list, dim DEVE essere uguale o non compila nemmeno
    template <typename... Args>
        requires (sizeof...(Args) == dim) && (std::is_convertible_v<Args, T> && ...)
    constexpr Vec(Args&&... args) : raw{ static_cast<T>(std::forward<Args>(args))... } {}

    //Uniforme
    constexpr Vec(const T& init) {
        raw.fill(init);
    }

   
    //Constexpr base canonica
    template <uint32_t index>
    static constexpr Vec Canon(T val = 1)
        requires std::is_arithmetic_v<T>&& std::convertible_to<int, T> {
        static_assert(index < dim, "Errore: canon index dev'essere minore di dim");
        Vec v = Vec();
        v[index] = val;
        return v;
    }
    //Runtime canonica
    template <bool check = true>
    static Vec Canon(uint32_t index, T val = 1)
        requires std::is_arithmetic_v<T>&& std::convertible_to<int, T> {

        if constexpr (check) {
            if (index >= dim)
                return Vec();
            //TODO: aggiungere una lista globale di pointer/metadati di variabili corrotte, per il runtime fixing
        }
        else {
            Vec v = Vec(); // Inizializzato a zero
            v[index] = val;
            return v;
        }
    }

    constexpr auto operator-() const requires requires(T t) { -t; } {
        using ResultType = decltype(-std::declval<T>());
        Vec<dim, ResultType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = -raw[i];
        }
        return result;
    }

    template <typename U>
        requires requires(T t, U u) { t + u; }
    constexpr Vec<dim, std::common_type_t<T, U>> operator+(const Vec<dim, U>& other) const {
        using ReturnType = std::common_type_t<T, U>;
        Vec<dim, ReturnType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = static_cast<ReturnType>(raw[i] + other.raw[i]);
        }
        return result;
    }

    template <typename U>
        requires requires(T t, U u) { t - u; }
    constexpr auto operator-(const Vec<dim, U>& other) const {
        using ResultType = decltype(std::declval<T>() - std::declval<U>());
        Vec<dim, ResultType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = raw[i] - other.raw[i];
        }
        return result;
    }

    //Prodotto scalare vec*vec
    // 1. PRODOTTO SCALARE (Vettore * Vettore dello stesso livello)
    template <typename U>
        requires (_depth == Vec<dim, U>::_depth) && requires(T t, U u) {
        t* u;
        requires requires(decltype(t * u) res) { res + res; };
    }
    constexpr auto operator*(const Vec<dim, U>& other) const {
        using ProductType = decltype(std::declval<T>()* std::declval<U>());
        ProductType acc = raw[0] * other.raw[0];
        for (size_t i = 1; i < dim; ++i) {
            acc = acc + (raw[i] * other.raw[i]);
        }
        return acc;
    }

    // 2. PRODOTTO PER SCALARE (Vincolato rigorosamente al livello adiacente o a depth 0)
    template <typename U>
        requires ((co::_depth_v<U> == _depth - 1) || (co::_depth_v<U> == 0)) && requires(T t, U u) { t* u; }
    constexpr auto operator*(const U& scalar) const {
        using ResultType = decltype(std::declval<T>()* std::declval<U>());
        Vec<dim, ResultType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = raw[i] * scalar;
        }
        return result;
    }

    // Prototipo dell'operatore Vec * Mat (la definizione è spostata sotto Mat)
    template <uint32_t NewCols, typename U>
        requires requires(T t, U u) {
        t* u;
        requires requires(decltype(t * u) res) { res + res; };
    }
    constexpr auto operator*(const Mat<dim, NewCols, U>& mat) const;

    template <uint32_t Dim2, typename U>
        requires requires(T t, U u) { t* u; }
    constexpr auto operator^(const Vec<Dim2, U>& other) const {
        using ProductType = decltype(std::declval<T>()* std::declval<U>());
        Mat<dim, Dim2, ProductType> result;
        for (uint32_t r = 0; r < dim; ++r) {
            for (uint32_t c = 0; c < Dim2; ++c) {
                result.raw[r * Dim2 + c] = raw[r] * other.raw[c];
            }
        }
        return result;
    }

    constexpr auto operator^(uint32_t exp) const {
        return (*this) * (*this);
    }

    constexpr auto norm() const requires std::is_arithmetic_v<T> {
        using ScalarType = decltype((*this)* (*this));
        using FPUType = std::conditional_t<(sizeof(ScalarType) <= 4), float, double>;
        return std::sqrt(static_cast<FPUType>((*this) * (*this)));
    }

    constexpr auto normalized() const requires std::is_arithmetic_v<T> {
        using FPUType = decltype(norm());
        const FPUType n = norm();
        if constexpr (std::numeric_limits<FPUType>::has_infinity) {
            if (n < std::numeric_limits<FPUType>::epsilon()) {
                return Vec<dim, FPUType>{};
            }
        }
        const FPUType inv_n = static_cast<FPUType>(1) / n;
        Vec<dim, FPUType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = static_cast<FPUType>(raw[i]) * inv_n;
        }
        return result;
    }

    template <typename U>
        requires (dim == 3) && requires(T t, U u) { t* u; t - u; }
    constexpr auto operator%(const Vec<3, U>& other) const {
        using ResultType = decltype(std::declval<T>()* std::declval<U>());
        return Vec<3, ResultType>{
            raw[1] * other.raw[2] - raw[2] * other.raw[1],
                raw[2] * other.raw[0] - raw[0] * other.raw[2],
                raw[0] * other.raw[1] - raw[1] * other.raw[0]
        };
    }

    template <typename U>
        requires requires(T t, U u) { t* u; }
    constexpr auto operator&(const Vec<dim, U>& other) const {
        using ResultType = decltype(std::declval<T>()* std::declval<U>());
        Vec<dim, ResultType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = raw[i] * other.raw[i];
        }
        return result;
    }

    template <typename NewType>
        requires requires(T t) { static_cast<NewType>(t); }
    constexpr auto as() const {
        Vec<dim, NewType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = static_cast<NewType>(raw[i]);
        }
        return result;
    }

    template <typename Scalar>
        requires requires(Scalar s, T t) { s* t; }
    friend constexpr auto operator*(const Scalar& scalar, const Vec& vec) {
        using ResultType = decltype(std::declval<Scalar>()* std::declval<T>());
        Vec<dim, ResultType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = scalar * vec.raw[i];
        }
        return result;
    }
};

template <uint32_t Rows, uint32_t Cols, typename T>
struct Mat {
    std::array<T, Rows* Cols> raw;

    enum { _depth = co::_depth_v<T> +1 };


    //COSTRUTTORI

    //Zeri
    constexpr Mat() : raw{} {}

    //Da raw array, a size giusta
    constexpr Mat(std::span<const T> source) {
        const std::size_t target_size = Rows * Cols;
        const std::size_t src_size = source.size();
        const std::size_t copy_size = std::min(target_size, src_size);

        if constexpr (std::is_trivially_copyable_v<T>) {
            ::memcpy(raw.data(), source.data(), copy_size * sizeof(T));
        }
        else {
            const T* src_ptr = source.data();
            for (std::size_t i = 0; i < copy_size; ++i) {
                raw[i] = src_ptr[i];
            }
        }
    }
    //Vedere costruttori vector
    constexpr Mat(std::span<const T> source, int32_t& diff) : raw{} {
        const std::size_t src_size = source.size();
        const std::size_t target_size = Rows * Cols;
        diff = static_cast<int32_t>(src_size) - static_cast<int32_t>(target_size);
        const std::size_t copy_size = std::min(target_size, src_size);

        if constexpr (std::is_trivially_copyable_v<T>) {
            ::memcpy(raw.data(), source.data(), copy_size * sizeof(T));
        }
        else {
            const T* src_ptr = source.data();
            for (std::size_t i = 0; i < copy_size; ++i) {
                raw[i] = src_ptr[i];
            }
        }
    }


    constexpr Mat(const T& init, bool diagonal_only = false) : raw{} {
        if (diagonal_only) {
            const uint32_t min_dim = std::min(Rows, Cols);
            for (uint32_t i = 0; i < min_dim; ++i) {
                raw[i * Cols + i] = init;
            }
        }
        else {
            raw.fill(init);
        }
    }


    constexpr auto t() const {
        Mat<Cols, Rows, T> result;
        for (uint32_t r = 0; r < Rows; ++r) {
            for (uint32_t c = 0; c < Cols; ++c) {
                result.raw[c * Rows + r] = raw[r * Cols + c];
            }
        }
        return result;
    }

    constexpr auto operator-() const requires requires(T t) { -t; } {
        using ResultType = decltype(-std::declval<T>());
        Mat<Rows, Cols, ResultType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = -raw[i];
        }
        return result;
    }

    template <typename U>
        requires requires(T t, U u) { t + u; }
    constexpr auto operator+(const Mat<Rows, Cols, U>& other) const {
        using ResultType = decltype(std::declval<T>() + std::declval<U>());
        Mat<Rows, Cols, ResultType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = raw[i] + other.raw[i];
        }
        return result;
    }

    template <typename U>
        requires requires(T t, U u) { t - u; }
    constexpr auto operator-(const Mat<Rows, Cols, U>& other) const {
        using ResultType = decltype(std::declval<T>() - std::declval<U>());
        Mat<Rows, Cols, ResultType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = raw[i] - other.raw[i];
        }
        return result;
    }

    // 1. PRODOTTO MATRICE * MATRICE (Stesso livello algebrico)
    template <uint32_t NewCols, typename U>
        requires (_depth == Mat<Cols, NewCols, U>::_depth) && requires(T t, U u) {
        t* u;
        requires requires(decltype(t * u) res) { res + res; };
    }
    constexpr auto operator*(const Mat<Cols, NewCols, U>& other) const {
        using ProductType = decltype(std::declval<T>()* std::declval<U>());
        Mat<Rows, NewCols, ProductType> result;

        for (uint32_t r = 0; r < Rows; ++r) {
            const T& a_val_0 = raw[r * Cols];
            for (uint32_t c = 0; c < NewCols; ++c) {
                result.raw[r * NewCols + c] = a_val_0 * other.raw[c];
            }
            for (uint32_t k = 1; k < Cols; ++k) {
                const T& a_val = raw[r * Cols + k];
                for (uint32_t c = 0; c < NewCols; ++c) {
                    result.raw[r * NewCols + c] = result.raw[r * NewCols + c] + (a_val * other.raw[k * NewCols + c]);
                }
            }
        }
        return result;
    }

    // 2. PRODOTTO MATRICE * VETTORE (Stesso livello algebrico)
    template <typename U>
        requires (_depth == Vec<Cols, U>::_depth) && requires(T t, U u) {
        t* u;
        requires requires(decltype(t * u) res) { res + res; };
    }
    constexpr auto operator*(const Vec<Cols, U>& vec) const {
        using ProductType = decltype(std::declval<T>()* std::declval<U>());
        Vec<Rows, ProductType> result;

        for (uint32_t r = 0; r < Rows; ++r) {
            ProductType acc = raw[r * Cols] * vec.raw[0];
            for (uint32_t c = 1; c < Cols; ++c) {
                acc = acc + (raw[r * Cols + c] * vec.raw[c]);
            }
            result.raw[r] = acc;
        }
        return result;
    }

    // 3. PRODOTTO MATRICE * SCALARE (Destro)
    template <typename U>
        requires ((co::_depth_v<U> == _depth - 1) || (co::_depth_v<U> == 0)) && requires(T t, U u) { t* u; }
    constexpr auto operator*(const U& scalar) const {
        using ResultType = decltype(std::declval<T>()* std::declval<U>());
        Mat<Rows, Cols, ResultType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = raw[i] * scalar;
        }
        return result;
    }

    template <typename NewType>
        requires requires(T t) { static_cast<NewType>(t); }
    constexpr auto as() const {
        Mat<Rows, Cols, NewType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = static_cast<NewType>(raw[i]);
        }
        return result;
    }

    template <typename Scalar>
        requires ((co::_depth_v<Scalar> == _depth - 1) || (co::_depth_v<Scalar> == 0)) && requires(Scalar s, T t) { s* t; }
    friend constexpr auto operator*(const Scalar& scalar, const Mat& mat) {
        using ResultType = decltype(std::declval<Scalar>()* std::declval<T>());
        Mat<Rows, Cols, ResultType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = scalar * mat.raw[i];
        }
        return result;
    }

    // Accesso in lettura/scrittura (non-const)
    T& operator()(uint32_t row, uint32_t col) {
        return raw[row * Cols + col]; 
    }

    // Accesso in sola lettura (const)
    const T& operator()(uint32_t row, uint32_t col) const {
        return raw[row * Cols + col];
    }

    //Costruttori statici
    static constexpr const Mat<Rows, Cols, T> Identity() {
        static_assert(std::is_convertible_v<int, T>, "Errore, identità non convertibile");
        return Mat<Rows, Cols, T>(1, true);
    }
};

// --- DEFINIZIONE ESTERNA DI VEC * MAT ---
// Risolve il problema del tipo incompleto (ora Mat è interamente definita)
template <uint32_t dim, typename T>
template <uint32_t NewCols, typename U>
    requires requires(T t, U u) {
    t* u;
    requires requires(decltype(t * u) res) { res + res; };
}
constexpr auto Vec<dim, T>::operator*(const Mat<dim, NewCols, U>& mat) const {
    using ProductType = decltype(std::declval<T>()* std::declval<U>());
    Vec<NewCols, ProductType> result;

    const T& v_val_0 = raw[0];
    for (uint32_t c = 0; c < NewCols; ++c) {
        result.raw[c] = v_val_0 * mat.raw[c];
    }

    for (uint32_t r = 1; r < dim; ++r) {
        const T& v_val = raw[r];
        for (uint32_t c = 0; c < NewCols; ++c) {
            result.raw[c] = result.raw[c] + (v_val * mat.raw[r * NewCols + c]);
        }
    }
    return result;
}

//Tipi utili
using Vec3f = Vec<3, float>;
using Vec4f = Vec<4, float>;
using Vec3u = Vec<3, uint32_t>;
using Vec4u = Vec<4, uint32_t>;
using Vec3i = Vec<3, int32_t>;
using Vec4i = Vec<4, int32_t>;
using Vec3u16 = Vec<3, uint16_t>;
using Vec4u16 = Vec<4, uint16_t>;
using Vec3i16 = Vec<3, int16_t>;
using Vec4i16 = Vec<4, int16_t>;

template <uint32_t dim, typename T>
using MatQ = Mat<dim, dim, T>;

using Mat4f = MatQ<4, float>;
using Mat3f = MatQ<3, float>;