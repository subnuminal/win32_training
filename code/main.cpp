#include <Windows.h>
#include <stdint.h>

#define global_variable static
#define internal static

struct win32_dimensions {
    int width;
    int height;
};

struct win32_back_buffer {
    void* memory;
    BITMAPINFO info;
    int bytesPerPixel;
    int width;
    int height;
};

global_variable bool IsRunning = false;
global_variable win32_back_buffer globalBackBuffer;
global_variable uint8_t globalRed = 0;
global_variable uint8_t globalGreen = 85;
global_variable uint8_t globalBlue = 160;

internal void Win32FillBackBuffer(uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t* row = (uint8_t*)globalBackBuffer.memory;
    int pitch = globalBackBuffer.width * globalBackBuffer.bytesPerPixel;
    for (int y = 0; y < globalBackBuffer.height; y++) {
        uint32_t* pixel = (uint32_t*)row;
        for (int x = 0; x < globalBackBuffer.width; x++) {
            uint8_t padding = 0;
            *pixel = ((padding << 24)|(red << 16)|(green << 8)|blue);
            pixel++;
        }
        row += pitch;
    }
}

internal void Win32RebuildBackBuffer(HWND window, int width, int height) {
    globalBackBuffer.bytesPerPixel = 4;
    globalBackBuffer.width = width;
    globalBackBuffer.height = height;
    
    if (globalBackBuffer.memory) {
        if (!VirtualFree(globalBackBuffer.memory, 0, MEM_RELEASE)) {
            OutputDebugStringW(L"failed to free memory\n");
        }
    }

    int memSize = globalBackBuffer.width * globalBackBuffer.height * globalBackBuffer.bytesPerPixel;
    globalBackBuffer.memory = VirtualAlloc(0, memSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    BITMAPINFOHEADER* hdr = &globalBackBuffer.info.bmiHeader;
    hdr->biSize = sizeof(BITMAPINFOHEADER);
    hdr->biWidth = globalBackBuffer.width;
    hdr->biHeight = -globalBackBuffer.height; // negative for top down dib
    hdr->biPlanes = 1;
    hdr->biBitCount = 32;
    hdr->biCompression = BI_RGB;
}

internal void Win32DisplayBuffer(HDC dc, int windowWidth, int windowHeight, win32_back_buffer globalBackBuffer) {
    StretchDIBits(
        dc,
        0, 0, windowWidth, windowHeight,
        0, 0, globalBackBuffer.width, globalBackBuffer.height,
        globalBackBuffer.memory,
        &(globalBackBuffer.info),
        DIB_RGB_COLORS,
        SRCCOPY);
}

internal win32_dimensions Win32GetDimensions(HWND window) {
    RECT r;
    GetClientRect(window, &r);
    win32_dimensions dim = {};
    dim.width = r.right - r.left;
    dim.height = r.bottom - r.top;
    return dim;
}

LRESULT Win32MainWindowProc(
    HWND   hwnd,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam) {
        LRESULT result = 0;

        switch (uMsg)
        {
            case WM_ACTIVATEAPP:{
                OutputDebugStringW(L"WM_ACTIVATEAPP\n");
            } break;

            case WM_CLOSE:{
                OutputDebugStringW(L"WM_CLOSE\n");
                IsRunning = false;
            } break;

            case WM_DESTROY:{
                OutputDebugStringW(L"WM_DESTROY\n");
                IsRunning = false;
            } break;

            case WM_PAINT:{
                PAINTSTRUCT p;
                HDC dc = BeginPaint(hwnd, &p);
                int width = p.rcPaint.right - p.rcPaint.left;
                int height = p.rcPaint.bottom - p.rcPaint.top;
                Win32FillBackBuffer(globalRed, globalGreen, globalBlue);
                Win32DisplayBuffer(dc, width, height, globalBackBuffer);
                EndPaint(hwnd, &p);
            } break;
        
            default:{
                result = DefWindowProcW(hwnd, uMsg, wParam, lParam);
            } break;
        }

        return result;
}

int WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nShowCmd) {
        WNDCLASSEXW winc = {};
        winc.cbSize = sizeof(WNDCLASSEXW);
        winc.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
        winc.lpfnWndProc = Win32MainWindowProc;
        winc.hInstance = hInstance;
        winc.lpszClassName = L"Training\n";

        if(RegisterClassExW(&winc)) {
            HWND window = CreateWindowExW(
                0,
                winc.lpszClassName,
                L"Training Game\n",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                hInstance,
                0);
            
            if (window) {
                IsRunning = true;
                HDC dc = GetDC(window);

                int offset = 0;

                Win32RebuildBackBuffer(window, 1280, 720);

                while(IsRunning) {
                    MSG msg;
                    while(PeekMessage(&msg, window, 0, 0, PM_REMOVE)) {
                        if (msg.message == WM_QUIT) {
                            IsRunning = false;
                        }
                        TranslateMessage(&msg);
                        DispatchMessage(&msg);
                    }
                    Win32FillBackBuffer((globalRed + offset), (globalGreen + offset), (globalBlue + offset));
                    win32_dimensions windowDim = Win32GetDimensions(window);
                    Win32DisplayBuffer(dc, windowDim.width, windowDim.height, globalBackBuffer);

                    offset++;
                }
            } else {
                OutputDebugStringW(L"failed to create window\n");
            }
        }
}