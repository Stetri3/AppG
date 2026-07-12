#pragma once

#include <Windows.h>
#include <iostream>
#include <cstdint>
//Convenzione invece di namespace, Q prefisso agli oggetti e variabili

typedef uint64_t (*QGenCallback)(void* inst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void qSetGen(QGenCallback callback);

uint64_t qOnEventDefault(void* inst, HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

struct qContext {
	HWND hWnd;
	HINSTANCE hInstance;
};

LRESULT CALLBACK qWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK qSetupWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

qContext qCreateWindow(const wchar_t* wName, void* inst);