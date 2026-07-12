#pragma once

namespace utils {
	template <typename T> inline T max_zero(T val) { return static_cast<T>((val > 0) ? val : 0); }
}