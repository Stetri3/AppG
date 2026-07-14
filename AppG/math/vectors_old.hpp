#pragma once
#include <cstdint>
#include <array>
#include <span>
#include <type_traits>
#include <memory.h>
#include <concepts>
#include <utility>



//GUIDA DI OPERATORI:
//+, - : standard
//Vec* X: Prodotto scalare (X = vec), Prodotto per matrice (a sinistra) (X = mat), Prodotto per scalare (X = T)
//


template <uint32_t Rows, uint32_t Cols, typename T>
struct Mat;

template <uint32_t dim = 3, typename T = float>
struct Vec {
    std::array<T, dim> raw;
    constexpr Vec() : raw{} {}
    constexpr Vec(std::span<const T> source) {
        std::copy(source.begin(), source.end(), raw.begin());
    }
    constexpr Vec(std::span<const T> source, int32_t& diff) : raw{} {
        const std::size_t src_size = source.size();
        diff = static_cast<int32_t>(src_size) - static_cast<int32_t>(dim);

        // 2. std::min compiles down to a hardware conditional move (cmov) instruction
        const std::size_t copy_size = std::min(dim, src_size);

        if constexpr (std::is_trivially_copyable_v<T>)
        {
            // MSVC lo trasforma istantaneamente in un intrinseco assembly
            ::memcpy(raw.data(), source.data(), copy_size * sizeof(T));
        }
        else
        {
            // Fallback per tipi non-POD
            const T* src_ptr = source.data();
            for (size_t i = 0; i < copy_size; ++i)
            {
                raw[i] = src_ptr[i];
            }
        }
    }
    constexpr Vec(const T& init) {
        raw.fill(init);
    }

