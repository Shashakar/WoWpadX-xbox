#include "Hooks.h"
#include "minhook.h"
#include "d3d9.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h" 
#include "ImGuiNotify/iconsfontawesome6.h" 
#include "ImGuiNotify/imguinotify.hpp" 
#include "ImGuiNotify/fa_solid_900.h"

#include <D3dx9tex.h>
#include "resource.h"
#pragma comment(lib, "D3dx9")


static bool _randSeeded = false;
 
HMODULE Hooks::hModule = NULL;
bool Hooks::detached = false;
HWND Hooks::hWindow = NULL;
bool Hooks::InitImGui = false;

bool Hooks::renderCrosshair = false;
int Hooks::crossX = 0;
int Hooks::crossY = 0;


void* Hooks::pDevice = NULL;


ImTextureID g_imgBackground = 0;
ImTextureID g_imgBottom = 0;
ImTextureID g_imgHeader = 0;
ImTextureID g_imgIconSuccess = 0;
ImTextureID g_imgIconWarning = 0;
ImTextureID g_imgCrosshair = 0;


typedef HRESULT(__stdcall* EndSceneFn)(IDirect3DDevice9* pDevice);
EndSceneFn oEndScene = nullptr;

typedef HRESULT(__stdcall* ResetFn)(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* param);
ResetFn oReset = nullptr;


HWND tempWnd = NULL;

BOOL CALLBACK EnumWindowsCallback(HWND handle, LPARAM lParam)
{
    DWORD wndProcId = 0;
    GetWindowThreadProcessId(handle, &wndProcId);

    if (GetCurrentProcessId() != wndProcId)
        return TRUE;

    tempWnd = handle;
    return FALSE;
}

HWND GetProcessWindow()
{
    tempWnd = (HWND)NULL;
    EnumWindows(EnumWindowsCallback, NULL);
    return tempWnd;
}

bool UpdateWindow()
{
	HWND wnd = GetProcessWindow();

	if (wnd != Hooks::hWindow)
	{
		Hooks::hWindow = wnd;
		/* Delete imgui to avoid errors */
		if (Hooks::InitImGui)
		{
			ImGui_ImplDX9_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();

			Hooks::InitImGui = false;
		}
		return true;
	}

	return false;
}

bool IsWindowValid(HWND hwnd)
{
	return hwnd && IsWindow(hwnd) && IsWindowVisible(hwnd);
}

bool LoadTextureFromResource(int resourceId, const wchar_t* resourceType, PDIRECT3DTEXTURE9* out_texture, int* out_width, int* out_height)
{
	HRSRC hResInfo = FindResource(Hooks::hModule, MAKEINTRESOURCE(resourceId), resourceType);
	if (!hResInfo)	return false;

	HGLOBAL hResData = LoadResource(Hooks::hModule, hResInfo);
	if (!hResData)	return false;

	void* pResData = LockResource(hResData);
	DWORD resSize = SizeofResource(Hooks::hModule, hResInfo);
	if (!pResData || resSize == 0)	return false;

	ID3DXBuffer* buffer = nullptr;
	HRESULT hr = D3DXCreateTextureFromFileInMemory((LPDIRECT3DDEVICE9)Hooks::pDevice, pResData, resSize, out_texture);
	if (FAILED(hr))
		return false;

	if (out_width || out_height) {
		D3DSURFACE_DESC desc;
		(*out_texture)->GetLevelDesc(0, &desc);
		if (out_width) *out_width = desc.Width;
		if (out_height) *out_height = desc.Height;
	}

	return true;
}

void SafeReleaseTexture(PDIRECT3DTEXTURE9& tex)
{
	if (tex) {
		tex->Release();
		tex = nullptr;
	}
}


