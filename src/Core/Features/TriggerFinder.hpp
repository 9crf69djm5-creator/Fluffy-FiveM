#pragma once
#include <Includes/Includes.hpp>

// ============================================================
//  Fluffy-FiveM | Trigger Finder
//  Scans FiveM process memory for Lua TriggerServerEvent
//  strings related to payments, weapons, and jobs.
//  This is an external read-only operation (no injection).
// ============================================================

namespace Core::Features {

    struct cTriggerFinder {

        // Keywords to auto-flag as "interesting"
        inline static const std::vector<std::string> s_Keywords = {
            "pay", "give", "money", "cash", "wage", "salary",
            "weapon", "gun", "arms", "ammo",
            "job", "work", "gang", "rank", "boss",
            "reward", "bonus", "item", "drug",
            "AddToBalance", "deposit", "bank",
            "GiveWeapon", "giveweapon",
            "TriggerServerEvent", "triggerserverevent"
        };

        // Results storage
        struct TriggerResult {
            uintptr_t Address;
            std::string Text;
            bool Flagged; // matched a keyword
        };

        inline static std::vector<TriggerResult> Results;
        inline static std::mutex ResultsMutex;
        inline static bool IsScanning  = false;
        inline static bool ScanDone    = false;
        inline static int  TotalFound  = 0;

        // ------------------------------------
        //  Scan: walk the game's full VA space
        //  looking for printable ASCII strings
        //  that look like Lua event names
        // ------------------------------------
        static void Scan() {
            if (IsScanning) return;
            IsScanning = true;
            ScanDone   = false;

            std::thread([]() {
                std::vector<TriggerResult> local;

                HANDLE hProcess = Mem.ProcHandle;
                if (!hProcess) {
                    IsScanning = false;
                    ScanDone   = true;
                    return;
                }

                SYSTEM_INFO si;
                GetSystemInfo(&si);

                uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;
                uintptr_t maxAddr = (uintptr_t)si.lpMaximumApplicationAddress;

                const size_t BufSize = 4096;
                std::vector<uint8_t> buffer(BufSize);

                while (addr < maxAddr) {
                    MEMORY_BASIC_INFORMATION mbi;
                    if (!VirtualQueryEx(hProcess, (LPCVOID)addr, &mbi, sizeof(mbi))) {
                        addr += 4096;
                        continue;
                    }

                    // Only scan committed, readable pages — skip executable/guard pages
                    bool readable = (mbi.State == MEM_COMMIT) &&
                                    (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY)) &&
                                    !(mbi.Protect & PAGE_GUARD);

                    if (readable && mbi.RegionSize > 0) {
                        size_t regionSize = mbi.RegionSize;
                        size_t offset = 0;

                        while (offset < regionSize) {
                            size_t toRead = min(BufSize, regionSize - offset);
                            SIZE_T bytesRead = 0;

                            if (!ReadProcessMemory(hProcess, (LPCVOID)(addr + offset), buffer.data(), toRead, &bytesRead) || bytesRead == 0) {
                                offset += BufSize;
                                continue;
                            }

                            // Extract printable ASCII strings of length >= 6
                            std::string current;
                            for (size_t i = 0; i < bytesRead; i++) {
                                uint8_t c = buffer[i];
                                if (c >= 0x20 && c <= 0x7E) {
                                    current += (char)c;
                                } else {
                                    if (current.size() >= 6) {
                                        // Check if it looks like a Lua event string
                                        // (has at least one colon, underscore, or is a known keyword)
                                        bool looksLikeEvent = (current.find(':') != std::string::npos ||
                                            current.find('_') != std::string::npos);

                                        if (looksLikeEvent || current.size() >= 10) {
                                            // Check against keywords
                                            std::string lower = current;
                                            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

                                            bool flagged = false;
                                            for (auto& kw : s_Keywords) {
                                                std::string lkw = kw;
                                                std::transform(lkw.begin(), lkw.end(), lkw.begin(), ::tolower);
                                                if (lower.find(lkw) != std::string::npos) {
                                                    flagged = true;
                                                    break;
                                                }
                                            }

                                            if (flagged || looksLikeEvent) {
                                                uintptr_t strAddr = addr + offset - current.size();
                                                local.push_back({ strAddr, current, flagged });
                                            }
                                        }
                                    }
                                    current.clear();
                                }
                            }
                            offset += bytesRead;
                        }
                    }

                    addr += mbi.RegionSize ? mbi.RegionSize : 4096;
                }

                {
                    std::lock_guard<std::mutex> lock(ResultsMutex);
                    Results     = std::move(local);
                    TotalFound  = (int)Results.size();
                }

                IsScanning = false;
                ScanDone   = true;

            }).detach();
        }

        // Clear previous scan results
        static void Clear() {
            std::lock_guard<std::mutex> lock(ResultsMutex);
            Results.clear();
            TotalFound = 0;
            ScanDone   = false;
        }
    };

    inline cTriggerFinder g_TriggerFinder;
}
