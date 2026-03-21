#pragma once
#include <Includes/Includes.hpp>
#include <Core/Offsets.hpp>
#include <Core/SDK/SDK.hpp>

// Threads
#include "Threads/ScreenResolution.hpp"
#include "Threads/UpdatePointers.hpp"
#include "Threads/UpdateNames.hpp"
#include "Threads/VehicleList.hpp"
#include "Threads/EntityList.hpp"


// Features
#include "Features/MagicBullets.hpp"
#include "Features/Triggerbot.hpp"
#include "Features/SilentAim.hpp"
#include "Features/Aimbot.hpp"
#include "Features/ESP.hpp"
#include "Features/TriggerFinder.hpp"
#include "Features/Spectate.hpp"
#include "Features/Invisibility.hpp"
//Exploits
#include "Features/Exploits/ResourceList.hpp"
#include <Auth/lazyimporter.hpp>

#include <regex>

namespace Core {

	inline static bool ThreadsStarted = false;

	inline void StartThreads() {
		ThreadsStarted = false;

		//Info
		std::thread(&Threads::cScreenResolution::Update, Threads::g_ScreenResolution).detach();
		std::thread(&Threads::cUpdatePtrs::Update, Threads::g_UpdatePtrs).detach();
		std::thread(&Threads::cEntityList::Update, &Threads::g_EntityList).detach();
		std::thread(&Threads::cVehicleList::Update, &Threads::g_VehicleList).detach();
		std::thread(&Threads::cUpdateNames::Update, &Threads::g_UpdateNames).detach();

		//Exploits
		std::thread(&Features::Exploits::cResourceList::List, Features::Exploits::g_ResourceList).detach();

		//Features
		std::thread(&Features::cAimbot::Start, Features::g_Aimbot).detach();
		std::thread(&Features::cSilentAim::HookSilent, Features::g_SilentAim).detach();
		std::thread(&Features::cTriggerbot::Start, Features::g_Triggerbot).detach();
		std::thread(&Features::cMagicBullets::Start, Features::g_MagicBullets).detach();
		std::thread([]() { Core::Features::cInvisibility::Start(); }).detach();
		std::thread([]() { Core::Features::cSpectate::Start(); }).detach();

		ThreadsStarted = true;
	}


	inline std::pair<DWORD, std::wstring> GetProcessPidByName(const std::wstring& processNamePattern) {
		HANDLE hSnapshot = SafeCall(CreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			return { 0, L"" };
		}

		DWORD pid = 0;
		std::wstring foundName;
		PROCESSENTRY32W pe;
		pe.dwSize = sizeof(pe);

		std::wregex pattern(processNamePattern, std::regex_constants::icase);

		if (SafeCall(Process32FirstW)(hSnapshot, &pe)) {
			do {
				std::wstring exeFile = pe.szExeFile;  // مباشرة Unicode
				if (std::regex_match(exeFile, pattern)) {
					pid = pe.th32ProcessID;
					foundName = exeFile;
					break;  // أول تطابق فقط
				}
			} while (SafeCall(Process32NextW)(hSnapshot, &pe));
		}

		SafeCall(CloseHandle)(hSnapshot);
		return { pid, foundName };
	}


	inline std::string trim(const std::string& str) {
		size_t first = str.find_first_not_of(' ');
		if (first == std::string::npos) return "";
		size_t last = str.find_last_not_of(' ');
		return str.substr(first, last - first + 1);
	}

	inline int GetBuild() {
		const wchar_t* localAppData = _wgetenv(L"LOCALAPPDATA");
		if (!localAppData) {
			return -1;
		}

		std::wstring iniFilePath = std::wstring(localAppData) + L"\\FiveM\\FiveM.app\\CitizenFX.ini";
		std::wifstream iniFile(iniFilePath);
		if (!iniFile.is_open()) {
			return -1;
		}

		std::wstring line;
		int buildNumber = -1;

		while (std::getline(iniFile, line)) {

			line.erase(0, line.find_first_not_of(L" \t\r\n"));
			line.erase(line.find_last_not_of(L" \t\r\n") + 1);

			if (line.rfind(L"SavedBuildNumber=", 0) == 0) {
				std::wstring buildStr = line.substr(wcslen(L"SavedBuildNumber="));
				try {
					buildNumber = std::stoi(buildStr);
					break;
				}
				catch (...) {
					return -1;
				}
			}
		}

		iniFile.close();

		if (buildNumber != -1) {

			return buildNumber;
		}
		return 3258;
	}



