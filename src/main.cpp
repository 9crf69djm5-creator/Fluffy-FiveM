#include <Windows.h>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <string>
#include <tlhelp32.h>
#include <conio.h>

#include <Gui/Overlay/Overlay.hpp>
#include <Core/Core.hpp>
#include <Security/AntiCrack.hpp>
#include <Auth/auth.hpp>
#pragma comment(lib, "Auth/library_x64.lib")
#include <Security/xorstr.hpp>
#include "Core/Update.hpp"

using namespace Core;
using namespace KeyAuth;

auto name = xorstr_("FLuffyFiveM");
auto ownerid = xorstr_("xY49P74OIm");
auto version = xorstr_("1.0");
auto url = xorstr_("https://keyauth.win/api/1.3/");
auto path = xorstr_("");

api KeyAuthApp(name.crypt_get(), ownerid.crypt_get(), version.crypt_get(), url.crypt_get(), path.crypt_get());

namespace Crash {
    LONG WINAPI AntiCrash(EXCEPTION_POINTERS* pExceptionPointers) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    SetUnhandledExceptionFilter(Crash::AntiCrash);

    AllocConsole();
    FILE* fDummy = nullptr;
    freopen_s(&fDummy, "CONIN$", "r", stdin);
    freopen_s(&fDummy, "CONOUT$", "w", stderr);
    freopen_s(&fDummy, "CONOUT$", "w", stdout);

    std::cout << xorstr_("\n [KeyAuth] Connecting...\n").crypt_get();
    KeyAuthApp.init();
    if (!KeyAuthApp.response.success) {
        std::cout << xorstr_(" [KeyAuth] Init failed: ").crypt_get() << KeyAuthApp.response.message << "\n";
        Sleep(4000);
        return 0;
    }

    Core::Update::CheckUpdate();
    if (Core::Update::UpdateAvailable) {
        std::cout << xorstr_(" [Update] New version: ").crypt_get() << Core::Update::LatestVersion
                  << xorstr_("  Open: ").crypt_get() << Core::Update::UpdateUrl << "\n";
    }

    std::string key;
    std::ifstream infile("license.txt");
    if (infile.good()) {
        std::getline(infile, key);
        infile.close();
        std::cout << xorstr_(" [KeyAuth] Using license from license.txt\n").crypt_get();
        KeyAuthApp.license(key);
    } else {
        std::cout << xorstr_(" [KeyAuth] Enter license key: ").crypt_get();
        std::getline(std::cin, key);
        KeyAuthApp.license(key);
        if (KeyAuthApp.response.success) {
            std::ofstream outfile("license.txt");
            if (outfile.good())
                outfile << key;
        }
    }

    if (!KeyAuthApp.response.success) {
        std::cout << xorstr_(" [KeyAuth] Failed: ").crypt_get() << KeyAuthApp.response.message << "\n";
        Sleep(3000);
        return 0;
    }

    g_MenuInfo.IsLogged = true;
    g_MenuInfo.login_success = true;
    std::cout << xorstr_(" [KeyAuth] Logged in.\n").crypt_get();
    std::cout << xorstr_(" Waiting for FiveM (grcWindow)...\n").crypt_get();

    HANDLE hProc = GetCurrentProcess();
    if (!SetPriorityClass(hProc, ABOVE_NORMAL_PRIORITY_CLASS)) {
    }

    if (!Mem.GetMaxPrivileges(hProc))
        return 0;

    while (!g_Variables.g_hGameWindow)
    {
        g_Variables.g_hGameWindow = FindWindowA(xorstr("grcWindow"), nullptr);
        if (g_Variables.g_hGameWindow)
        {
            auto WindowInfo = Utils::GetWindowPosAndSize(g_Variables.g_hGameWindow);
            g_Variables.g_vGameWindowSize = WindowInfo.second;
            g_Variables.g_vGameWindowPos = WindowInfo.first;
            g_Variables.g_vGameWindowCenter = {
                g_Variables.g_vGameWindowSize.x / 2,
                g_Variables.g_vGameWindowSize.y / 2
            };
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    FreeConsole();

    GetWindowThreadProcessId(g_Variables.g_hGameWindow, &g_Variables.ProcIdFiveM);

//    std::thread(&AntiCrack::DoProtect).detach();

    if (!Core::SetupOffsets())
        return 0;

    Gui::cOverlay.Render();

    return 0;
}
