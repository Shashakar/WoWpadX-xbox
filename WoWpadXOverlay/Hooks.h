#pragma once
#include "windows.h" 
#include <string>

namespace Hooks
{
	extern HMODULE hModule;
	extern void* pDevice;
	extern bool detached;


	extern HWND hWindow;
	extern bool InitImGui;

	extern bool renderCrosshair;
	extern int crossX;
	extern int crossY;

	void Initialize();
	void SendNotification(std::string& title, std::string& content, int unique_id, int duration, int imageID);
}