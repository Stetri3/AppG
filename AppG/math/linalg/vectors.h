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
#include "vec_utils.h"


//ATTENZIONE! IL CODICE COMPILA E FUNZIONA, MAI CAMBIARLO, SE PROPRIO PROPRIO FARE UN BACKUP TENENDO QUESTO MESSAGGIO

//Macro per il casting inline, eliminata a fine file
#define C(x) static_cast<AsType>(x)

//Dichiarazioni per cond
template <uint32_t dim, typename T>
struct Vec;
template <uint32_t Rows, uint32_t Cols, typename T>
struct Mat;
template <uint32_t dim, typename T>
struct MatQ;

namespace math::cond {
    //Condizionali constexpr 
    
    //Concetti di strutture algebriche (utili per verifiche)
    //Notare che ovviamente differiscono dalle vere strutture algebriche
    //In quanto non è possibile e anche abbastanza restrittivo verificare la chiusura
    //Inoltre le proprietà fanno riferimento agli elementi, non a loro insiemi ovviamente
    //Quasi dimenticavo ovviamente non controllano neanche l'esistenza di elementi "speciali" (1,0,...)
    //Quini le nomenclature saranno leggermente diverse per questo
    
    //Semigruppo (chiuso rispetto alla somma)
    template <typename T>
    concept Semigroup = requires(T a, T b) {
        { a + b } -> std::same_as<T>;
    };
    template <typename T>
    struct is_semigroup : std::bool_constant<Semigroup<T>> {};
    template <typename T>
    inline constexpr bool is_semigroup_v = is_semigroup<T>::value;

    //Gruppo (chiuso rispetto alla somma e con el. inverso/divisione)
    template <typename T>
    concept Group = Semigroup<T> && requires(T a, T b) {
        { a - b } -> std::same_as<T>;
        { -a }    -> std::same_as<T>;
    };
    template <typename T>
    struct is_group : std::bool_constant<Group<T>> {};
    template <typename T>
    inline constexpr bool is_group_v = is_group<T>::value;

    //Semigruppo per il prodotto (semigruppo rispetto a *)
    template <typename T>
    concept SemigroupProd = requires(T a, T b) {
        { a* b } -> std::same_as<T>;
    };
    template <typename T>
    struct is_semigroupprod : std::bool_constant<SemigroupProd<T>> {};
    template <typename T>
    inline constexpr bool is_semigroupprod_v = is_semigroupprod<T>::value;

    //SEMIANELLO (chiuso rispetto a + e *)
    template <typename T>
    concept Semiring = Semigroup<T> && SemigroupProd<T>;
    template <typename T>
    struct is_semiring : std::bool_constant<Semiring<T>> {};
    template <typename T>
    inline constexpr bool is_semiring_v = is_semiring<T>::value;

    //ANELLO (con + e *, con inverso additivo e inverso unario)
    template <typename T>
    concept Ring = Group<T> && SemigroupProd<T>;
    template <typename T>
    struct is_ring : std::bool_constant<Ring<T>> {};
    template <typename T>
    inline constexpr bool is_ring_v = is_ring<T>::value;

    //ANELLO CON DIVISIONE (autoesplicativo)
    template <typename T>
    concept RingDiv = Ring<T> && requires(T a, T b) {
        { a / b } -> std::same_as<T>;
    };
    template <typename T>
    struct is_ringdiv : std::bool_constant<RingDiv<T>> {};
    template <typename T>
    inline constexpr bool is_ringdiv_v = is_ringdiv<T>::value;

    //Per campo (RingDiv + commutatività per moltiplicazione) usiamo semplicemente tipo aritmetico
    template <typename T>
    concept Field = std::is_arithmetic_v<T>;
    template <typename T>
    struct is_field : std::bool_constant<Field<T>> {};
    template <typename T>
    inline constexpr bool is_field_v = is_field<T>::value;

