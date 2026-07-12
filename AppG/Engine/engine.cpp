#include "engine.h"
#include "../program.h"

Engine::Engine(Program* program) : device(program->device)
{
	wgpuDeviceGetFeatures(device, &devFeatures);
	wgpuDeviceGetLimits(device, &devLimits);
}
