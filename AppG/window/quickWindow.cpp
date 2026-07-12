
#include "quickWindow.h"

uint64_t qOnEventDefault(void* inst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CLOSE:
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_SIZE: {
		RECT clientRect;
		GetClientRect(hWnd, &clientRect);
		//Attenzione, le coordinate windows vanno left->right top->bottom, (0,0) è alto a sinistra
		
		return 0;
	}
	case WM_ERASEBKGND:
		return 1;
	default:
		DebugMessage("Window event non gestito, tipo: ", message);
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
}
QGenCallback qOnEvent = qOnEventDefault;


void qSetGen(QGenCallback callback) {
	qOnEvent = callback;
}

LRESULT CALLBACK qWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return qOnEvent((void*)GetWindowLongPtr(hWnd, GWLP_USERDATA), hWnd, message, wParam, lParam);
	
}

LRESULT qSetupWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_NCCREATE) {
		LRESULT res = DefWindowProc(hWnd, message, wParam, lParam);
		CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
		void* inst = (void*)pCreate->lpCreateParams;
		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)inst);
		SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)qWndProc);
		return res;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

qContext qCreateWindow(const wchar_t* wName, void* inst) {
	HINSTANCE hInstance = GetModuleHandleW(NULL);
	WNDCLASSEXW wc = {};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.hInstance = hInstance;
	wc.lpszClassName = wName;
	wc.hCursor = LoadCursorW(NULL, LPCWSTR(IDC_ARROW));
	wc.hbrBackground = NULL;
	wc.lpfnWndProc = qSetupWndProc;
	if (!RegisterClassExW(&wc)) {
		std::cerr << "Failed to register window class: " << GetLastError() << std::endl;
		return qContext{nullptr, hInstance};
	}
	return qContext{CreateWindowExW(0, wName, L"Canvas", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, wc.hInstance, inst), hInstance};
}