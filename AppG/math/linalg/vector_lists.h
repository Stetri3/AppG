#pragma once
#include "vectors.h"
#include <array>
#include <vector>

//In vectors.h, i constexpr conditionals sono nel namespace math::cond alias ::co::
template <uint32_t size, co::VecType V>
requires(size > 0)
struct sVArr {
	std::array<V, size> raw;
	static constexpr uint32_t size = size;

	//COSTRUTTORI

	//Zeri
	constexpr sVArr() noexcept: raw{} {}

	//Uniforme (a initval)
	constexpr sVArr(V initVal) noexcept {
		raw.fill(initVal);
	}

	//Copia da un iterable. Nota: usare solo se si è sicuri che source.size() = size
	//Altrimenti usare quello sotto a questo, ovvero sArr(std::span<V>, int32_t& diff)
	constexpr sVArr(std::span<const V> source) {
		//Dato che le persone a volte sono stupide, qui in debug non mettiamo noexcept
		std::copy(source.begin(), source.end(), raw.begin());
	}

	//Copia da un iterable, da sinistra. Diff restituisce la differenza in size, source_size - sArr.size()
	//Eventuali elementi non raggiunti sono impostati a 0 (ovvero Vec() a zeri)
	constexpr sVArr(std::span<const V> source, int32_t& diff) : raw{} {
		const std::size_t src_size = source.size();
		diff = static_cast<int32_t>(src_size) - static_cast<int32_t>(size);
		const std::size_t copy_size = std::min(static_cast<std::size_t>(size), src_size);

		if constexpr (std::is_trivially_copyable_v<V>) {
			::memcpy(raw.data(), source.data(), copy_size * sizeof(V));
		}
		else {
			const V* src_ptr = source.data();
			for (size_t i = 0; i < copy_size; ++i) {
				raw[i] = src_ptr[i];
			}
		}
	}


};

