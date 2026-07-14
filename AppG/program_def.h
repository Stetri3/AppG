#pragma once

//File di definizioni comuni (indipendenti) di program
//Inoltre qui ci metto i todo relativi a program


namespace program {
	enum class RetCode : int {
		crash_risk = -1,
		none = 0,
		//...
		win_zero = 4, //La window ha almeno una dimensione 0
	};
}