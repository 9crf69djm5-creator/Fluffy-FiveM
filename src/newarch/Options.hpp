#pragma once

#include <ImGui/Files/imgui.h>

namespace Cheat
{
    class Options
    {
    public:
        struct LegitBot
        {
            struct AimBot
            {
                bool Enabled;
                int KeyBind = 0;
                int KeyBindState = 0;
                bool TargetNPC;
                int HitBox = 0;
                int MaxDistance = 250;
                float FOV = 10.f;
                float SmoothHorizontal = 80.f;
                float SmoothVertical = 80.f;
            }AimBot;
        }LegitBot;
        struct Visuals
        {
            struct ESP
            {
                struct Players
                {
                    bool Enabled = true;
                    bool ShowLocalPlayer = true;
                    bool ShowNPCs;
                    int RenderDistance = 300;
                    bool Box;
                    bool Skeleton = true;
                    bool Name = true; ImVec2 NameRawPos; int NamePosNum;
                    bool HealthBar = true; ImVec2 HealthBarRawPos; int HealthBarPosNum;
                    bool ArmorBar; ImVec2 ArmorBarRawPos; int ArmorBarPosNum;
                    bool WeaponName; ImVec2 WeaponNameRawPos; int WeaponNamePosNum;
                    bool Distance; ImVec2 DistanceRawPos; int DistancePosNum;
                    bool SnapLines;
                }Players;

                struct Vehicles
                {
                    bool Enabled;
                    bool LocalVehicle;
                    bool SnapLines;
                    bool Box;
                    bool Name;
                }Vehicles;
            }ESP;
        }Visuals;
    };
}

inline Cheat::Options g_Options;