    //Tipi un po' più "complicati"
    //PRODOTTO CON IMMAGINE A SEMIGRUPPO: il prodotto è chiuso rispetto alla somma, ovvero esiste 
    //un'operazione prodotto (ordinato T*U) la cui immagine è chiusa rispetto alla somma, T*U + T*U = T*U
    template <typename T, typename U>
    concept ProdImageSemigroup = requires(T t, U u) {
        { t* u };
    }&& Semigroup<decltype(std::declval<T>()* std::declval<U>())>;
    template <typename T, typename U>
    struct is_prodimagesemigroup : std::bool_constant<ProdImageSemigroup<T, U>> {};
    template <typename T, typename U>
    inline constexpr bool is_prodimagesemigroup_v = is_prodimagesemigroup<T, U>::value;

    //SOMMA CON IMMAGINE A SEMIGRUPPO (la somma è chiusa rispetto al prodotto)
    //Come sopra invertendo somma con prodotto
    template <typename T, typename U>
    concept SumImageSemigroup = requires(T t, U u) {
        { t + u };
    }&& SemigroupProd<decltype(std::declval<T>() + std::declval<U>())>;
    template <typename T, typename U>
    struct is_sumimagesemigroup : std::bool_constant<SumImageSemigroup<T, U>> {};
    template <typename T, typename U>
    inline constexpr bool is_sumimagesemigroup_v = is_sumimagesemigroup<T, U>::value;

    //IDENTIFICATORI TIPI ALGEBRICI
    //Per identificare vec
    template <typename T>
    struct is_vec : std::false_type {};

    template <uint32_t dim, typename T>
    struct is_vec<Vec<dim, T>> : std::true_type {};

    // Il concetto finale
    template <typename T>
    concept VecType = is_vec<T>::value;
    //Alias constexpr bool diretto
    template <typename T>
    inline constexpr bool is_vec_v = is_vec<T>::value;

    
    //Per MAT::

    //Per matQ
    template <typename T>
    struct is_matQ : std::false_type {};

    template <uint32_t dim, typename T>
    struct is_matQ<MatQ<dim, T>> : std::true_type {};

    // Il concetto finale
    template <typename T>
    concept MatQType = is_matQ<T>::value;
    //Alias bool
    template <typename T>
    inline constexpr bool is_matQ_v = is_matQ<T>::value;

    //Per mat generica
    template <typename T>
    struct is_mat : std::false_type {};

    template <uint32_t Rows, uint32_t Cols, typename T>
    struct is_mat<Mat<Rows, Cols, T>> : std::true_type {};

    template <uint32_t dim, typename T>
    struct is_mat<MatQ<dim, T>> : std::true_type {};

    // Il concetto finale
    template <typename T>
    concept MatType = is_mat<T>::value;
    //Alias bool
    template <typename T>
    inline constexpr bool is_mat_v = is_mat<T>::value;

    // 2. Il concetto algebraic (Vec || Mat)
    template <typename T>
    concept AlgebraicType = VecType<T> || MatType<T>;

    // =================================================================
    // 3. Il trait "is_algebraic" definito in una riga usando il concetto
    // =================================================================
    template <typename T>
    struct is_algebraic : std::bool_constant<AlgebraicType<T>> {};

    //Constexpr bool
    template <typename T>
    inline constexpr bool is_algebraic_v = is_algebraic<T>::value;

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

    //utile: template per passare tipi a val vs const ref
    template <typename T>
    using pass_t = std::conditional_t<
        (sizeof(T) <= 16 && std::is_trivially_copyable_v<T>),
        T,
        const T&
    >;
}

namespace co = math::cond;




template <uint32_t dim = 3, typename T = float>
struct Vec {
    std::array<T, dim> raw;

    //Definizioni constexpr per ricavare i meta
    static constexpr uint32_t SIZE = dim;
    using ElType = T;

    //Definizione a 0 costo (0 righe a runtime) per scovare differenze tra vector simili
    enum { _depth = co::_depth_v<T> + 1};


    //COSTRUTTORI:

