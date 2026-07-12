#pragma once
#include <dawn/webgpu.h>
#include <utility>

namespace utils::wgpu {
	WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options);
	WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor);
	WGPUDeviceLostCallbackInfo setDLCInfo(WGPUDeviceLostCallback callback, void* userdata);
	WGPUUncapturedErrorCallbackInfo setUECInfo(WGPUUncapturedErrorCallback callback, void* userdata);
	WGPUQueueWorkDoneCallbackInfo setQWorkDoneInfo(void* userdata);
	std::pair<WGPUSurfaceTexture, WGPUTextureView> getNextTexture(WGPUSurface surface, WGPUTextureViewDescriptor desc);
}