#include "Aimbot.hpp"

#define SKEL_Neck_1 0x9995
#define SKEL_Head   0x796e
#define SKEL_Spine3 0x60f2

void Core::Features::cAimbot::SetViewAngles(CPed* Ped, D3DXVECTOR3 BonePos)
{
    uintptr_t CamFollowPedCamera = Mem.Read<uintptr_t>(Core::SDK::Pointers::pCamGamePlayDirector + 0x2C0);
    if (!CamFollowPedCamera) return;

    D3DXVECTOR3 CamPos = Mem.Read<D3DXVECTOR3>(CamFollowPedCamera + 0x60);
    D3DXVECTOR3 CurrentViewAngles = Mem.Read<D3DXVECTOR3>(CamFollowPedCamera + 0x40);
    D3DXVec3Normalize(&CurrentViewAngles, &CurrentViewAngles);
    
    uintptr_t CamFollowVehicle = Mem.Read<uintptr_t>(CamFollowPedCamera + 0x10);
    if (!CamFollowVehicle) return;

    if (Core::SDK::Pointers::pLocalPlayer->InVehicle())
    {
        if (Mem.Read<float>(CamFollowVehicle + 0x2AC) == -2.f)
        {
            Mem.Write<float>(CamFollowVehicle + 0x2AC, 0.f);
            Mem.Write<float>(CamFollowVehicle + 0x2C0, 111.f);
            Mem.Write<float>(CamFollowVehicle + 0x2C4, 111.f);
        }
    }

    D3DXVECTOR3 TargetViewAngles = BonePos - CamPos;
    D3DXVec3Normalize(&TargetViewAngles, &TargetViewAngles);

    D3DXVECTOR3 FinalAngles;

    if (g_Config.Aimbot->SmoothHorizontal > 1 || g_Config.Aimbot->SmoothVertical > 1)
    {
        D3DXVECTOR3 Delta = TargetViewAngles - CurrentViewAngles;
        FinalAngles = TargetViewAngles;

        if (g_Config.Aimbot->SmoothHorizontal > 1)
        {
            FinalAngles.x = CurrentViewAngles.x + (Delta.x / (float)g_Config.Aimbot->SmoothHorizontal);
            FinalAngles.y = CurrentViewAngles.y + (Delta.y / (float)g_Config.Aimbot->SmoothHorizontal);
        }

        if (g_Config.Aimbot->SmoothVertical > 1)
        {
            FinalAngles.z = CurrentViewAngles.z + (Delta.z / (float)g_Config.Aimbot->SmoothVertical);
        }

        D3DXVECTOR3 Cam3DAngles = Mem.Read<D3DXVECTOR3>(CamFollowPedCamera + 0x3D0);
        D3DXVECTOR3 ThirdPersonAngles = FinalAngles;
        ThirdPersonAngles.z = FinalAngles.z - (CurrentViewAngles.z - Cam3DAngles.z);

        Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + 0x40, FinalAngles);
        Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + 0x3D0, ThirdPersonAngles);
    }
    else
    {
        Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + 0x40, TargetViewAngles);
        Mem.Write<D3DXVECTOR3>(CamFollowPedCamera + 0x3D0, TargetViewAngles);
    }
}

void Core::Features::cAimbot::Start()
{
    while (true)
    {
        if (g_Config.Aimbot->Enabled &&
            g_Config.Aimbot->KeyBind &&
            GetAsyncKeyState(g_Config.Aimbot->KeyBind) & 0x8000 &&
            GetForegroundWindow() != g_Variables.g_hCheatWindow)
        {
            CPed* Ped = Core::SDK::Game::GetClosestPed(g_Config.Aimbot->MaxDistance, g_Config.Aimbot->IgnoreNPCs, g_Config.Aimbot->OnlyVisible);
            if (!Ped) continue;

           
            uintptr_t FragInstNMGta = Mem.Read<uintptr_t>((uintptr_t)Ped + g_Offsets.m_FragInst);
            uintptr_t v9 = Mem.Read<uintptr_t>(FragInstNMGta + 0x68);
            if (!v9) continue;

            Core::SDK::Game::cSkeleton_t Skeleton;
            Skeleton.m_pSkeleton = Mem.Read<uintptr_t>(v9 + 0x178);
            Skeleton.crSkeletonData.Ptr = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton);
            Skeleton.crSkeletonData.m_Used = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x1A);
            Skeleton.crSkeletonData.m_NumBones = Mem.Read<unsigned int>(Skeleton.crSkeletonData.Ptr + 0x5E);
            Skeleton.crSkeletonData.m_BoneIdTable_Slots = Mem.Read<unsigned short>(Skeleton.crSkeletonData.Ptr + 0x18);
            if (!Skeleton.crSkeletonData.m_BoneIdTable_Slots) continue;
            Skeleton.crSkeletonData.m_BoneIdTable = Mem.Read<uintptr_t>(Skeleton.crSkeletonData.Ptr + 0x10);
            Skeleton.Arg1 = Mem.Read<D3DXMATRIX>(Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x8));
            Skeleton.Arg2 = Mem.Read<uintptr_t>(Skeleton.m_pSkeleton + 0x18);

            
            uint16_t BoneId;
            switch (g_Config.Aimbot->HitBox)
            {
            case 0: BoneId = SKEL_Head; break;
            case 1: BoneId = SKEL_Neck_1; break;
            case 2: BoneId = SKEL_Spine3; break;
            default: BoneId = SKEL_Head; break;
            }

            D3DXVECTOR3 TargetPos = Core::SDK::Game::GetBonePosComplex(Ped, BoneId, Skeleton);
            D3DXVECTOR2 ScreenPos = Core::SDK::Game::WorldToScreen(TargetPos);

            if (Core::SDK::Game::IsOnScreen(ScreenPos))
            {
                int Fov = static_cast<int>(std::hypot(ScreenPos.x - g_Variables.g_vGameWindowCenter.x, ScreenPos.y - g_Variables.g_vGameWindowCenter.y));
                if (Fov < g_Config.Aimbot->FOV)
                {
                    D3DXVECTOR3 Offset = D3DXVECTOR3(0, 0, 0.08f);
                    SetViewAngles(Ped, TargetPos + Offset);
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::nanoseconds(1));
    }
}