	inline bool SetupOffsets() {
		g_Offsets.CurrentBuild = GetBuild();
		std::wstring gtaProcessPattern = (L"FiveM_b\\d{4}_GTAProcess\\.exe");
		std::wstring gameProcessPattern = (L"FiveM_b\\d{4}_GameProcess\\.exe");
		std::wstring gtaProcess = (L"FiveM_GTAProcess.exe");
		std::wstring gameProcess = (L"FiveM_GameProcess.exe");


		auto [pid1, name1] = GetProcessPidByName(gtaProcessPattern);
		auto [pid2, name2] = GetProcessPidByName(gameProcessPattern);
		auto [pid3, name3] = GetProcessPidByName(gtaProcess);
		auto [pid4, name4] = GetProcessPidByName(gameProcess);

		if (pid1) {
			g_Variables.ProcIdFiveM = pid1;
			Mem.ProcName = name1;
		}
		else if (pid2) {
			g_Variables.ProcIdFiveM = pid2;
			Mem.ProcName = name2;
		}
		else if (pid3) {
			g_Variables.ProcIdFiveM = pid3;
			Mem.ProcName = name3;
		}
		else if (pid4) {
			g_Variables.ProcIdFiveM = pid4;
			Mem.ProcName = name4;
		}
		else {
			std::cerr << ("Failed to detect FiveM process.") << std::endl;
			return false;
		}

		if (!Mem.OpenProcByPid()) {
			return false;
		}
		std::cout << ("Process Infos:") << "\n";
		std::cout << ("Process ID: ") << g_Variables.ProcIdFiveM << "\n";
		std::wcout << (L"Process Name: ") << Mem.ProcName << "\n";
		std::cout << ("Version: ") << g_Offsets.CurrentBuild << "\n";
		std::cout << ("Module Base: ") << std::hex << Mem.ModBase << std::dec << "\n";
		std::cout << ("Module Size: ") << Mem.ModBaseSize << "\n\n";

		if (!Mem.ModBase || !Mem.ModBaseSize || !g_Variables.ProcIdFiveM) {
			return false;
		}

		// Default ped-related offsets (known builds overwrite below; unknown builds keep these)
		g_Offsets.m_WeaponManager = 0x10D8;
		g_Offsets.m_WeaponInfo = 0x20;
		g_Offsets.m_LastVehicle = 0xD30;
		g_Offsets.m_PlayerInfo = 0x10C8;
		g_Offsets.m_PlayerId = 0x88;
		g_Offsets.m_FragInst = 0x1400;
		g_Offsets.m_Armor = 0x14E0;
		g_Offsets.m_PedFlag = 0x1414;

		switch (g_Offsets.CurrentBuild) {
		case 3095:
			g_Offsets.m_World = Mem.ModBase + 0x25B14B0;
			g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1FBD4F0;
			g_Offsets.m_ViewPort = Mem.ModBase + 0x201DBA0;
			g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x201ED50;
			g_Offsets.m_BlipList = Mem.ModBase + 0x2023400;

			g_Offsets.m_MaxHealth = 0x284;

			g_Offsets.m_WeaponManager = 0x10B8;
			g_Offsets.m_EntityType = 0x1098;
			g_Offsets.m_LastVehicle = 0xD10;
			g_Offsets.m_PlayerInfo = 0x10A8;
			g_Offsets.m_FragInst = 0x1430;
			g_Offsets.m_PlayerId = 0xE8;
			g_Offsets.m_Armor = 0x150C;
			g_Offsets.m_PedFlag = 0x1444;

			g_Offsets.CurrentBuild = 3095;
			break;

		case 3258:
			g_Offsets.m_World = Mem.ModBase + 0x25b14b0;;
			g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1FBD4F0;
			g_Offsets.m_ViewPort = Mem.ModBase + 0x201dba0;;
			g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x201ED50;
			g_Offsets.m_BlipList = Mem.ModBase + 0x2023400;

			g_Offsets.m_MaxHealth = 0x284;

			g_Offsets.m_WeaponManager = 0x10B8;
			g_Offsets.m_EntityType = 0x1098;
			g_Offsets.m_LastVehicle = 0xD10;
			g_Offsets.m_PlayerInfo = 0x10A8;
			g_Offsets.m_FragInst = 0x1430;
			g_Offsets.m_PlayerId = 0xE8;
			g_Offsets.m_Armor = 0x150C;
			g_Offsets.m_PedFlag = 0x1444;

			g_Offsets.CurrentBuild = 3258;
			break;

		case 3323:
			g_Offsets.m_World = Mem.ModBase + 0x25C15B0;
			g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F85458;
			g_Offsets.m_ViewPort = Mem.ModBase + 0x202DC50;
			g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x202EB48;
			g_Offsets.m_BlipList = Mem.ModBase + 0x20333E0;

			g_Offsets.m_MaxHealth = 0x284;

			g_Offsets.m_WeaponManager = 0x10B8;
			g_Offsets.m_EntityType = 0x1098;
			g_Offsets.m_LastVehicle = 0xD10;
			g_Offsets.m_PlayerInfo = 0x10A8;
			g_Offsets.m_FragInst = 0x1430;
			g_Offsets.m_PlayerId = 0xE8;
			g_Offsets.m_Armor = 0x150C;
			g_Offsets.m_PedFlag = 0x1444;

			g_Offsets.CurrentBuild = 3323;
			break;

		case 3407:
			g_Offsets.m_World = Mem.ModBase + 0x25D7108;
			g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1F9A9D8;
			g_Offsets.m_ViewPort = Mem.ModBase + 0x20431C0;
			g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x20440C8;
			g_Offsets.m_BlipList = Mem.ModBase + 0x2047D50;

			g_Offsets.m_MaxHealth = 0x284;
			g_Offsets.m_WeaponManager = 0x10B8;
			g_Offsets.m_EntityType = 0x1098;
			g_Offsets.m_LastVehicle = 0xD10;
			g_Offsets.m_PlayerInfo = 0x10A8;
			g_Offsets.m_FragInst = 0x1430;
			g_Offsets.m_PlayerId = 0xE8;
			g_Offsets.m_Armor = 0x150C;
			g_Offsets.m_PedFlag = 0x1444;

			g_Offsets.CurrentBuild = 3407;
			break;

		case 3570:
			g_Offsets.m_World = Mem.ModBase + 0x25EC580;
			g_Offsets.m_ReplayInterFace = Mem.ModBase + 0x1FB0418;
			g_Offsets.m_ViewPort = Mem.ModBase + 0x2058BA0;
			g_Offsets.m_CamGameplayDirector = Mem.ModBase + 0x2059A48;
			g_Offsets.m_BlipList = Mem.ModBase + 0x2061870;

			g_Offsets.m_MaxHealth = 0x284;
			g_Offsets.m_WeaponManager = 0x10B8;
			g_Offsets.m_EntityType = 0x1098;
			g_Offsets.m_LastVehicle = 0xD10;
			g_Offsets.m_PlayerInfo = 0x10A8;
			g_Offsets.m_FragInst = 0x1430;
			g_Offsets.m_PlayerId = 0xE8;
			g_Offsets.m_Armor = 0x150C;
			g_Offsets.m_PedFlag = 0x1444;

			g_Offsets.CurrentBuild = 3570;
			break;
		}

		if (g_Offsets.CurrentBuild != 0)
		{
			//Functions Signatures
			{

				g_Offsets.m_GiveWeapon = Mem.FindSignature(
					{ 0x48, 0x89, 0x5c, 0x24, 0x00 , 0x48, 0x89, 0x6c, 0x24, 0x00 , 0x48, 0x89, 0x74, 0x24, 0x00 , 0x57, 0x48, 0x83, 0xec, 0x00 , 0x41, 0x8b, 0xf0, 0x8b, 0xfa, 0x48, 0x8b, 0xd9, 0xe8 }
				);


				g_Offsets.m_MagicBulletsPatch = Mem.FindSignature(
					{ 0x0F, 0x29, 0x4F, 0x00, 0x83, 0x8F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x4F }
				);


				g_Offsets.m_ArmsKinematics = Mem.FindSignature(
					{ 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC3, 0x60, 0x48, 0xFF, 0xCF, 0x75, 0xE6, 0x48, 0x8B, 0x5C, 0x24 }
				);


				g_Offsets.m_LegsKinematics = Mem.FindSignature(
					{ 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xC3, 0x60, 0x48, 0xFF, 0xCF, 0x75, 0xDF, 0x48, 0x8B, 0x5C, 0x24, 0x00, 0x48, 0x8B, 0x6C, 0x24, 0x00, 0x48, 0x8B, 0x74, 0x24 }
				);

				g_Offsets.m_SilentAim = Mem.FindSignature(
					{ 0x48, 0x8D, 0x45, 0x00, 0xF3, 0x0F, 0x10, 0x00, 0xF3, 0x0F, 0x10, 0x48, 0x00, 0xF3, 0x0F, 0x11, 0x45 }
				);

				g_Offsets.m_InfiniteCombatRoll = Mem.FindSignature(
					{ 0x89, 0x81, 0x00, 0x00, 0x00, 0x00, 0x8B, 0x87, 0x00, 0x00, 0x00, 0x00, 0xF7, 0xD0 }
				);


				g_Offsets.m_InfiniteAmmo0 = Mem.FindSignature(
					{ 0x41, 0x2B, 0xC9, 0x3B, 0xC8, 0x0F, 0x4D, 0xC8 }
				);

				g_Offsets.m_InfiniteAmmo1 = Mem.FindSignature(
					{ 0x41, 0x2B, 0xD1, 0xE8 }
				);

				g_Offsets.m_AimCPedPatternResult = Mem.FindSignature(
					{ 0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x48, 0x85, 0xC9, 0x74, 0x05, 0xE8, 0x00, 0x00, 0x00, 0x00, 0x8A, 0xCB }
				);


			}

			//Offsets Update
			{

				uintptr_t SpeedSigAddr = Mem.FindSignature({ 0x0f, 0x2f, 0xc2, 0x77, 0x00, 0xf3, 0x0f, 0x10, 0x83, 0x00, 0x00, 0x00, 0x00, 0x48, 0x83, 0xc4 });
				g_Offsets.m_Speed = Mem.Read<int>(SpeedSigAddr + 9);

				uintptr_t VehicleEngineSigAddr = Mem.FindSignature({ 0xf3, 0x0f, 0x11, 0x80, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x87, 0x00, 0x00, 0x00, 0x00, 0xf3, 0x0f, 0x10, 0x05 });
				g_Offsets.m_VehicleEngineHealth = Mem.Read<int>(VehicleEngineSigAddr + 4);


				uintptr_t SpreadSigAddr = Mem.FindSignature({ 0xf3, 0x41, 0x0f, 0x10, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0xca });
				g_Offsets.m_Spread = Mem.Read<int>(SpreadSigAddr + 5);


				uintptr_t RecoilSigAddr = Mem.FindSignature({ 0xf3, 0x0f, 0x10, 0xb0, 0x00, 0x00, 0x00, 0x00, 0x49, 0x8b, 0x87 });
				g_Offsets.m_Recoil = Mem.Read<int>(RecoilSigAddr + 4);


				uintptr_t ObjectSigAddr = Mem.FindSignature({ 0x4c, 0x8b, 0x41, 0x00, 0x4d, 0x85, 0xc9 });
				g_Offsets.m_CObject = (int)Mem.Read<BYTE>(ObjectSigAddr + 3);



				uintptr_t CWeaponSigAddr = Mem.FindSignature({ 0x48, 0x8b, 0x8b, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0xd7, 0xe8, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0x8b, 0x00, 0x00, 0x00, 0x00, 0x48, 0x8b, 0xd7, 0xe8 });
				g_Offsets.m_CWeapon = Mem.Read<int>(CWeaponSigAddr + 3);

				uintptr_t VehicleSigAddr = Mem.FindSignature({ 0x4c, 0x8b, 0x94, 0xc7, 0x00, 0x00, 0x00, 0x00, 0xeb, 0x00, 0x4d, 0x8b, 0xd6, 0x41, 0x8a, 0x82 });
				g_Offsets.m_VehicleDriver = Mem.Read<int>(VehicleSigAddr + 4);

				uintptr_t RagDollSigAddr = Mem.FindSignature({ 0x8b, 0x81, 0x00, 0x00, 0x00, 0x00, 0x83, 0xe0, 0x00, 0xc1, 0xe0, 0x00, 0x3d, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x8f });
				g_Offsets.m_NoRagDoll = Mem.Read<int>(RagDollSigAddr + 2);


				uintptr_t SeatBealtAddr = Mem.FindSignature({ 0xf6, 0x81,0x00 ,0x00 ,0x00 ,0x00 ,0x00 , 0x75,0x00 , 0x8b, 0x83 });
				g_Offsets.m_SeatBealt = Mem.Read<int>(SeatBealtAddr + 2);


				uintptr_t VehicleDoorsLockAddr = Mem.FindSignature({ 0x74, 0x00, 0x83, 0xb9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x75, 0x00, 0x45, 0x84, 0xf6 });
				g_Offsets.m_VehicleDoorsLockState = Mem.Read<int>(VehicleDoorsLockAddr + 4);


				uintptr_t HandlingSigAddr = Mem.FindSignature({ 0x48, 0x8b, 0x83,0x00,0x00,0x00,0x00, 0x8b, 0xd5, 0x48, 0x89, 0x44, 0x24,0x00, 0x4c, 0x89, 0x74, 0x24 });
				g_Offsets.m_Handling = Mem.Read<int>(HandlingSigAddr + 3);

			}
		}

		return true;
	}
}

