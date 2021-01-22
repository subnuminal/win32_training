#include <Windows.h>
#include <stdint.h>
#include <Xinput.h>

#define global_variable static
#define internal static

#define bool32 int32_t

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
    return ERROR_DEVICE_NOT_CONNECTED;
}
x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

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

struct win32_xinput_gamepad {
    bool32 buttonA;
    bool32 buttonB;
    bool32 buttonX;
    bool32 buttonY;
};

global_variable bool IsRunning = false;
global_variable win32_back_buffer globalBackBuffer;
global_variable uint8_t globalRed = 0;
global_variable uint8_t globalGreen = 85;
global_variable uint8_t globalBlue = 160;

internal void Win32InitXInput() {
    HMODULE xInput = LoadLibraryExW(L"xinput1_4.dll", 0, 0);
    if (!xInput) {
        xInput = LoadLibraryExW(L"xinput1_3.dll", 0, 0);
    }

    if (!xInput) {
        xInput = LoadLibraryExW(L"xinput9_1_0.dll", 0, 0);
    }

    if (xInput) {
        XInputGetState = (x_input_get_state*)GetProcAddress(xInput, "XInputGetState");
        XInputSetState = (x_input_set_state*)GetProcAddress(xInput, "XInputSetState");
    }
}

internal XINPUT_STATE* Win32ReadUserXInput() {
    DWORD dwResult;    
    for (DWORD i=0; i< XUSER_MAX_COUNT; i++ ) {
        XINPUT_STATE state;
        ZeroMemory( &state, sizeof(XINPUT_STATE) );

        // Simply get the state of the controller from XInput.
        dwResult = XInputGetState( i, &state );

        if( dwResult == ERROR_SUCCESS ) {
            return &state;
        }
    }

    return 0;
}

internal win32_xinput_gamepad Win32GetUserXInput() {
    XINPUT_STATE* state = Win32ReadUserXInput();

    win32_xinput_gamepad gamepad = {};

    if (state) {
        gamepad.buttonA = state->Gamepad.wButtons & XINPUT_GAMEPAD_A;
        gamepad.buttonB = state->Gamepad.wButtons & XINPUT_GAMEPAD_B;
        gamepad.buttonX = state->Gamepad.wButtons & XINPUT_GAMEPAD_X;
        gamepad.buttonY = state->Gamepad.wButtons & XINPUT_GAMEPAD_Y;
    }

    return gamepad;
}

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
        
        Win32InitXInput();

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
                int redOffset = 0;
                int greenOffset = 0;
                int blueOffset = 0;

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

                    win32_xinput_gamepad gamepad = Win32GetUserXInput();

                    if (gamepad.buttonA) {
                        redOffset++;
                    }
                    if (gamepad.buttonB) {
                        greenOffset++;
                    }
                    if (gamepad.buttonX) {
                        blueOffset++;
                    }
                    if (gamepad.buttonY) {
                        offset++;
                    }

                    Win32FillBackBuffer((globalRed + redOffset + offset), (globalGreen + greenOffset + offset), (globalBlue + blueOffset + offset));
                    win32_dimensions windowDim = Win32GetDimensions(window);
                    Win32DisplayBuffer(dc, windowDim.width, windowDim.height, globalBackBuffer);
                }
            } else {
                OutputDebugStringW(L"failed to create window\n");
            }
        }
}