#pragma once
namespace program {
	enum class RetCode : int {
		crash_risk = -1,
		none = 0,
		//...
		win_zero = 4, //La window ha almeno una dimensione 0
	};
}