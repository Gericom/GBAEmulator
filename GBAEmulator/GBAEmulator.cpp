// GBAEmulator.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include <stdio.h>
#include "Core\MemoryBus.h"
#include "Core\BIOS.h"
#include "Core\WRAM.h"
#include "Core\IORegisters.h"
#include "Core\GamePakInterface.h"
#include "Core\ARM7TDMI.h"
#include "GBAEmulator.h"

static bool stop = false;

DWORD WINAPI MainThread(LPVOID lpParam);

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GBAEMULATOR, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	SetThreadPriority(CreateThread(NULL, 0, MainThread, NULL, 0, NULL), THREAD_PRIORITY_NORMAL);


    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GBAEMULATOR));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GBAEMULATOR));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_GBAEMULATOR);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Parse the menu selections:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
		stop = true;
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


DWORD WINAPI MainThread(LPVOID lpParam)
{
	FILE* rom = fopen("d:\\Old\\Temp\\MKSC\\Mario Kart Super Circuit (U).gba", "rb");
	fseek(rom, 0, SEEK_END);
	long size = ftell(rom);
	uint8_t* data = (uint8_t*)malloc(size);
	fseek(rom, 0, SEEK_SET);
	fread(data, 1, size, rom);
	fclose(rom);
	FILE* biosFile = fopen("d:\\Projects\\Visual Studio 2015\\GBAEmulator\\Debug\\GBA.ROM", "rb");
	fseek(biosFile, 0, SEEK_END);
	size = ftell(biosFile);
	uint8_t* biosData = (uint8_t*)malloc(size);
	fseek(biosFile, 0, SEEK_SET);
	fread(biosData, 1, size, biosFile);
	fclose(biosFile);
	MemoryBus* memoryBus = new MemoryBus();
	BIOS* bios = new BIOS(biosData);
	memoryBus->SetDevice(bios, 0);
	WRAM* wram = new WRAM();
	memoryBus->SetDevice(wram, 3);
	IORegisters* ioRegisters = new IORegisters();
	memoryBus->SetDevice(ioRegisters, 4);
	GamePakInterface* gamePak = new GamePakInterface(data);
	memoryBus->SetDevice(gamePak, 8);
	memoryBus->SetDevice(gamePak, 9);
	memoryBus->SetDevice(gamePak, 10);
	memoryBus->SetDevice(gamePak, 11);
	memoryBus->SetDevice(gamePak, 12);
	memoryBus->SetDevice(gamePak, 13);
	memoryBus->SetDevice(gamePak, 14);
	ARM7TDMI* processor = new ARM7TDMI(memoryBus, 0x08000000);
	while (!stop)
	{
		memoryBus->RunCycle();
		processor->RunCycle();
	}
	delete processor;
	delete gamePak;
	delete ioRegisters;
	delete wram;
	delete memoryBus;
	free(data);
	return 0;
}