    //Tenere copia default (per ereditare trivial copiability)
    Vec(const Vec&) = default;
    Vec(Vec&) = default;
    constexpr Vec& operator =(const Vec&) = default;

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
    constexpr Vec(co::pass_t<T> init) {
        raw.fill(init);
    }

   
    //Constexpr base canonica
    template <uint32_t index>
    static constexpr Vec Canon(co::pass_t<T> val = 1) {
        static_assert(index < dim, "Errore: canon index dev'essere minore di dim");
        Vec v = Vec();
        v[index] = val;
        return v;
    }
    //Runtime canonica
    template <bool check = true>
    static Vec Canon(uint32_t index, co::pass_t<T> val = 1)
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
        requires requires(T t, U u) { t - u; }
    constexpr auto operator+(const Vec<dim, U>& other) const {
        using ResultType = decltype(std::declval<T>() + std::declval<U>());
        Vec<dim, ResultType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = raw[i] + other.raw[i];
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

    // PRODOTTO SCALARE (Vettore * Vettore dello stesso livello)
    //Possibile se prodimagesemigroup (spiegato a definizione)
    //_depth è meta solo per sync VecType, non è un constraint algebrico
    template <typename U>
        requires (_depth == Vec<dim, U>::_depth && co::ProdImageSemigroup<T,U>)
    constexpr auto operator*(const Vec<dim, U>& other) const {
        using ProductType = decltype(std::declval<T>()* std::declval<U>());
        ProductType acc = raw[0] * other.raw[0];
        for (size_t i = 1; i < dim; ++i) {
            acc = acc + (raw[i] * other.raw[i]);
        }
        return acc;
    }

    // 2. PRODOTTO PER SCALARE GENERICO (Vincolato rigorosamente al livello adiacente o a depth 0)
    template <typename U>
        requires (co::_depth_v<U> <_depth && !co::is_mat_v<U> && requires(T t, U u) { t* u; }
            && !requires(U u, T t) { { u* t } -> std::convertible_to<T>; })
    constexpr auto operator*(const U& scalar) const {
        using ResultType = decltype(std::declval<T>()* std::declval<U>());
        Vec<dim, ResultType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = raw[i] * scalar;
        }
        return result;
    }

    // PRODOTTO PER SCALARE DI CAMPO A DESTRA (Vettore * Scalare) (mantiene Vettore nello stesso spazio)
    template <typename U>
        requires (co::_depth_v<U> < _depth
    && requires(T t, U u) { { t* u } -> std::convertible_to<T>; })
        constexpr auto operator*(const U& scalar) const {
        Vec<dim, T> result; // Il tipo di ritorno rimane Vec<dim, T>
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = raw[i] * scalar; // Cast o conversione implicita a T
        }
        return result;
    }

    // 3. PRODOTTO PER SCALARE GENERICO A SINISTRA (Scalare * Vettore)
    template <typename U>
        requires ((co::_depth_v<U> < _depth&& !co::is_mat_v<U>) && requires(U u, T t) { u* t; }
            && !requires(U u, T t) { { u* t } -> std::convertible_to<T>; })
    friend constexpr auto operator*(const U& scalar, const Vec<dim, T>& vec) {
        using ResultType = decltype(std::declval<U>()* std::declval<T>());
        Vec<dim, ResultType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = scalar * vec.raw[i];
        }
        return result;
    }

    // PRODOTTO PER SCALARE DI CAMPO A SINISTRA (Scalare * Vettore)
    template <typename U>
        requires (co::_depth_v<U> < _depth
    && requires(U u, T t) { { u* t } -> std::convertible_to<T>; })
        friend constexpr auto operator*(const U& scalar, const Vec<dim, T>& vec) {
        Vec<dim, T> result; // Anche qui il tipo di ritorno rimane Vec<dim, T>
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = scalar * vec.raw[i]; // Cast o conversione implicita a T
        }
        return result;
    }


    //EXTERIOR PRODUCT (vec ^ vec = mat)
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

    constexpr auto norm() const requires co::Field<T> {
        using ScalarType = decltype((*this)* (*this));
        using FPUType = std::conditional_t<(sizeof(ScalarType) <= 4), float, double>;
        return std::sqrt(static_cast<FPUType>((*this) * (*this)));
    }

    constexpr auto normalized() const requires co::Field<T> {
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

    //Vector product
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

    //Hadamard product
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

    //Static cast a un altro T (ovviamente deve essere castable)
    template <typename NewType>
        requires requires(T t) { static_cast<NewType>(t); }
    constexpr auto as() const {
        Vec<dim, NewType> result;
        for (size_t i = 0; i < dim; ++i) {
            result.raw[i] = static_cast<NewType>(raw[i]);
        }
        return result;
    }

    //UTILS
    constexpr auto str() const noexcept {
        return math::utils::to_string(*this);
    }
};

