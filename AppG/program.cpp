#include "program.h"
#include "utils/wgpu_utils.h"
#include "utils.h"
#include "window/quickWindow.h"
#include <tuple>

namespace program::detail {;
	WGPUSurface getWindowSurface(WGPUInstance &instance, qContext context);
}

uint64_t basicCallback(void* inst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	Program* th = static_cast<Program*>(inst);
	switch (message) {
	case WM_CLOSE:
		th->onClose();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE: {
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		//Attenzione, le coordinate windows vanno left->right top->bottom, (0,0) è alto a sinistra
		th->notifySizeChange(clientRect);
		return 0;
	}
	case WM_ERASEBKGND:
		return 1;
	default:
		//DebugMessage("Window event catturato, tipo: ", message);
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}

int Program::beginGraphics()
{
	if (windowView) {
		wgpuTextureViewRelease(this->windowView);
	}
	if (windowTexture.texture) {
		wgpuTextureRelease(windowTexture.texture);
	}
	if (dirtyGSize) {
		if (windowSize.w == 0 || windowSize.h == 0) {
			std::tie(this->windowTexture, this->windowView) = utils::wgpu::getNextTexture(windowSurface, surfViewDesc);
			return 4;
		}
		else {
			this->windowConfig.width = windowSize.w;
			this->windowConfig.height = windowSize.h;
			wgpuSurfaceConfigure(windowSurface, &windowConfig);
			dirtyGSize = false;
		}
	}

	std::tie(this->windowTexture, this->windowView) = utils::wgpu::getNextTexture(windowSurface, surfViewDesc);
	if (windowTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
		if (windowTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
			//Per ora ignoriamo
		}
		else {
			DebugMessage("Errore, windowTexture non inizializzato correttamente. Status: ", (int)(windowTexture.status));
			onClose();
			return -3;
		}
	}
	engine->Update(gTimer.get());
	engine->Render(windowView);
	return 0;
}

int Program::beginLogic()
{
	//Check size
	if (dirtySize)
		onResizeWindow();
	return 0;
}

Program::Program(GraphicInit gIn, WindowInit wIn)
{
	DebugMessage("Initializing graphics...");
	this->instance = wgpuCreateInstance(&(gIn.iDesc));
	if (!instance) {
		DebugMessage("Errore, instance non creata");
		return;
	}
	this->adapter = utils::wgpu::requestAdapterSync(instance, &gIn.adOptions);
	if(!adapter) {
		DebugMessage("Errore, adapter non creato");
		return;
	}

	this->device = utils::wgpu::requestDeviceSync(adapter, &gIn.dDesc);
	if (!device) {
		DebugMessage("Errore, device non creato");
		return;
	}

	DebugMessage("initializing window...");
	this->windowContext = qCreateWindow(wIn.wName, this);
	qSetGen(wIn.callback);
	if (wIn.callback == nullptr) qSetGen(qOnEventDefault);
	this->windowSurface = program::detail::getWindowSurface(instance, windowContext);

	queue = wgpuDeviceGetQueue(device);

	//Ottieni capabilities per configurazione ottimale
	WGPUSurfaceCapabilities capabilities = {};
	wgpuSurfaceGetCapabilities(windowSurface, adapter, &capabilities);
	if (capabilities.formats) PREFERRED_FORMAT = capabilities.formats[0]; //Il primo format è sempre il preferito
	else PREFERRED_FORMAT = WGPUTextureFormat_BGRA8UnormSrgb;
	wgpuSurfaceCapabilitiesFreeMembers(capabilities);
}

void Program::init()
{
	
	//Configura la surface
	windowConfig.nextInChain = nullptr; // No additional options
	windowConfig.width = 800; // Larghezza della superficie
	windowConfig.height = 600; // Altezza della superficie
	windowConfig.format = PREFERRED_FORMAT; // Formato della superficie
	windowConfig.viewFormatCount = 0; // Numero di formati di vista supportati
	windowConfig.viewFormats = nullptr; // Formati di vista supportati
	windowConfig.usage = WGPUTextureUsage_RenderAttachment; // Uso della superficie (render target)
	windowConfig.device = device; // Dispositivo associato alla superficie
	windowConfig.presentMode = WGPUPresentMode_Fifo; // Modalità di presentazione (vsync)
	//Fifo prende il primo buffer dalla queue (no frame scartati), mailbox ha un solo buffer (passa il più nuovo), immediate non ha buffer(passa chi è generato)
	windowConfig.alphaMode = WGPUCompositeAlphaMode_Auto; // Modalità di composizione alpha (lascia scelta al sistema)
	//TODO: rendering, pipeline, shader, ecc

	wgpuSurfaceConfigure(this->windowSurface, &windowConfig); //Configura la superficie con le impostazioni specificate


	//Imposta un messaggio rapido per work done attraverso utils (demo)
	wgpuQueueOnSubmittedWorkDone(queue, utils::wgpu::setQWorkDoneInfo(nullptr));

	ShowWindow(windowContext.hWnd, SW_SHOW);

	surfViewDesc.nextInChain = nullptr;
	surfViewDesc.label = "Surface texture view"_ws;
	surfViewDesc.dimension = WGPUTextureViewDimension_2D;
	surfViewDesc.baseMipLevel = 0;
	surfViewDesc.mipLevelCount = 1;
	surfViewDesc.baseArrayLayer = 0;
	surfViewDesc.arrayLayerCount = 1;
	surfViewDesc.aspect = WGPUTextureAspect_All;
	std::tie(this->windowTexture, this->windowView) = utils::wgpu::getNextTexture(windowSurface, surfViewDesc);
	if (windowTexture.status != WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal) {
		if (windowTexture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal) {
			//Per ora ignoriamo
		}
		else {
			DebugMessage("Errore, windowTexture non inizializzato correttamente. Status: ", (int)(windowTexture.status));
			onClose();
			return;
		}
	}
	engine = std::make_unique<Engine>(this);
	DebugMessage("Initializing engine...");
	engine->Initialize();

	//Post init:
	gTimer.start();
}

bool Program::run()
{
	//Dispatcher (NON HANDLER)
	MSG msg;
	while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg); // Chiama la tua qWndProc
	}

	//Qui la logica fissa rising edge
	int retL = beginLogic();
	int retG = beginGraphics();

	if (retG == 4) {
		//RetCode::window_zero : 
		return true;
	}

	

	return running;
}

void Program::onResizeWindow()
{
	this->windowSize = asyincWindowSize;
	engine->HandleResize(windowSize.w, windowSize.h);
	//Nota: per comodità teniamo dirtySize true fino a beginGraphics(), toggle sarà fatto lì
	//Update: dirtySize rimosso qui, dirtyGSize per la parte grafica
	dirtySize = false;
}

void Program::notifySizeChange(RECT rect)
{
	uint16_t w = utils::max_zero(rect.right - rect.left);
	uint16_t h = utils::max_zero(rect.bottom - rect.top);
	asyincWindowSize = { w, h };
	dirtySize = true;
	dirtyGSize = true;
}

void Program::onClose()
{
	DestroyWindow(windowContext.hWnd);
	clean();
	running = false;
}

void Program::clean()
{
	if (windowView) wgpuTextureViewRelease(windowView);
	if (windowSurface) {
		wgpuSurfaceUnconfigure(windowSurface);
		wgpuSurfaceRelease(windowSurface);
	}
	if (queue) wgpuQueueRelease(queue);
	if (device) {
		wgpuDeviceDestroy(device);
		wgpuDeviceRelease(device); }
	if (adapter) wgpuAdapterRelease(adapter);
	if (instance) wgpuInstanceRelease(instance);
}