    //Overloading per operazioni algebriche
    //-vec
    constexpr auto operator-() const requires requires(T t) { -t; } {
        using ResultType = decltype(-std::declval<T>());
        Vec<dim, ResultType> result;

        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = -raw[i];
        }
        return result;
    }

    //Vec + Vec
    template <typename U>
        requires requires(T t, U u) { t + u; } // Verifica che l'addizione sia legale
    constexpr Vec<dim, std::common_type_t<T, U>> operator+(const Vec<dim, U>& other) const {
        // Il tipo di ritorno calcolato a compile-time
        using ReturnType = std::common_type_t<T, U>;
        Vec<dim, ReturnType> result;

        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = static_cast<ReturnType>(raw[i] + other.raw[i]);
        }
        return result;
    }
    //Vec - vec
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
    //per vec*vec
    template <typename U>
        requires requires(T t, U u) {
        // 1. Verifica che la moltiplicazione sia valida
        t* u;
        // 2. Verifica che il risultato si possa sommare a se stesso
            requires requires(decltype(t * u) res) { res + res; };
    }
    constexpr auto operator*(const Vec<dim, U>& other) const {
        // Deduciamo il tipo esatto dell'accumulatore dall'espressione (T * U)
        using ProductType = decltype(std::declval<T>()* std::declval<U>());

        // Inizializziamo il primo elemento per evitare problemi con tipi custom senza costruttore di default a 0
        ProductType acc = raw[0] * other.raw[0];

        for (size_t i = 1; i < dim; ++i) {
            acc = acc + (raw[i] * other.raw[i]);
        }

        return acc;
    }

    //Per vec* scalare:
    template <typename U>
        requires requires(T t, U u) { t* u; } // Vincolo: T deve essere moltiplicabile per U
    constexpr auto operator*(const U& scalar) const {
        using ResultType = decltype(std::declval<T>()* std::declval<U>());
        Vec<dim, ResultType> result;

        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = raw[i] * scalar;
        }
        return result;
    }

    // Prodotto Vettore-Matrice a sinistra: v (1 x dim) * mat (dim x NewCols) -> Vec<NewCols>
    template <uint32_t NewCols, typename U>
        requires requires(T t, U u) {
        t* u;
        requires requires(decltype(t * u) res) { res + res; };
    }
    constexpr auto operator*(const Mat<dim, NewCols, U>& mat) const {
        using ProductType = decltype(std::declval<T>()* std::declval<U>());
        Vec<NewCols, ProductType> result;

        // Inizializziamo la prima riga (r = 0) usando direttamente l'assegnamento
        const T& v_val_0 = raw[0];
        for (uint32_t c = 0; c < NewCols; ++c) {
            result.raw[c] = v_val_0 * mat.raw[c];
        }

        // Accumuliamo le righe successive (r = 1 ... dim-1)
        for (uint32_t r = 1; r < dim; ++r) {
            const T& v_val = raw[r];
            for (uint32_t c = 0; c < NewCols; ++c) {
                result.raw[c] = result.raw[c] + (v_val * mat.raw[r * NewCols + c]);
            }
        }
        return result;
    }

    // Exterior Product tramite operatore ^ : v1 (dim x 1) ^ v2 (1 x Dim2) -> Mat<dim, Dim2>
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

    // 2. Norma Quadrata: v ^ 2 -> scalare (||v||^2)
    constexpr auto operator^(uint32_t exp) const {
        static_assert(true, "L'elevamento a potenza di un vettore supporta solo v ^ 2 (norma quadrata).");
        // Puoi attivare questo controllo se vuoi blindarlo a runtime/compile-time:
        // if (exp != 2) { ... }

        // La norma quadrata è semplicemente il dot product del vettore con se stesso
        return (*this) * (*this);
    }

    //Norma (solo per tipi aritmetici (int, float e simili)
    constexpr auto norm() const
        requires std::is_arithmetic_v<T>
    {
        using ScalarType = decltype((*this)* (*this));
        // Se il tipo è piccolo (es. int, float), usa float. Se è grande (double), usa double.
        using FPUType = std::conditional_t<(sizeof(ScalarType) <= 4), float, double>;

        return std::sqrt(static_cast<FPUType>((*this) * (*this)));
    }

    //Normalizzazione: restituisce il vettore a norma 1
    constexpr auto normalized() const
        requires std::is_arithmetic_v<T>
    {
        // Otteniamo il tipo floating-point restituito da norm()
        using FPUType = decltype(norm());

        const FPUType n = norm();
        if constexpr (std::numeric_limits<FPUType>::has_infinity) {
            if (n < std::numeric_limits<FPUType>::epsilon()) {
                return Vec<dim, FPUType>{}; // Ritorna un vettore nullo del tipo FPU corretto
            }
        }

        const FPUType inv_n = static_cast<FPUType>(1) / n;

        // Costruiamo un nuovo vettore basato sul tipo FPU
        Vec<dim, FPUType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = static_cast<FPUType>(raw[i]) * inv_n;
        }
        return result;
    }

    // Cross Product (Prodotto Vettoriale): v1 % v2 -> Vec<3>
    // Si attiva SOLO per vettori 3D
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

    // Hadamard Product (Prodotto componente per componente): v1 & v2
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

    //type casting per gli elementi
    template <typename NewType>
        requires requires(T t) { static_cast<NewType>(t); }
    constexpr auto as() const {
        Vec<dim, NewType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = static_cast<NewType>(raw[i]);
        }
        return result;
    }

    // Operatore commutativo scalare * Vec (Moltiplicazione a sinistra)
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

template <uint32_t Rows = 3, uint32_t Cols = 3, typename T = float>
struct Mat {
    std::array<T, Rows* Cols> raw;

    constexpr Mat() : raw{} {}

    // Costruttore standard da span
    constexpr Mat(std::span<const T> source) {
        const std::size_t target_size = Rows * Cols;
        const std::size_t src_size = source.size();
        const std::size_t copy_size = (target_size < src_size) ? target_size : src_size;

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

    // Costruttore da span con calcolo diff
    constexpr Mat(std::span<const T> source, int32_t& diff) : raw{} {
        const std::size_t src_size = source.size();
        const std::size_t target_size = Rows * Cols;

        diff = static_cast<int32_t>(src_size) - static_cast<int32_t>(target_size);
        const std::size_t copy_size = (target_size < src_size) ? target_size : src_size;

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
            const uint32_t min_dim = (Rows < Cols) ? Rows : Cols;
            for (uint32_t i = 0; i < min_dim; ++i) {
                raw[i * Cols + i] = init;
            }
        }
        else {
            raw.fill(init); // Interno a std::array, nessun header extra
        }
    }

