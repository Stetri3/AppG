#include "wgpu_utils.h"

#include <iostream>
#include <thread>
#include <chrono>

namespace utils::wgpu {
	const bool debug = true;
	//Richiede l'adapter in modo sincrono, praticamente infallibile ma time consuming
	// (Nota, in realtà spesso non passa neanche un ciclo)
	WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
		struct UserData {
			WGPUAdapter adapter = nullptr;
			bool requestEnded = false;
		};
		UserData userData;

		//Funzione di callback, ora va inserita nella struttura callbackInfo, prima andava direttamente alla requestAdapter con i parametri
		auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* pUserData, void* unused) {
			UserData& userData = *reinterpret_cast<UserData*>(pUserData);
			if (status == WGPURequestAdapterStatus_Success) {
				userData.adapter = adapter;
			}
			else {
				std::cerr << "Failed to get adapter: " << message.data << std::endl;
			}
			userData.requestEnded = true;
			};
		WGPURequestAdapterCallbackInfo callbackInfo = {};
		callbackInfo.nextInChain = nullptr; // No additional options
		callbackInfo.callback = onAdapterRequestEnded;
		callbackInfo.userdata1 = &userData;
		callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;



		wgpuInstanceRequestAdapter(instance, options, callbackInfo);

		while (!userData.requestEnded)
		{

			wgpuInstanceProcessEvents(instance);
			std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Sleep briefly to avoid busy-waiting
			if constexpr (debug) {
				static int counter = 0;
				counter++;
				std::cout << "Cicli attesi adapter: " << counter << std::endl;
			}
		}
		return userData.adapter;
	}

	WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor) {
		struct UserData {
			WGPUDevice device = nullptr;
			bool requestEnded = false;
		};
		UserData userData;
		auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* pUserData, void* unused) {
			UserData& userData = *reinterpret_cast<UserData*>(pUserData);
			if (status == WGPURequestDeviceStatus_Success) {
				userData.device = device;
			}
			else {
				std::cerr << "Failed to get device: " << message.data << std::endl;
			}
			userData.requestEnded = true;
			};
		WGPURequestDeviceCallbackInfo callbackInfo = {};
		callbackInfo.nextInChain = nullptr; // No additional options
		callbackInfo.callback = onDeviceRequestEnded;
		callbackInfo.userdata1 = &userData;
		callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
		wgpuAdapterRequestDevice(adapter, descriptor, callbackInfo);
		while (!userData.requestEnded)
		{

			std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Sleep briefly to avoid busy-waiting
			wgpuInstanceProcessEvents(wgpuAdapterGetInstance(adapter));
			if constexpr (debug) {
				static int counter = 0;
				counter++;
				std::cout << "Cicli attesi device: " << counter << std::endl;
			}
		}
		return userData.device;
	}
	WGPUDeviceLostCallbackInfo setDLCInfo(WGPUDeviceLostCallback callback, void* userdata) {
		WGPUDeviceLostCallbackInfo callbackInfo = {};
		callbackInfo.nextInChain = nullptr; // No additional options
		callbackInfo.callback = callback;
		callbackInfo.userdata1 = userdata;
		callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
		callbackInfo.userdata2 = nullptr; // Not used in this example
		return callbackInfo;
	}

	WGPUUncapturedErrorCallbackInfo setUECInfo(WGPUUncapturedErrorCallback callback, void* userdata) {
		WGPUUncapturedErrorCallbackInfo callbackInfo = {};
		callbackInfo.nextInChain = nullptr; // No additional options
		callbackInfo.callback = callback;
		callbackInfo.userdata1 = userdata;
		callbackInfo.userdata2 = nullptr; // Not used in this example
		return callbackInfo;
	}
	WGPUQueueWorkDoneCallbackInfo setQWorkDoneInfo(void* userdata) {
		WGPUQueueWorkDoneCallbackInfo callbackInfo = {};
		auto callback = [](WGPUQueueWorkDoneStatus status, WGPUStringView message, void* pUserData, void* unused) {
			// Qui puoi gestire il completamento del lavoro in coda
			std::cout << "Work done callback: " << message.data << " (status: " << status << ")" << std::endl;
			// Se hai bisogno di accedere a userdata, puoi farlo qui
			};
		callbackInfo.nextInChain = nullptr; // No additional options
		callbackInfo.callback = callback;
		callbackInfo.mode = WGPUCallbackMode_AllowSpontaneous;
		callbackInfo.userdata1 = userdata;
		callbackInfo.userdata2 = nullptr; // Not used in this example
		return callbackInfo;
	}

	std::pair<WGPUSurfaceTexture, WGPUTextureView> getNextTexture(WGPUSurface surface, WGPUTextureViewDescriptor desc) {
		//ask next surfacetexture, create textureview
		WGPUSurfaceTexture t;
		wgpuSurfaceGetCurrentTexture(surface, &t);
		desc.format = wgpuTextureGetFormat(t.texture);
		WGPUTextureView v = wgpuTextureCreateView(t.texture, &desc);
		return { t, v };
	}
}