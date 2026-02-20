// dllmain.cpp : Define o ponto de entrada para o aplicativo DLL.
#include <stdio.h>
#include "Hooks.h"
#include "minhook.h"
#include "OverlayServer.h"


DWORD WINAPI MainThread(LPVOID lpThreadParameter)
{
    Hooks::hModule = (HMODULE)lpThreadParameter;

#ifdef _DEBUG
    printf("WoWpadXOverlay main thread initialized. Starting hooks...\n");
#endif

    Hooks::Initialize();
    return TRUE;
}

DWORD WINAPI ExitThread(LPVOID lpThreadParameter)
{
    if (!Hooks::detached)
    {
        Hooks::detached = true;
        FreeLibraryAndExitThread(Hooks::hModule, TRUE);
    }
    return TRUE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
#ifdef _DEBUG 
    AllocConsole();
    freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
#endif

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);

        if (MH_Initialize() != MH_OK)
        {
            return FALSE;
        }

        CreateThread(nullptr, 0, MainThread, hModule, 0, nullptr);
        OverlayServer::Start();

        break;

    case DLL_PROCESS_DETACH:
        MH_Uninitialize();
        if (!Hooks::detached)
            CreateThread(nullptr, 0, ExitThread, hModule, 0, nullptr);
        OverlayServer::Stop();

        break;
    }

    return TRUE;
}