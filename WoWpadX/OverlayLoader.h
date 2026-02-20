#pragma once
#include "Windows.h" 
#include <string>


namespace OverlayLoader
{
	bool Load(DWORD pid, const std::string& dllPath);
}