template <uint32_t Rows, uint32_t Cols, typename T>
struct Mat {
    std::array<T, Rows* Cols> raw;

    //Ricavo tipi
    static constexpr uint32_t ROWS = Rows;
    static constexpr uint32_t COLS = Cols;
    using ElType = T;

    enum { _depth = co::_depth_v<T> +1 };


    //COSTRUTTORI
    //Tenere copia default (per ereditare trivial copiability)
    Mat(const Mat&) = default;
    Mat(Mat&) = default;
    constexpr Mat& operator =(const Mat&) = default;


    //Zeri
    constexpr Mat() : raw{} {}

    //Da raw array, a size giusta. Mai usare se non si conosce la size a priori
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

    //Uniforme
    constexpr Mat(co::pass_t<T> init) : raw{} {
        raw.fill(init);
    }

    //Da initializer list
    template <typename... Args>
        requires (sizeof...(Args) == Rows*Cols) && (std::is_convertible_v<Args, T> && ...)
    constexpr Mat(Args&&... args) : raw{ static_cast<T>(std::forward<Args>(args))... } {}


    //trasposta
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

    // 2. PRODOTTO MATRICE * VETTORE
    template <typename U>
        requires (_depth == Vec<Cols, U>::_depth && co::ProdImageSemigroup<T,U>)
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


    //PRODOTTO VETTORE * MATRICE
    template <typename U>
        requires co::ProdImageSemigroup<U,T>
    friend constexpr auto operator*(const Vec<Rows, U>& vec, const Mat& mat) {
        using ProductType = decltype(std::declval<U>()* std::declval<T>());
        Vec<Cols, ProductType> result;

        const U& v_val_0 = vec.raw[0];
        for (uint32_t c = 0; c < Cols; ++c) {
            result.raw[c] = v_val_0 * mat.raw[c];
        }

        for (uint32_t r = 1; r < Rows; ++r) {
            const U& v_val = vec.raw[r];
            for (uint32_t c = 0; c < Cols; ++c) {
                result.raw[c] = result.raw[c] + (v_val * mat.raw[r * Cols + c]);
            }
        }
        return result;
    }

    // 3. PRODOTTO MATRICE * SCALARE GENERICO
    template <typename U>
        requires (co::_depth_v<U> < _depth && requires(T t, U u) { t* u; })
    constexpr auto operator*(const U& scalar) const {
        using ResultType = decltype(std::declval<T>()* std::declval<U>());
        Mat<Rows, Cols, ResultType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = raw[i] * scalar;
        }
        return result;
    }

    

    //PRODOTTO SCALARE * MATRICE GENERICO
    template <typename U>
        requires ((co::_depth_v<U> < _depth || !co::is_algebraic_v<U>) && requires(T t, U u) { u* t; })
    friend constexpr auto operator*(const U& scalar, const Mat& mat) {
        using ResultType = decltype(std::declval<U>()* std::declval<T>());
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

    //Static casting
    template <typename NewType>
        requires requires(T t) { static_cast<NewType>(t); }
    constexpr auto as() const {
        Mat<Rows, Cols, NewType> result;
        for (size_t i = 0; i < Rows * Cols; ++i) {
            result.raw[i] = static_cast<NewType>(raw[i]);
        }
        return result;
    }

    //UTILS
    constexpr auto str() const noexcept {
        return math::utils::to_string(*this);
    }

    
};