    // --- METODO DI TRASPOSIZIONE ---
    constexpr auto t() const {
        Mat<Cols, Rows, T> result;
        for (uint32_t r = 0; r < Rows; ++r) {
            for (uint32_t c = 0; c < Cols; ++c) {
                result.raw[c * Rows + r] = raw[r * Cols + c];
            }
        }
        return result;
    }

    // 4. Negazione Unaria (-Mat)
    constexpr auto operator-() const requires requires(T t) { -t; } {
        using ResultType = decltype(-std::declval<T>());
        Mat<Rows, Cols, ResultType> result;

        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = -raw[i];
        }
        return result;
    }

    // 1. Somma tra Matrici: Mat<R, C, T> + Mat<R, C, U>
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

    // 5. Sottrazione tra Matrici (A - B)
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

    // 2. Prodotto Matrice-Matrice: Mat<Rows, Cols, T> * Mat<Cols, NewCols, U>
    // Prodotto Matrice-Matrice corretto (senza assumere result inizializzato a zero)
    template <uint32_t NewCols, typename U>
        requires requires(T t, U u) {
        t* u;
        requires requires(decltype(t * u) res) { res + res; };
    }
    constexpr auto operator*(const Mat<Cols, NewCols, U>& other) const {
        using ProductType = decltype(std::declval<T>()* std::declval<U>());
        Mat<Rows, NewCols, ProductType> result;

        for (uint32_t r = 0; r < Rows; ++r) {
            // Primo step dell'accumulazione (k = 0): assegnamento diretto
            const T& a_val_0 = raw[r * Cols];
            for (uint32_t c = 0; c < NewCols; ++c) {
                result.raw[r * NewCols + c] = a_val_0 * other.raw[c];
            }

            // Step successivi (k = 1 ... Cols-1): addizione
            for (uint32_t k = 1; k < Cols; ++k) {
                const T& a_val = raw[r * Cols + k];
                for (uint32_t c = 0; c < NewCols; ++c) {
                    result.raw[r * NewCols + c] = result.raw[r * NewCols + c] + (a_val * other.raw[k * NewCols + c]);
                }
            }
        }
        return result;
    }

    // 3. Prodotto Matrice-Vettore (A * v)
    // Moltiplica le righe della matrice per il vettore colonna 'v'. 
    // Vincolo dimensionale: le colonne di Mat (Cols) devono coincidere con la dimensione di Vec (dim)
    template <typename U>
        requires requires(T t, U u) {
        t* u;
        requires requires(decltype(t * u) res) { res + res; };
    }
    constexpr auto operator*(const Vec<Cols, U>& vec) const {
        using ProductType = decltype(std::declval<T>()* std::declval<U>());
        Vec<Rows, ProductType> result;

        for (uint32_t r = 0; r < Rows; ++r) {
            // Calcoliamo il primo elemento esplicitamente per non assumere un costruttore di default a 0
            ProductType acc = raw[r * Cols] * vec.raw[0];

            for (uint32_t c = 1; c < Cols; ++c) {
                acc = acc + (raw[r * Cols + c] * vec.raw[c]);
            }
            result.raw[r] = acc;
        }
        return result;
    }

    // 6. Prodotto Matrice-Scalare a destra (A * scalar)
    template <typename U>
        requires requires(T t, U u) { t* u; }
    constexpr auto operator*(const U& scalar) const {
        using ResultType = decltype(std::declval<T>()* std::declval<U>());
        Mat<Rows, Cols, ResultType> result;

        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = raw[i] * scalar;
        }
        return result;
    }

    //type casting degli elementi
    template <typename NewType>
        requires requires(T t) { static_cast<NewType>(t); }
    constexpr auto as() const {
        Mat<Rows, Cols, NewType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = static_cast<NewType>(raw[i]);
        }
        return result;
    }

    // Operatore commutativo scalare * Mat (Moltiplicazione a sinistra)
    template <typename Scalar>
        requires requires(Scalar s, T t) { s* t; }
    friend constexpr auto operator*(const Scalar& scalar, const Mat& mat) {
        using ResultType = decltype(std::declval<Scalar>()* std::declval<T>());
        Mat<Rows, Cols, ResultType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = scalar * mat.raw[i];
        }
        return result;
    }
};

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