void LoadTextures()
{
	// Release old textures if they exist
	SafeReleaseTexture((PDIRECT3DTEXTURE9&)g_imgBackground);
	SafeReleaseTexture((PDIRECT3DTEXTURE9&)g_imgBottom);
	SafeReleaseTexture((PDIRECT3DTEXTURE9&)g_imgHeader);
	SafeReleaseTexture((PDIRECT3DTEXTURE9&)g_imgIconSuccess);
	SafeReleaseTexture((PDIRECT3DTEXTURE9&)g_imgIconWarning);
	SafeReleaseTexture((PDIRECT3DTEXTURE9&)g_imgCrosshair);


	PDIRECT3DTEXTURE9 bg = NULL;
	PDIRECT3DTEXTURE9 bgBottom = NULL;
	PDIRECT3DTEXTURE9 imgHeader = NULL;
	PDIRECT3DTEXTURE9 imgIcnSucc = NULL;
	PDIRECT3DTEXTURE9 imgIcnWrn = NULL;
	PDIRECT3DTEXTURE9 cross = NULL;

	LoadTextureFromResource(IDB_PNG_BG, L"PNG", &bg, NULL, NULL);
	LoadTextureFromResource(IDB_PNG_BGBOTTOM, L"PNG", &bgBottom, NULL, NULL);
	LoadTextureFromResource(IDB_PNG_HEADER, L"PNG", &imgHeader, NULL, NULL);
	LoadTextureFromResource(IDB_PNG_ILLIDAN, L"PNG", &imgIcnSucc, NULL, NULL);
	LoadTextureFromResource(IDB_PNG_RHONIN, L"PNG", &imgIcnWrn, NULL, NULL);
	LoadTextureFromResource(IDB_PNG_CROSSHAIR, L"PNG", &cross, NULL, NULL);

	g_imgBackground = (ImTextureID)bg;
	g_imgBottom = (ImTextureID)bgBottom;
	g_imgHeader = (ImTextureID)imgHeader;
	g_imgIconSuccess = (ImTextureID)imgIcnSucc;
	g_imgIconWarning = (ImTextureID)imgIcnWrn;
	g_imgCrosshair = (ImTextureID)cross;
}

HRESULT __stdcall detoured_EndScene(IDirect3DDevice9* pDevice)
{
	Hooks::pDevice = (void*)pDevice;

	if (UpdateWindow())
	{
		return oEndScene(pDevice);
	}

	if (pDevice->TestCooperativeLevel() == D3DERR_DEVICELOST || !IsWindowValid(Hooks::hWindow))
	{
		return oEndScene(pDevice);
	}

	if ((!IsWindow(Hooks::hWindow) || !IsWindowVisible(Hooks::hWindow)) || GetForegroundWindow() != Hooks::hWindow)
	{
		return oEndScene(pDevice);
	}

	if (!Hooks::InitImGui)
	{
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;

		ImGui_ImplWin32_Init(Hooks::hWindow);
		ImGui_ImplDX9_Init(pDevice);

		io.Fonts->AddFontDefault();


		ImFontConfig font_cfg;
		font_cfg.FontDataOwnedByAtlas = false;
		io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 17.f, &font_cfg);

		// Initialize notify
		static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

		ImFontConfig icons_config;
		icons_config.MergeMode = true;
		icons_config.PixelSnapH = true;
		icons_config.FontDataOwnedByAtlas = false;


		io.Fonts->AddFontFromMemoryTTF((void*)fa_solid_900, sizeof(fa_solid_900), 16.f, &icons_config, icons_ranges);

		LoadTextures();


		Hooks::InitImGui = true;
	}

	if (!Hooks::InitImGui) return oEndScene(pDevice);

	ImGuiIO& io = ImGui::GetIO();

	RECT rect;
	GetClientRect(Hooks::hWindow, &rect);
	io.DisplaySize = ImVec2(
		(float)(rect.right - rect.left),
		(float)(rect.bottom - rect.top)
	);

	static INT64 frequency = 0;
	static INT64 last_time = 0;

	if (!frequency)
	{
		QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);
		QueryPerformanceCounter((LARGE_INTEGER*)&last_time);
	}

	INT64 current_time;
	QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
	io.DeltaTime = (float)(current_time - last_time) / frequency;
	last_time = current_time;



	ImGui_ImplDX9_NewFrame();
	ImGui::NewFrame();


	if (Hooks::renderCrosshair)
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		// Size of the crosshair image
		const float crossSize = 64.0f;
		const float halfSize = crossSize * 0.5f;

		float crossX = static_cast<float>(Hooks::crossX);
		float crossY = static_cast<float>(Hooks::crossY);

		ImVec2 topLeft = ImVec2(crossX - halfSize, crossY - halfSize);
		ImVec2 bottomRight = ImVec2(crossX + halfSize, crossY + halfSize);

		ImGui::GetBackgroundDrawList(viewport)->AddImage(
			g_imgCrosshair,
			topLeft,
			bottomRight
		);
	}


	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 1.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);

	// Notifications color setup
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.10f, 0.10f, 0.10f, 0.00f));
	ImGui::RenderNotifications(g_imgBackground, g_imgBottom, g_imgHeader, g_imgIconSuccess);

	ImGui::PopStyleVar(2);
	ImGui::PopStyleColor(1);

	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

    return oEndScene(pDevice);
}

HRESULT __stdcall detoured_Reset(LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* param)
{
	HRESULT result = NULL;
	if (Hooks::InitImGui)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		result = oReset(pDevice, param);
		ImGui_ImplDX9_CreateDeviceObjects();
	}

	return result != NULL ? result : oReset(pDevice, param);

}
// Hook initialization (early hook)

typedef HRESULT(STDMETHODCALLTYPE* PFN_CreateDevice)(
	IDirect3D9* self, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
	DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* ppp, IDirect3DDevice9** ppDev);