//TIPO MatQ : Matrice quadrata

template <uint32_t dim, typename T>
struct MatQ : public Mat<dim, dim, T> {

    //Ereditarietà
    using Base = Mat<dim, dim, T>;
    using Base::Mat;
    using Base::raw;

    static constexpr uint32_t DIM = dim;

    //COSTRUTTORI

    //Tenere copia default (per ereditare trivial copiability)
    MatQ(const MatQ&) = default;
    MatQ(MatQ&) = default;
    constexpr MatQ& operator =(const MatQ&) = default;


    //Ereditati da base
    constexpr MatQ() : Base(){}
    constexpr MatQ(std::span<T> source): Base(source){}
    constexpr MatQ(std::span<T> source, uint32_t& diff) : Base(source, diff){}
    constexpr MatQ(co::pass_t<T> init): Base(init){}
    //Initializer list
    template <typename... Args>
        requires (sizeof...(Args) == dim) && (std::is_convertible_v<Args, T> && ...)
    constexpr MatQ(Args&&... args) : Base(args) {}


    //Conversione implicita da Base (Mat) quando Cols = Rows = dim, T = T
    constexpr MatQ(const Mat<dim, dim, T>& other): Base(other) {}

    constexpr MatQ& operator=(const Base& other) {
        Base::operator=(other);
        return *this;
    }

    //Costruttori statici

    //A compile time strict
    template <T val = 1>
    static consteval MatQ Diagonal() noexcept {
        MatQ ret{}; //costruzione a zeri
        for (uint32_t i = 0; i < dim; ++i) {
            ret.raw[i * (dim + 1)] = val;
        }
        return ret;
    }

    //Stessa funzione, semi compile time
    static constexpr MatQ Diagonal(co::pass_t<T> val) {
        MatQ ret{}; //costruzione a zeri
        for (uint32_t i = 0; i < dim; ++i) {
            ret.raw[i * (dim + 1)] = val;
        }
        return ret;
    }
    //Alias per comodità
    static consteval MatQ One() { return Diagonal(); } //Alias per 1
    inline static constexpr MatQ One(co::pass_t<T> val) { return Diagonal(val); } //Alias per 1


    //OVERLOAD OVERRIDING
    

    //Determinanti: questi variano in base al tipo e alla dimensione
    //Attenzione: se si vuole evitare overflow usare det<AsType>() definito sotto
    //Dim = 1, semplicemente il valore
    constexpr T det() const noexcept 
        requires (dim == 1) {
        return raw[0];
    }

    //Dim = 2, basta un Ring (non serve la divisione)
    constexpr T det() const noexcept 
        requires (dim == 2) && co::Ring<T> {
        return raw[0] * raw[3] - raw[1] * raw[2];
    }

    //Dim=3, regola di sarrus (da qui in poi è richiesto un field commutativo per l'efficienza)
    constexpr T det() const noexcept 
        requires (dim == 3) && co::Field<T> {
        // Calcoliamo i tre minori 2x2 della riga inferiore
        const T cofactor0 = raw[4] * raw[8] - raw[5] * raw[7];
        const T cofactor1 = raw[3] * raw[8] - raw[5] * raw[6];
        const T cofactor2 = raw[3] * raw[7] - raw[4] * raw[6];

        // Prodotto scalare della prima riga con i cofattori
        return raw[0] * cofactor0 - raw[1] * cofactor1 + raw[2] * cofactor2;
    }

