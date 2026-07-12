#pragma once
#include <dawn/webgpu.h>

#ifndef E_COUPLED
constexpr bool E_COUPLED = true; //metodo di coupling program/engine.
//Se true, engine modifica direttamente i field di Program senza passare attraverso i metodi
//Se false, program richiede i valori ad engine e ognuno modifica solo i propri membri
#endif

class Program;

class Engine {
	Program* program;
	WGPUDevice device;
	WGPUSupportedFeatures devFeatures;
	WGPULimits devLimits;

public:
	Engine(Program* program);
};