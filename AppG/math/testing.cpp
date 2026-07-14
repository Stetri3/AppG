#include "vectors.h"
#define DEBUG
#include "../debug_helper.h"
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
    Vec<2, Mat3f> v_mat; // supponendo l'uso delle tue typedef
    Mat3f m_singola(1.0f, true); // Identità se hai tenuto la logica diagonal

    // Deve distribuire la matrice su ogni elemento del vettore
    auto res_ambig = v_mat * m_singola;

    DebugMessage("--- ALL STRICT TESTS PASSED ---");
    return 0;
}