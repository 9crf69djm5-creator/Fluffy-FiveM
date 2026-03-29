#pragma once
#include <Includes/Includes.hpp>

// ============================================================
//  Fluffy-FiveM | Spectate
//  Continuously reads the target player's head position
//  and writes it into the camera's look-at position,
//  effectively locking the view onto that player.
// ============================================================

namespace Core::Features {

    struct cSpectate {

        inline static CPed*  TargetPed  = nullptr; // set from GUI / player-list
        inline static bool   Enabled    = false;
        inline static bool   Running    = false;

        // Write the camera position directly to the target's coords.
        // We move the camera *to* the player, looking in the same
        // direction — a "fly-on" spectate effect achievable externally.
        static void Update() {
            if (!Enabled || !TargetPed) return;

            uintptr_t CamFollowPedCamera = Mem.Read<uintptr_t>(Core::SDK::Pointers::pCamGamePlayDirector + 0x2C0);
            if (!CamFollowPedCamera) return;

            // Get target head position
            D3DXVECTOR3 HeadPos = TargetPed->GetBonePosDefault(0);
            if (HeadPos.x == 0.f && HeadPos.y == 0.f && HeadPos.z == 0.f) return;

            // Offset camera slightly behind/above the target so we can see them
            D3DXVECTOR3 CamOffset = D3DXVECTOR3(0.0f, -5.0f, 2.0f);

            // Read current cam position to compute direction
            D3DXVECTOR3 CurrentCamPos = Mem.Read<D3DXVECTOR3>(CamFollowPedCamera + 0x60);

            // Move the overlay camera to be near the target
            D3DXVECTOR3 NewCamPos = HeadPos + CamOffset;
            Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + 0x60, NewCamPos);

            // Point the camera at the target's head
            D3DXVECTOR3 Dir = HeadPos - NewCamPos;
            D3DXVec3Normalize(&Dir, &Dir);
            Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + 0x40, Dir);
            Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + 0x3D0, Dir);
        }

        static void Start() {
            Running = true;
            while (Running) {
                Update();
                std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60hz
            }
        }

        static void Stop() {
            Enabled    = false;
            TargetPed  = nullptr;
        }
    };

    inline cSpectate g_Spectate;
}
