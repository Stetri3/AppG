#include "program.h"
#include "utils/wgpu_utils.h"
#include <iostream>

GraphicInit::GraphicInit() {
	iDesc.nextInChain = nullptr;
	adOptions = {};
	dDesc.nextInChain = nullptr; // No additional options
	dDesc.requiredLimits = nullptr; // Use default limits
	dDesc.requiredFeatures = nullptr; // Use default features
	//Callback errore device lost
	dDesc.deviceLostCallbackInfo = utils::wgpu::setDLCInfo([](const WGPUDevice* device, WGPUDeviceLostReason reason, WGPUStringView message, void* userdata, void* userdata2) {
		std::cerr << "Device lost: " << message.data << " (reason: " << reason << ")" << std::endl;
		// Qui puoi eseguire il cleanup delle risorse e chiudere l'applicazione
		exit(1);
		}, nullptr);

	//Callback errore non catturato (da fare dopo ora andiamo alla mano tanto non sono cose pesanti al massimo si chiude)
	//No invece a quanto pare è quello più importante sono gli errori silenziosi
	dDesc.uncapturedErrorCallbackInfo = utils::wgpu::setUECInfo([](const WGPUDevice* device, WGPUErrorType type, WGPUStringView message, void* userdata, void* userdata2) {
		std::cerr << "Uncaptured error: " << message.data << " (type: " << type << ")" << std::endl;
		// Qui puoi eseguire il logging o altre azioni in caso di errori non catturati
		}, nullptr);
}
namespace program::detail {

	WGPUSurface getWindowSurface(WGPUInstance& instance, qContext context){
		WGPUSurfaceSourceWindowsHWND surfaceSource = {};
		surfaceSource.chain.sType = WGPUSType_SurfaceSourceWindowsHWND;
		surfaceSource.hwnd = context.hWnd;
		surfaceSource.hinstance = context.hInstance;
		WGPUSurfaceDescriptor surfaceDesc = {};
		surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&surfaceSource.chain);
		return wgpuInstanceCreateSurface(instance, &surfaceDesc);
	}
}