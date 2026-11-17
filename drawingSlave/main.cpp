// Epic Pen Content Surface: \\.\DISPLAY2 or DISPLAY1
// Epic Pen Toolbar -> close

#include <cstdint>
#include <memoryapi.h>
#include <windows.h>
#include <print>
#include <array>
#include <chrono>
using namespace std::chrono;
#include <uxtheme.h>
#include <dwmapi.h>

#include <d3d11.h>
#include <winreg.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

ID3D11Device* D3DDevice = nullptr;
ID3D11DeviceContext* D3DDeviceContext = nullptr;
IDXGISwapChain* swapChain = nullptr;
ID3D11RenderTargetView* mainRenderTargetView = nullptr;

std::array<float, 4> targetViewColor = { 0.0f, 0.0f, 0.0f, 0.0f };

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    D3DDevice->CreateRenderTargetView(pBackBuffer, nullptr, &mainRenderTargetView);
    pBackBuffer->Release(); // <- chybí
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 0;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel{};
    std::array<D3D_FEATURE_LEVEL, 2> featureLevelArray = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray.data(), 2, D3D11_SDK_VERSION, &sd, &swapChain, &D3DDevice, &featureLevel, &D3DDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray.data(), 2, D3D11_SDK_VERSION, &sd, &swapChain, &D3DDevice, &featureLevel, &D3DDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}
static void FeedMouseToImGui(HWND hwnd)
{
    ImGuiIO& io = ImGui::GetIO();

    POINT p{};
    if (GetCursorPos(&p))
    {
        ScreenToClient(hwnd, &p);
        io.AddMousePosEvent((float)p.x, (float)p.y);
    }

    const bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
    const bool rmb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    const bool mmb = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;

    io.AddMouseButtonEvent(0, lmb);
    io.AddMouseButtonEvent(1, rmb);
    io.AddMouseButtonEvent(2, mmb);
}

void startDraw() {
  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();
}
void endDraw() {
  ImGui::Render();

  D3DDeviceContext->OMSetRenderTargets(1, &mainRenderTargetView, nullptr);
  D3DDeviceContext->ClearRenderTargetView(mainRenderTargetView, targetViewColor.data());
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

  swapChain->Present(0, 0);
}

bool closeWindow(LPCWSTR windowTitle) {
  HWND windowHandle = FindWindowW(nullptr, windowTitle);
  if (windowHandle == NULL) {
      return false;
  }
    
  if (!PostMessageW(windowHandle, WM_CLOSE, 0, 0)) {
      std::println("WM_CLOSE failed, error: {}", GetLastError());
      return false;
  }

  return true;
}

void terminateDrawing(){
	startDraw();
	endDraw();
    
	ImGui_ImplDX11_Shutdown();
  ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();

	if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = nullptr; }
	if (swapChain) { swapChain->Release(); swapChain = nullptr; }
	if (D3DDeviceContext) { D3DDeviceContext->Release(); D3DDeviceContext = nullptr; }
	if (D3DDevice) { D3DDevice->Release(); D3DDevice = nullptr; }
}

enum class opCodes : uint32_t {
  read  = 0x05,
  write = 0x10,
  base  = 0x15,
	hideOverlay = 0x20,
  cr3   = 0x47,
	init = 0x99,
	end = 0x100
};

struct umRequest {
  opCodes  code;
	uint32_t bytesToRW;
	uint64_t address;
};

struct vec3_t{
	float x;
	float y;
	float z;
};

uint64_t controlFuncAddr = 0;
umRequest* sharedBuffer = 0;

__int64(__fastcall* NtMITDeactivateInputProcessing)() = nullptr;

template<typename T>
T driverRead(uint64_t VA) {
	
	if(!NtMITDeactivateInputProcessing)
    return {};

  __int64 ret = 0;

	sharedBuffer->address = VA;
	sharedBuffer->bytesToRW = sizeof(T);
	sharedBuffer->code = opCodes::read;

  asm volatile(
    "xor %%rcx, %%rcx     \n\t"   // RCX = 0
    "mov %[addr], %%rdx   \n\t"   // RDX = controlFuncAddr
    "call *%[f]           \n\t"   // call fn
    : "=a"(ret)
    : [f]"r"(NtMITDeactivateInputProcessing),
      [addr]"r"(controlFuncAddr)
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory"
  );

  auto* payload = reinterpret_cast<const uint8_t*>(sharedBuffer) + sizeof(umRequest);
	return *reinterpret_cast<const T*>(payload);
}

