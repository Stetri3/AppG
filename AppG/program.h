#pragma once
#include <dawn/webgpu.h>
#include "window/quickWindow.h"
#include "Engine/engine.h"
#include "comps/lifetime.h"
#include <memory>

#ifndef DebugMessage
#define DebugMessage(...) (void(0))
#endif // !DebugMessage
class Program;

uint64_t basicCallback(void* inst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct GraphicInit {
	WGPUInstanceDescriptor iDesc{};
	WGPURequestAdapterOptions adOptions{};
	WGPUDeviceDescriptor dDesc{};
	GraphicInit();
};
struct WindowInit {
	const wchar_t* wName = L"Window";
	QGenCallback callback = basicCallback;
};
class Program {
	friend class Engine;
private:
	//WGPU stuff
	WGPUInstance instance = nullptr;
	WGPUAdapter adapter = nullptr;
	WGPUDevice device = nullptr;
	WGPUQueue queue = nullptr;
	WGPUSurface windowSurface = nullptr;
	WGPUSurfaceConfiguration windowConfig = {};
	WGPUTextureViewDescriptor surfViewDesc = {};
	WGPUSurfaceTexture windowTexture = {};
	WGPUTextureView windowView = {};
	qContext windowContext = {};
	WGPUTextureFormat PREFERRED_FORMAT = WGPUTextureFormat::WGPUTextureFormat_RGBA8Uint;

	//engine
	std::unique_ptr<Engine> engine;
	//logic stuff
	struct Size {
		uint16_t w;
		uint16_t h;
	};
	Size windowSize{ 0,0 };
	Size asyincWindowSize{ 0,0 };
	bool dirtySize = true;
	bool dirtyGSize = true;
	bool running = true;
	Time gTimer = Time(false);


	int beginGraphics();
	int beginLogic();

public:
	Program(GraphicInit = GraphicInit(), WindowInit = WindowInit());
	void init();
	bool run();
	void onResizeWindow();
	void notifySizeChange(RECT rect);
	

	WGPUInstance getInstance() const { return instance; }
	WGPUAdapter getAdapter() const { return adapter; }
	WGPUDevice getDevice() const { return device; }
	WGPUSurface getWinSurface() const { return windowSurface; }
	WGPUQueue getQueue() const { return queue; }
	void setCallback(QGenCallback callback) { qSetGen(callback); }
	void onClose();
	void clean();
};