#pragma once
#include <cstdint>
//*
//@brief Semplice wrapper per intervalli di tempo
class Time {
	int64_t raw = 0; //Tempo "iniziale"
	bool paused = true;
public:
	Time(bool start = true, bool safe_get = false);
	void start() noexcept;
	void pause() noexcept;
	int64_t get(int64_t point) const noexcept;
	//@brief Chiamarlo prima di start o dopo stop conterrà valori inutili
	int64_t get() const noexcept;
	void setTime(int64_t elapsed) noexcept;
	int64_t stop() noexcept;
};