static PFN_CreateDevice oCreateDevice = nullptr;

HRESULT STDMETHODCALLTYPE hkCreateDevice(
	IDirect3D9* self, UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow,
	DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* ppp, IDirect3DDevice9** ppDev)
{
	HRESULT hr = oCreateDevice(self, Adapter, DeviceType, hFocusWindow, BehaviorFlags, ppp, ppDev);
	if (SUCCEEDED(hr) && ppDev && *ppDev)
	{
		void** vtbl = *reinterpret_cast<void***>(*ppDev);
		void* pEndScene = vtbl[42];
		void* pReset = vtbl[16];

		MH_CreateHook(pEndScene, &detoured_EndScene, reinterpret_cast<LPVOID*>(&oEndScene));
		MH_EnableHook(pEndScene);

		MH_CreateHook(pReset, &detoured_Reset, reinterpret_cast<LPVOID*>(&oReset));
		MH_EnableHook(pReset);
	}
	return hr;
}

typedef IDirect3D9* (WINAPI* PFN_Direct3DCreate9)(UINT);
static PFN_Direct3DCreate9 oDirect3DCreate9 = nullptr;

IDirect3D9* WINAPI hkDirect3DCreate9(UINT SDKVersion)
{
	IDirect3D9* pD3D = oDirect3DCreate9(SDKVersion);

	if (pD3D)
	{
		void** vtbl = *reinterpret_cast<void***>(pD3D);
		void* pCreateDevice = vtbl[16]; // IDirect3D9::CreateDevice

		MH_CreateHook(pCreateDevice, &hkCreateDevice, reinterpret_cast<LPVOID*>(&oCreateDevice));
		MH_EnableHook(pCreateDevice);
	}

	return pD3D;
}

void HookDirect3DCreate9()
{
	HMODULE hD3D9 = GetModuleHandleA("d3d9.dll"); 
	auto pCreate9 = GetProcAddress(hD3D9, "Direct3DCreate9");
	if (pCreate9)
	{
		MH_CreateHook(pCreate9, &hkDirect3DCreate9, reinterpret_cast<LPVOID*>(&oDirect3DCreate9));
		MH_EnableHook(pCreate9);
	}
}


DWORD WINAPI HookEndScene()
{
	// Create a dummy Direct3D9 device
	IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION);
	if (!pD3D) return false;

	IDirect3DDevice9* pDevice = nullptr;
	D3DPRESENT_PARAMETERS d3dpp = {};
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = GetProcessWindow();

	// Create a temporary device
	if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow,
		D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice) != D3D_OK)
	{
		d3dpp.Windowed = !d3dpp.Windowed;

		if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dpp.hDeviceWindow,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &pDevice) != D3D_OK)
		{
			pD3D->Release();
			return false;
		}
	}

	// Get EndScene function pointer
	void** vtable = *reinterpret_cast<void***>(pDevice);
	void* pEndScene = vtable[42];
	void* pReset = vtable[16];

	// Clean up temporary device
	pDevice->Release();
	pD3D->Release();

	if (MH_CreateHook(pEndScene, &detoured_EndScene, reinterpret_cast<LPVOID*>(&oEndScene)) != MH_OK)
	{
		return false;
	}

	if (MH_EnableHook(pEndScene) != MH_OK)
	{
		return false;
	}

	if (MH_CreateHook(pReset, &detoured_Reset, reinterpret_cast<LPVOID*>(&oReset)) != MH_OK)
	{
		return false;
	}

	if (MH_EnableHook(pReset) != MH_OK)
	{
		return false;
	}


	return true;
}

void Hooks::SendNotification(std::string& title, std::string& content, int unique_id, int duration, int imageID)
{
	ImGuiToast toast(ImGuiToastType::None, duration);

	toast.setTitle(title.c_str());
	toast.setContent(content.c_str());

	if (imageID < 0 || imageID > 1)
	{

		if (!_randSeeded) {
			srand(static_cast<unsigned int>(time(nullptr)));
			_randSeeded = true;
		}

		imageID = rand() % 2;
	}

	switch (imageID)
	{
	case 0:  
		toast.setIconTexture(g_imgIconSuccess);
		break;
	case 1:
		toast.setIconTexture(g_imgIconWarning);
		break;
	}


	ImGui::InsertNotification(toast);
}

void Hooks::Initialize()
{
	// Try to hook immediately using dummy device (late injection case)
	if (!HookEndScene())
	{

		// 1) Wait for d3d9.dll to be loaded by the game
		while ((GetModuleHandleA("d3d9.dll")) == nullptr) {
			Sleep(100);
		}
		// If that failed, set up early injection hook via Direct3DCreate9
		HookDirect3DCreate9();
	}
}