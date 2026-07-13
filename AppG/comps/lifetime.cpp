#include "lifetime.h"
#include <chrono>

// Utilizziamo lo steady_clock per evitare problemi con i cambi di ora del sistema
namespace {
    using Clock = std::chrono::steady_clock;
    inline int64_t nanoseconds() noexcept {
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            Clock::now().time_since_epoch()
        ).count();
    }
}
Time::Time(bool start_immediately, bool safe_get) {
    if (start_immediately) {
        start();
    }
}

void Time::start() noexcept {
    // Se il timer era in pausa, 'raw' conteneva il tempo accumulato.
    // Sottraendolo da now(), invertiamo il segno del timestamp di partenza.
    if (!paused) {
        //Niente succede, oppure il timer si riazzera, dipende da quale è più ottimizzata
        raw = nanoseconds();
        return;
    }
    paused = false;
    raw = nanoseconds() - raw; //quando !paused, raw contiene l'epoch simulando che sia chiamato un delta prima
}

void Time::pause() noexcept {
    // Calcola il tempo accumulato fino ad ora.
    raw = nanoseconds() - raw; //Quando paused, raw contiene il delta
    paused = true;
}

void Time::setTime(int64_t elapsed) noexcept {
    raw = paused? elapsed : nanoseconds() - elapsed;
}

int64_t Time::get() const noexcept {
    return paused ? raw : nanoseconds() - raw;
}
int64_t Time::get(int64_t point) const noexcept {
    //return il tempo passato da point (point relativo a start)
    return paused ? raw - point : nanoseconds() - raw - point;
}

int64_t Time::stop() noexcept {
    if (paused) {
        int64_t total_elapsed = raw;
        raw = 0;
        return total_elapsed;
    }

    int64_t total_elapsed = nanoseconds() - raw;
    raw = 0;
    paused = true;
    return total_elapsed;
}