    //Dim=4, suddivisione in blocchi 2x2
    constexpr T det() const noexcept
        requires (dim == 4 && co::Field<T>) {
        // 2x2 determinants of the top two rows
        const T s0 = raw[0] * raw[5] - raw[1] * raw[4];
        const T s1 = raw[0] * raw[6] - raw[2] * raw[4];
        const T s2 = raw[0] * raw[7] - raw[3] * raw[4];
        const T s3 = raw[1] * raw[6] - raw[2] * raw[5];
        const T s4 = raw[1] * raw[7] - raw[3] * raw[5];
        const T s5 = raw[2] * raw[7] - raw[3] * raw[6];

        // 2x2 determinants of the bottom two rows
        const T c0 = raw[8] * raw[13] - raw[9] * raw[12];
        const T c1 = raw[8] * raw[14] - raw[10] * raw[12];
        const T c2 = raw[8] * raw[15] - raw[11] * raw[12];
        const T c3 = raw[9] * raw[14] - raw[10] * raw[13];
        const T c4 = raw[9] * raw[15] - raw[11] * raw[13];
        const T c5 = raw[10] * raw[15] - raw[11] * raw[14];

        // Pairing them up (determinant is the dot product of these wedge products)
        return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
    }

    //Determinanti con promozione esplicita per evitare overflow o cambiare tipo di calcolo
    //AsType definisce il tipo a cui promuovere
    //Es. hai una matrice di int16 che arriva fino a 30k per elemento-> det<int32_t>() evita overflow
    //Mentre det<float>() calcola COME SE fosse float

    //Dim = 2
    template <typename AsType>
        requires (dim == 2) && co::Field<AsType>&& std::is_convertible_v<T, AsType>
    constexpr AsType det() const {
        return C(raw[0]) * C(raw[3]) -
            C(raw[1]) * C(raw[2]);
    }
    //Dim = 3
    template <typename AsType>
        requires (dim == 3) && co::Field<AsType>&& std::is_convertible_v<T, AsType>
    constexpr AsType det() const {
        const AsType cofactor0 = C(raw[4]) * C(raw[8]) -
            C(raw[5]) * C(raw[7]);
        const AsType cofactor1 = C(raw[3]) * C(raw[8]) -
            C(raw[5]) * C(raw[6]);
        const AsType cofactor2 = C(raw[3]) * C(raw[7]) -
            C(raw[4]) * C(raw[6]);

        return C(raw[0]) * cofactor0 -
            C(raw[1]) * cofactor1 +
            C(raw[2]) * cofactor2;
    }

    template <typename AsType>
        requires (dim == 4) && co::Field<AsType>&& std::is_convertible_v<T, AsType>
    constexpr AsType det() const {
        // Caricamento con estensione di segno al volo (zero-cost sui registri della CPU)
        const AsType r0 = C(raw[0]), r1 = C(raw[1]);
        const AsType r2 = C(raw[2]), r3 = C(raw[3]);
        const AsType r4 = C(raw[4]), r5 = C(raw[5]);
        const AsType r6 = C(raw[6]), r7 = C(raw[7]);
        const AsType r8 = C(raw[8]), r9 = C(raw[9]);
        const AsType r10 = C(raw[10]), r11 = C(raw[11]);
        const AsType r12 = C(raw[12]), r13 = C(raw[13]);
        const AsType r14 = C(raw[14]), r15 = C(raw[15]);

        const AsType s0 = r0 * r5 - r1 * r4;
        const AsType s1 = r0 * r6 - r2 * r4;
        const AsType s2 = r0 * r7 - r3 * r4;
        const AsType s3 = r1 * r6 - r2 * r5;
        const AsType s4 = r1 * r7 - r3 * r5;
        const AsType s5 = r2 * r7 - r3 * r6;

        const AsType c0 = r8 * r13 - r9 * r12;
        const AsType c1 = r8 * r14 - r10 * r12;
        const AsType c2 = r8 * r15 - r11 * r12;
        const AsType c3 = r9 * r14 - r10 * r13;
        const AsType c4 = r9 * r15 - r11 * r13;
        const AsType c5 = r10 * r15 - r11 * r14;

        return s0 * c5 - s1 * c4 + s2 * c3 + s3 * c2 - s4 * c1 + s5 * c0;
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
using Mat4f = MatQ<4, float>;
using Mat3f = MatQ<3, float>;
using Mat4i = MatQ<4, int32_t>;
using Mat3i = MatQ<3, int32_t>;
using Mat4u = MatQ<4, uint32_t>;
using Mat3u = MatQ<3, int32_t>;

#undef C