bool driverGetCr3(const char* name) {
	
	if(!NtMITDeactivateInputProcessing)
    return 0;

  __int64 ret;
  
	auto* payload = reinterpret_cast<char*>(reinterpret_cast<uint8_t*>(sharedBuffer) + sizeof(umRequest));

  constexpr size_t kPageSize = 0x1000;
  const size_t cap = kPageSize - sizeof(umRequest);

  const size_t len = strnlen(name, cap - 1);
  std::memcpy(payload, name, len);
	payload[len] = '\0';
	
	sharedBuffer->bytesToRW = len + 1;
	sharedBuffer->code = opCodes::cr3;

  asm volatile(
    "xor %%rcx, %%rcx     \n\t"   // RCX = 0
    "mov %[addr], %%rdx   \n\t"   // RDX = controlFuncAddr
    "call *%[f]           \n\t"   // call fn
    : "=a"(ret)
    : [f]"r"(NtMITDeactivateInputProcessing),
      [addr]"r"(controlFuncAddr)
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory"
  );

	return 1;
}

uint64_t driverGetBase(const wchar_t* name) {
	if(!NtMITDeactivateInputProcessing)
		return 0;
	
	__int64 ret;

	sharedBuffer->code = opCodes::base;
	auto* payload = reinterpret_cast<uint8_t*>(sharedBuffer) + sizeof(umRequest);

	if(!name || name[0] == '\0'){
		payload[0] = 0xDE;
		payload[1] = 0xAD;
	}
	else
	{
    constexpr size_t kPageSize = 0x1000;
    const size_t capBytes = kPageSize - sizeof(umRequest);
		const size_t capWchars = capBytes / sizeof(wchar_t);
  	
		size_t n = 0;
    while (n + 1 < capWchars && name[n] != L'\0')
    	++n;
		auto payloadW = reinterpret_cast<wchar_t*>(payload);
    std::memset(payloadW, 0, capWchars * sizeof(wchar_t));
		std::memcpy(payloadW, name, n * sizeof(wchar_t));
		payloadW[n] = L'\0';
	}
	
	asm volatile(
    "xor %%rcx, %%rcx     \n\t"   // RCX = 0
    "mov %[addr], %%rdx   \n\t"   // RDX = controlFuncAddr
    "call *%[f]           \n\t"   // call fn
    : "=a"(ret)
    : [f]"r"(NtMITDeactivateInputProcessing),
      [addr]"r"(controlFuncAddr)
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory"
  );

	return *reinterpret_cast<uint64_t*>(payload);
}

bool driverHide(HWND hwnd) {
	
	if(!NtMITDeactivateInputProcessing)
    return 0;

  __int64 ret = 0;

	sharedBuffer->code = opCodes::hideOverlay;
	sharedBuffer->address = reinterpret_cast<uint64_t>(hwnd);

  asm volatile(
    "xor %%rcx, %%rcx     \n\t"   // RCX = 0
    "mov %[addr], %%rdx   \n\t"   // RDX = controlFuncAddr
    "call *%[f]           \n\t"   // call fn
    : "=a"(ret)
    : [f]"r"(NtMITDeactivateInputProcessing),
      [addr]"r"(controlFuncAddr)
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory"
  );

	return ret;
}

