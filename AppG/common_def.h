#pragma once
#include <dawn/webgpu.h>
#include <cstddef> 

#ifndef DebugMessage
#define DebugMessage(...) (void(0))
#endif // !DebugMessage



//Aggiunge ""_ws per convertire char* in WGPUStringView, più veloce che constructor
constexpr WGPUStringView operator "" _ws(const char* str, std::size_t len) {
	return WGPUStringView{
		.data = str,
		.length = len
	};
}