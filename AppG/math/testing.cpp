
#include "linalg/vectors.h"
//#include "../debug_helper.h"
#include <iostream>

void run_algebra_tests();
int main() {
    DebugMessage("--- STARTING BRAVE TESTS (STRICT ALGEBRA) ---");

    // =========================================================================
    // TEST 1: Il vero Prodotto Geometrico di Blocco (Mat * Vec dello stesso livello)
    // =========================================================================
    Mat<2, 2, Vec3f> m_blocchi(Vec3f(2.0f));
    Vec<2, Vec3f> v_coordinato(Vec3f(3.0f));

    // Compila come riga per colonna geometrico. I vettori profondi si contraggono 
    // nel loro prodotto interno legittimo. 
    // Risultato: Vec<2, float>
    auto res_geom = m_blocchi * v_coordinato;

    DebugMessage("Test 1 - Vero Prodotto Geometrico di Blocco:");
    for (float val : res_geom.raw) {
        DebugMessage(val); // Due volte 36
    }

    // =========================================================================
    // TEST 2: Omotetia Profonda (Scalare primordiale nudo, depth 0)
    // =========================================================================
    // Il float penetra la struttura senza alterare la forma dei blocchi.
    // Risultato: Mat<2, 2, Vec3f> dove ogni componente dei vettori è 20.0f
    auto res_deep_scal = m_blocchi * 10.0f;

    DebugMessage("Test 2 - Omotetia Profonda:");
    for (float val : res_deep_scal.raw[0].raw) {
        DebugMessage(val); // Tre volte 20
    }

    // =========================================================================
    // TEST 3: Il test di Ambiguità Critica (Risolto dalle depth)
    // =========================================================================
    // Se creiamo un vettore di matrici e lo moltiplichiamo per una matrice singola?
    // v_mat: Vettore di matrici 2x2 di float (_depth = 2)
    // m_singola: Matrice 2x2 di float (_depth = 1)
    // Siccome m_singola ha depth == _depth - 1 ed è compatibile come scalare
    // per gli elementi interni (che sono Mat2f), deve fare il prodotto per scalare di blocco.
    Vec<3, Mat3f> v_mat{1,2,3}; // supponendo l'uso delle tue typedef
    Mat3f m_singola = Mat3f::One(); // Identità se hai tenuto la logica diagonal

    // Deve distribuire la matrice su ogni elemento del vettore
    auto res_ambig = v_mat * m_singola;
    DebugMessage(res_ambig);
    DebugMessage("--- ALL STRICT TESTS PASSED ---");

    Mat3i a{1, 1, 1, 2, 1, 3, 5, 9, 0};
    auto b = v_mat * a;
    DebugMessage(b.str());
    run_algebra_tests();
    return 0;
}
void run_algebra_tests() {
    // 1. Matrice * Vettore
    constexpr Mat<2, 3, int> A{
        1, 2, 3,
        4, 5, 6
    };
    constexpr Vec<3, int> v{ 2, 1, 3 };
    auto res_mv = A * v; // Atteso: [13, 31]
    DebugMessage(res_mv.str());

    // 2. Vettore * Matrice
    constexpr Vec<2, int> u{ 2, 3 };
    auto res_vm = u * A; // Atteso: [14, 19, 24]
    DebugMessage(res_vm.str());

    // 3. Trasposta
    auto A_t = A.t(); // Atteso: [[1, 4]; [2, 5]; [3, 6]]
    DebugMessage(A_t.str());

    // 4. Matrice * Matrice
    constexpr Mat<3, 2, int> B{
        7, 8,
        9, 1,
        2, 3
    };
    auto C = A * B; // Atteso: [[31, 19]; [85, 55]]
    DebugMessage(C.str());

    // 5. Hadamard (%) e Norme su vettori
    constexpr Vec<3, float> v1{ 3.0f, 0.0f, 4.0f };
    constexpr Vec<3, float> v2{ 2.0f, 10.0f, -1.0f };

    auto v_had = v1 % v2; // Atteso: [6, 0, -4]
    DebugMessage(v_had.str());

    float n_sq = v1.norm(); // Atteso: 25
    DebugMessage(n_sq);

    auto v1_n = v1.normalized(); // Atteso: [0.6, 0.0, 0.8]
    DebugMessage(v1_n.str());
}