void driverStop(){
	if(!NtMITDeactivateInputProcessing)
    return;

  __int64 ret;

	sharedBuffer->code = opCodes::end;

  asm volatile(
    "xor %%rcx, %%rcx     \n\t"   // RCX = 0
    "mov %[addr], %%rdx   \n\t"   // RDX = controlFuncAddr
    "call *%[f]           \n\t"   // call fn
    : "=a"(ret)
    : [f]"r"(NtMITDeactivateInputProcessing),
      [addr]"r"(controlFuncAddr)
    : "rcx", "rdx", "r8", "r9", "r10", "r11", "memory"
  );
}
int main(int, char**){
    std::println("Hello Underground!");

  LoadLibraryA("user32.dll");
	LoadLibraryA("win32u.dll");

	sharedBuffer = reinterpret_cast<umRequest*>(VirtualAlloc(nullptr, 0x1000, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
	if(!sharedBuffer)
		return 0;

	HKEY hKey{};
	DWORD disposition{};

	LONG result = RegCreateKeyExW(
    HKEY_LOCAL_MACHINE,
    L"SOFTWARE\\ezappka",
    0,
    nullptr,
    0,
    KEY_SET_VALUE,
    nullptr,
    &hKey,
    &disposition
	);

	if (result == ERROR_SUCCESS) {
    uintptr_t addr = reinterpret_cast<uintptr_t>(sharedBuffer);

    RegSetValueExW(
        hKey,
        L"buffah",
        0,
        REG_QWORD,
        reinterpret_cast<const BYTE*>(&addr),
        sizeof(addr)
    );

    RegCloseKey(hKey);
	}

  const HMODULE win32u = GetModuleHandleA("win32u.dll");

  *(FARPROC*)&NtMITDeactivateInputProcessing = GetProcAddress(win32u, "NtMITDeactivateInputProcessing");

  std::println("Waiting for driver initialization...");

  while(sharedBuffer->code != opCodes::init){
		Sleep(200);
  }

	controlFuncAddr = sharedBuffer->address;	
  std::println("Control Function: {:#x}", reinterpret_cast<uint64_t>(controlFuncAddr));
  std::println("Shared Buffer UM: {:#x}", reinterpret_cast<uint64_t>(sharedBuffer));

	system("pause");

	driverGetCr3("sauerbraten.ex");

	system("pause");

	auto baseAddress = driverGetBase(0);

	std::println("base: {:#x}", baseAddress);


	auto magicBytes = driverRead<std::array<char, 2>>(baseAddress);
//auto start = high_resolution_clock::now();
	//for(int i = 0; i < 1000000; i++)
	//	magicBytes = driverRead<std::array<char, 2>>(baseAddress);
//auto stop = high_resolution_clock::now();

//auto duration = duration_cast<microseconds>(stop - start);

//std::println("Dur: {}", duration.count());

	std::println("magic: {}", magicBytes);

    HWND windowHandle = FindWindowW(nullptr, L"Epic Pen Content Surface: \\\\.\\DISPLAY1");
    if(!windowHandle){
    	windowHandle = FindWindowW(nullptr, L"Epic Pen Content Surface: \\\\.\\DISPLAY2"); 
    	if(!windowHandle)
				return -1;
		}
		
		if(!driverHide(windowHandle)){
			std::println("Cant hide overlay!");
			
		}
		auto openglBaseAddress = driverGetBase(L"opengl32.dll");
		std::println("opengl base: {:#x}", openglBaseAddress);
    MARGINS margins = { -1 };
    margins.cyBottomHeight = margins.cyTopHeight = margins.cxLeftWidth = margins.cxRightWidth = -1;
    DwmExtendFrameIntoClientArea(windowHandle, &margins);
    SetLayeredWindowAttributes(windowHandle, 0, 255, LWA_ALPHA);

    if(!closeWindow(L"Epic Pen Toolbar")){
        std::println("Cant close toolbar!");
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();

    if (!CreateDeviceD3D(windowHandle)){
        std::println("Cant create device!");
        return -1;
    }

  ImGui_ImplWin32_Init(windowHandle);
  ImGui_ImplDX11_Init(D3DDevice, D3DDeviceContext);

  MSG msg{};

  bool running = true;

	while (running)
	{
		magicBytes = driverRead<std::array<char, 2>>(baseAddress);
		std::println("magic: {}", magicBytes);
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (GetAsyncKeyState(VK_END) & 1)        running = false;

		FeedMouseToImGui(windowHandle);
    startDraw();
		ImGui::ShowDemoWindow();
		ImGuiIO& io = ImGui::GetIO();
 
		if (GetAsyncKeyState(VK_LBUTTON))
			io.MouseDown[0] = true;
		else io.MouseDown[0] = false;;
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    for (int i = 0; i < 5; i++)
        drawList->AddText({100.f + i * 20.f,100.f + i * 20.f}, ImColor(255,0,0), "Hello world");

    endDraw();
	}
	terminateDrawing();
}
