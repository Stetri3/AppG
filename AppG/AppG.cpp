// AppG.cpp: definisce il punto di ingresso dell'applicazione.
//

#include "AppG.h"
#include <utility>
#include <chrono>
#include <thread>
#include "utils/quickShaders.h"
using namespace std;

//Variabili globali
bool run = true;
float fps = 60.0f;
bool hasDevice = false;

#define FRAME_DURATION long(1000000.0f / fps) // Durata di un frame in microsecondi





int main()
{


	Program program;
	program.init();
	WGPUShaderSourceWGSL source_test;
	source_test.chain.next = nullptr;
	source_test.chain.sType = WGPUSType_ShaderSourceWGSL; // Tells WebGPU this is WGSL text
	source_test.code = shader_test;
	WGPUShaderModuleDescriptor shDesc{};
	shDesc.nextInChain = &source_test.chain;
	shDesc.label = "Test shader"_ws;
	WGPUShaderModule shMod = wgpuDeviceCreateShaderModule(program.getDevice(), &shDesc);
	
	while (program.run()) {
		//Testing
		wgpuSurfacePresent(program.getWinSurface());
		this_thread::sleep_for(chrono::milliseconds(16));
	}
}
