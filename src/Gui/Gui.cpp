// Gui.cpp
#include "Gui.hpp"

#include <ImGui/Files/imgui.h>
#include <ImGui/Files/imgui_internal.h>

// Aqui voc� deve incluir seus headers personalizados, fonts, op��es etc
#include <newarch/Fonts.hpp>
#include <ImGui/FontAwesome/FontAwesome.hpp>
#include <newarch/Options.hpp>
#include <CustomWidgets/Custom.hpp>
#include <Core/Features/Exploits/Exploits.hpp>
#include <CustomWidgets/Preview.hpp>
#include <CustomWidgets/Notify.hpp>
#include <Core/SDK/SDK.hpp>
#include <ImGui/origens.h>
#include <Core/Features/Exploits/GiveWeapon.hpp>
#include <Core/Features/TriggerFinder.hpp>
#include <cstring>
#include <mutex>

const char* bones[] = { ("Head"), ("Neck"), ("Chest") };

// Vari�veis globais da GUI
int PauseLoop;
inline std::mutex DrawMtx;
int CurrentTab = 0;

static uintptr_t s_PlayerTargetPed = 0;
static int s_VehicleListSel = -1;
static uintptr_t s_TrollTargetPed = 0;
static uintptr_t s_TpTargetPed = 0;
bool IsSelected;
static int selectedItem = 0;

static bool GodAviso = false;
static bool NCAviso = false;
static bool EspAviso = false;
static bool EntryAviso = false;
auto CurrentVehicle = Core::SDK::Pointers::pLocalPlayer->GetLastVehicle();

// CHEATINGS BOOL
static bool GodMode = false;

#include <vector>

struct Locations_t {
    std::string Name;
    D3DXVECTOR3 Coords;
};

std::vector<Locations_t> Locations = {
    Locations_t(xorstr("Waypoint"), D3DXVECTOR3(0, 0, 0)),
    Locations_t(xorstr("Square"), D3DXVECTOR3(156.184, -1043.17, 29.3236)),
    Locations_t(xorstr("Pier"), D3DXVECTOR3(-1847.72, -1223.36, 13.8745)),
    Locations_t(xorstr("Paleto Bay"), D3DXVECTOR3(-397.605, 6047.57, 32.1797)),
    Locations_t(xorstr("Central Bank"), D3DXVECTOR3(221.781, 217.278, 106.705)),
    Locations_t(xorstr("Cassino"), D3DXVECTOR3(885.322, 16.8489, 80.65)),
    Locations_t(xorstr("Los Santos Airport"), D3DXVECTOR3(-975.532, -2880.89, 16.2665)),
    Locations_t(xorstr("Sandy Shores"), D3DXVECTOR3(1681.48, 3251.91, 40.809)),
};

namespace {

namespace MiscPedActions {

inline CPed* AsRemoteTarget( CPed* p ) {
    if ( !p || p == Core::SDK::Pointers::pLocalPlayer )
        return nullptr;
    return p;
}

inline void TeleportLocalTo( D3DXVECTOR3 pos ) {
    auto* lp = Core::SDK::Pointers::pLocalPlayer;
    if ( lp )
        lp->SetPos( pos );
}

inline void BringPedToMe( CPed* target ) {
    auto* lp = Core::SDK::Pointers::pLocalPlayer;
    if ( !AsRemoteTarget( target ) || !lp )
        return;
    D3DXVECTOR3 at = lp->GetPos( );
    at.z += 0.85f;
    target->SetPos( at );
}

inline void SetPedRagdoll( CPed* ped, bool ragdollOn ) {
    if ( !ped )
        return;
    Mem.Write<BYTE>( reinterpret_cast<uintptr_t>( ped ) + g_Offsets.m_NoRagDoll, ragdollOn ? BYTE( 0x80 ) : BYTE( 0x20 ) );
}

inline void PushPedVelocityUp( CPed* ped, float addZ ) {
    if ( !ped )
        return;
    D3DXVECTOR3 v = ped->GetVelocity( );
    v.z += addZ;
    Mem.Write<D3DXVECTOR3>( reinterpret_cast<uintptr_t>( ped ) + 0x320, v );
}

} // namespace MiscPedActions

inline void SnapshotEntityList( std::vector<Core::SDK::Game::EntityStruct>& out )
{
    std::lock_guard<std::mutex> lk( Core::Threads::g_EntityList.EntityListMutex );
    out = Core::SDK::Game::EntityList;
}

} // namespace

namespace Gui
{

    void ShutDown()
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    void Render()
    {
        if (!g_MenuInfo.IsOpen)
            return;


        static float f = 0.0f;
        static int counter = 0;
        static ImVec2 newWindowPos = ImVec2(-1, -1);

        ImVec2 windowSize = ImVec2(700, 500);
        ImVec2 windowPos = ImVec2((ImGui::GetIO().DisplaySize.x - windowSize.x) * 0.5f,
            (ImGui::GetIO().DisplaySize.y - windowSize.y) * 0.5f);

        if (newWindowPos.x < 0 || newWindowPos.y < 0)
            newWindowPos = windowPos;

        ImGui::SetNextWindowSize(windowSize);
        ImGui::SetNextWindowPos(newWindowPos, ImGuiCond_Always);

        // ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(8 / 255.f, 8 / 255.f, 8 / 255.f, 1.f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);

        static bool dragging = false;
        static ImVec2 dragClickOffset;

        enum SubTab
        {
            NONE,
            MISC_EXPLOITS,
            MISC_PLAYERLIST,
            MISC_VEHICLELIST,
            MISC_TELEPORT,
            MISC_TRIGGERFINDER,
            MISC_LUAEXECUTOR,
            MISC_TROLLING,
            AIM_AIMBOT,
            AIM_SILENT,
            AIM_TRIGGER,
            VISUALS_PLAYER,
            VISUALS_VEHICLES,
            VISUALS_CONFIG,
            OTHERS_SETTINGS
        };
        static SubTab selectedSubTab = NONE;
        static int currentMainTab = -1;

        static float indicatorAnimY = 0.0f;
        static float targetIndicatorY = 0.0f;
        static bool initialized = false;
        if (!initialized)
        {
            currentMainTab = 0;
            selectedSubTab = AIM_AIMBOT;

            indicatorAnimY = targetIndicatorY = ImGui::GetCursorScreenPos().y + 40.f;

            initialized = true;
        }

        if (ImGui::Begin("##MainWindow", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse))
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 winPos = ImGui::GetWindowPos();
            ImVec2 winSize = ImGui::GetWindowSize();
            ImVec2 mousePos = ImGui::GetIO().MousePos;

            float headerHeight = 30.0f;
            float sidebarWidth = 190.0f;

            ImGui::SetCursorScreenPos(winPos);
            ImGui::InvisibleButton(xorstr("window_drag_area"), ImVec2(winSize.x, headerHeight));

            if (ImGui::IsItemActive())
            {
                if (!dragging)
                {
                    dragging = true;
                    dragClickOffset = ImVec2(mousePos.x - winPos.x, mousePos.y - winPos.y);
                }
            }
            else dragging = false;

            if (dragging)
                newWindowPos = ImVec2(mousePos.x - dragClickOffset.x, mousePos.y - dragClickOffset.y);

            ImU32 headerColor = ImGui::GetColorU32(ImVec4(27 / 255.f, 28 / 255.f, 31 / 255.f, 1.0f));
            draw_list->AddRectFilled(winPos, ImVec2(winPos.x + winSize.x, winPos.y + headerHeight), headerColor, 12.0f, ImDrawFlags_RoundCornersTop);

            ImU32 sidebarColor = ImGui::GetColorU32(ImVec4(0.0667f, 0.0667f, 0.0784f, 1.0f));
            draw_list->AddRectFilled(ImVec2(winPos.x, winPos.y + headerHeight), ImVec2(winPos.x + sidebarWidth, winPos.y + winSize.y), sidebarColor);

            ImGui::PushFont(InterLight);
            draw_list->AddText(ImVec2(winPos.x + 15, winPos.y + (headerHeight - ImGui::CalcTextSize(xorstr("Fluffy-FiveM")).y) / 2.0f),
            ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), xorstr("Fluffy-FiveM"));
            ImGui::PopFont();

            ImU32 colIndicator = IM_COL32(184, 51, 142, 255);
            ImVec4 textHighlight = ImVec4(1, 1, 1, 1);
            ImVec4 textNormal = ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

            ImGui::SetCursorScreenPos(ImVec2(winPos.x + 10, winPos.y + headerHeight + 10));
            ImGui::BeginChild(xorstr("SidebarChild"), ImVec2(sidebarWidth - 20, winSize.y - headerHeight - 20), false, ImGuiWindowFlags_NoScrollbar);

            auto SidebarTab = [&](const char* name, int tabIndex, SubTab firstSubTab)
                {
                    bool selected = (currentMainTab == tabIndex);

                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

                    ImVec4 textColor = selected ? ImVec4(5.f, 1.f, 1.f, 1.f) : ImVec4(0.5f, 0.5f, 0.5f, 1.f);
                    ImGui::PushStyleColor(ImGuiCol_Text, textColor);

                    ImGui::PushFont(InterRegular);

                    if (ImGui::Selectable(name, selected, ImGuiSelectableFlags_SpanAllColumns, ImVec2(0, 25)))
                    {
                        currentMainTab = tabIndex;
                        selectedSubTab = firstSubTab;
                        targetIndicatorY = ImGui::GetItemRectMin().y - winPos.y;
                    }

                    ImGui::PopFont();
                    ImGui::PopStyleColor(4);

                    if (selected)
                        targetIndicatorY = ImGui::GetItemRectMin().y - winPos.y;
                };


            static float circlePosAnimX = -1.f;
            static float circlePosAnimY = -1.f;
            static float circlePosTargetX = -1.f;
            static float circlePosTargetY = -1.f;
            static SubTab lastSelectedSubTab = (SubTab)(-1);

            auto SubtabSelectable = [&](const char* name, SubTab subTabId)
                {
                    bool selected = (selectedSubTab == subTabId);

                    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
                    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));

                    ImVec2 cursorPos = ImGui::GetCursorScreenPos();

                    ImVec4 textColor = selected ? ImVec4(1.f, 1.f, 1.f, 1.f) : ImVec4(0.5f, 0.5f, 0.5f, 1.f);
                    ImGui::PushStyleColor(ImGuiCol_Text, textColor);

                    float offsetX = 10.0f;

                    float circleSpace = 14.0f;

                    ImGui::PushFont(InterRegular);
                    ImGui::SetCursorScreenPos(ImVec2(cursorPos.x + circleSpace + offsetX, cursorPos.y));
                    ImGui::TextUnformatted(name);
                    ImGui::PopFont();
                    ImGui::PopStyleColor();

                    ImGui::PopStyleColor(3);

                    std::string buttonId = xorstr("subtab_") + std::to_string(subTabId);
                    ImVec2 btnPos = ImGui::GetCursorScreenPos();
                    btnPos.y -= ImGui::GetTextLineHeightWithSpacing();
                    ImGui::SetCursorScreenPos(btnPos);
                    ImGui::InvisibleButton(buttonId.c_str(), ImVec2(100, ImGui::GetTextLineHeightWithSpacing()));

                    if (ImGui::IsItemClicked())
                        selectedSubTab = subTabId;

                    if (selected)
                    {
                        ImVec2 winPos = ImGui::GetWindowPos();
                        ImVec2 textSize = ImGui::CalcTextSize(name);
                        circlePosTargetX = (cursorPos.x + 6.0f) - winPos.x;
                        circlePosTargetY = (cursorPos.y + textSize.y * 0.5f) - winPos.y;

                        if (circlePosAnimX < 0.f)
                            circlePosAnimX = circlePosTargetX;
                        if (circlePosAnimY < 0.f)
                            circlePosAnimY = circlePosTargetY;
                    }
                };

            SidebarTab(xorstr("Aim Assistance"), 0, AIM_AIMBOT);
            if (currentMainTab == 0)
            {
                SubtabSelectable(xorstr("Aimbot"), AIM_AIMBOT);
                SubtabSelectable(xorstr("Silent"), AIM_SILENT);
                SubtabSelectable(xorstr("Trigger"), AIM_TRIGGER);
            }

            SidebarTab(xorstr("Visuals"), 1, VISUALS_PLAYER);
            if (currentMainTab == 1)
            {
                SubtabSelectable(xorstr("Player"), VISUALS_PLAYER);
                SubtabSelectable(xorstr("Vehicles"), VISUALS_VEHICLES);
                SubtabSelectable(xorstr("Configs"), VISUALS_CONFIG);
            }

            SidebarTab(xorstr("Misc"), 2, MISC_EXPLOITS);
            if (currentMainTab == 2)
            {
                SubtabSelectable(xorstr("Exploits"), MISC_EXPLOITS);
                SubtabSelectable(xorstr("Player List"), MISC_PLAYERLIST);
                SubtabSelectable(xorstr("Vehicle List"), MISC_VEHICLELIST);
                SubtabSelectable(xorstr("Teleport"), MISC_TELEPORT);
                SubtabSelectable(xorstr("Trigger Finder"), MISC_TRIGGERFINDER);
                SubtabSelectable(xorstr("Lua Executor"), MISC_LUAEXECUTOR);
                SubtabSelectable(xorstr("Trolling"), MISC_TROLLING);
            }

            SidebarTab(xorstr("Settings"), 3, OTHERS_SETTINGS);
            if (currentMainTab == 3)
            {
                SubtabSelectable(xorstr("Settings"), OTHERS_SETTINGS);
            }

            float animSpeed = 6.0f;
            circlePosAnimX += (circlePosTargetX - circlePosAnimX) * ImGui::GetIO().DeltaTime * animSpeed;
            circlePosAnimY += (circlePosTargetY - circlePosAnimY) * ImGui::GetIO().DeltaTime * animSpeed;

            if (circlePosAnimX >= 0.f && circlePosAnimY >= 0.f)
            {
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                ImVec2 winPos = ImGui::GetWindowPos();
                ImU32 circleColor = IM_COL32(184, 51, 142, 255);
                draw_list->AddCircleFilled(ImVec2(circlePosAnimX, circlePosAnimY) + winPos, 3.0f, circleColor);
            }

            ImVec2 childPos = ImGui::GetWindowPos();

            indicatorAnimY += (targetIndicatorY - indicatorAnimY) * 0.2f;

            draw_list->AddRectFilled(
                ImVec2(winPos.x, winPos.y + indicatorAnimY),
                ImVec2(winPos.x + 3, winPos.y + indicatorAnimY + 25),
                colIndicator,
                1.0f
            );

            ImGui::EndChild();

            ImGui::SetCursorPos(ImVec2(sidebarWidth + 10, headerHeight + 10));
            ImGui::BeginChild(xorstr("MainContent"), ImVec2(winSize.x - sidebarWidth - 20, winSize.y - headerHeight - 20), false);

            switch (selectedSubTab)
            {
            case NONE: ImGui::Text(xorstr("Selecione uma subtab...")); break;
            case AIM_AIMBOT:

                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Features"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 250));
                    {
                        ImGui::Checkbox(xorstr("Enabled"), &g_Config.Aimbot->Enabled);
                        if (g_Config.Aimbot->Enabled)
                        {

                            ImGui::Checkbox(xorstr("Draw FOV"), &g_Config.Aimbot->ShowFov);
                            ImGui::Checkbox(xorstr("Ignore NPCs"), &g_Config.Aimbot->IgnoreNPCs);
                            ImGui::Checkbox(xorstr("Only Visible"), &g_Config.Aimbot->OnlyVisible);
                            ImGui::Combo(xorstr("Hitbox"), &g_Config.Aimbot->HitBox, bones, IM_ARRAYSIZE(bones));

                        }


                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {
                        float fixedX = 188.f;

                        ImGui::CustomChild(xorstr("Settings"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {
                            if (g_Config.Aimbot->Enabled)
                            {
                                ImGui::SliderInt(xorstr("Field Of View"), &g_Config.Aimbot->FOV, 0, 1000, xorstr("%dpx"));
                                ImGui::SliderInt(xorstr("Max distance"), &g_Config.Aimbot->MaxDistance, 0, 1000, xorstr("%dm"));
                                ImGui::SliderInt(xorstr("Smooth X"), &g_Config.Aimbot->SmoothHorizontal, 0, 100, xorstr("%d%"));
                                ImGui::SliderInt(xorstr("Smooth Y"), &g_Config.Aimbot->SmoothVertical, 0, 100, xorstr("%d%"));
                                ImGui::KeyBind(xorstr("Key"), &g_Config.Aimbot->KeyBind, &g_Config.Aimbot->KeyBindState);
                                if (g_Config.Aimbot->ShowFov) {
                                    ImGui::PushFont(InterRegular);
                                    ImGui::AlignTextToFramePadding();
                                    ImGui::Text(xorstr("Fov Color"));
                                    ImGui::SameLine();
                                    ImGui::SetCursorPosX(fixedX);
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
                                    if (ImGui::ColorButton(xorstr("Fov Color"), g_Config.Aimbot->FovColor)) {
                                        ImGui::OpenPopup(xorstr("FovColorPicker"));
                                    }
                                    if (ImGui::BeginPopup(xorstr("FovColorPicker"))) {
                                        ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.Aimbot->FovColor, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                        ImGui::EndPopup();
                                    }
                                }
                            }
                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                break;
            case AIM_SILENT:
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Features"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 250));
                    {
                        ImGui::Checkbox(xorstr("Enabled"), &g_Config.SilentAim->Enabled);
                        if (g_Config.SilentAim->Enabled)
                        {

                            ImGui::Checkbox(xorstr("Magic Bullet"), &g_Config.SilentAim->MagicBullets);
                            ImGui::Checkbox(xorstr("Draw FOV"), &g_Config.SilentAim->ShowFov);
                            ImGui::Checkbox(xorstr("Ignore NPCs"), &g_Config.SilentAim->IgnoreNPCs);
                            ImGui::Checkbox(xorstr("Only Visible"), &g_Config.SilentAim->OnlyVisible);

                        }


                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {
                        float fixedX = 188.f;

                        ImGui::CustomChild(xorstr("Settings"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {
                            if (g_Config.SilentAim->Enabled)
                            {
                                ImGui::SliderInt(xorstr("Field Of View"), &g_Config.SilentAim->FOV, 0, 1000, xorstr("%dpx"));
                                ImGui::SliderInt(xorstr("Max distance"), &g_Config.SilentAim->MaxDistance, 0, 1000, xorstr("%dm"));
                                ImGui::KeyBind(xorstr("Key"), &g_Config.SilentAim->KeyBind, &g_Config.SilentAim->KeyBindState);
                                if (g_Config.SilentAim->ShowFov) {
                                    ImGui::PushFont(InterRegular);
                                    ImGui::AlignTextToFramePadding();
                                    ImGui::Text(xorstr("Fov Color"));
                                    ImGui::SameLine();
                                    ImGui::SetCursorPosX(fixedX);
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
                                    if (ImGui::ColorButton(xorstr("Fov Color"), g_Config.SilentAim->FovColor)) {
                                        ImGui::OpenPopup(xorstr("FovColorPicker"));
                                    }
                                    if (ImGui::BeginPopup(xorstr("FovColorPicker"))) {
                                        ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.SilentAim->FovColor, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                        ImGui::EndPopup();
                                    }
                                }
                            }
                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                break;
            case AIM_TRIGGER:
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {

                    float fixedX = 188.f;

                    ImGui::CustomChild(xorstr("Features"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 250));
                    {
                        ImGui::Checkbox(xorstr("Enabled"), &g_Config.TriggerBot->Enabled);
                        if (g_Config.TriggerBot->Enabled)
                        {
                            ImGui::Checkbox(xorstr("Smart Triggerbot"), &g_Config.TriggerBot->SmartTrigger);
                            if (!g_Config.TriggerBot->SmartTrigger) {
                                ImGui::Checkbox(xorstr("Draw FOV"), &g_Config.TriggerBot->ShowFov);
                            }
                            else
                            {
                                g_Config.TriggerBot->ShowFov = false;
                            }
                            ImGui::Checkbox(xorstr("Ignore NPCs"), &g_Config.TriggerBot->IgnoreNPCs);
                            ImGui::Checkbox(xorstr("Only Visible"), &g_Config.TriggerBot->OnlyVisible);

                        }


                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {
                        ImGui::CustomChild(xorstr("Settings"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {
                            if (g_Config.TriggerBot->Enabled)
                            {
                                if (!g_Config.TriggerBot->SmartTrigger) {
                                    ImGui::SliderInt(xorstr("Field of View"), &g_Config.TriggerBot->FOV, 0, 400, xorstr("%d"), 0);
                                    ImGui::SliderInt(xorstr("Max Distance"), &g_Config.TriggerBot->MaxDistance, 0, 1000, xorstr("%dm"), 0);
                                }

                                ImGui::SliderInt(xorstr("Delay"), &g_Config.TriggerBot->Delay, 0, 10, xorstr("%d"), 0);
                                ImGui::KeyBind(xorstr("Key"), &g_Config.TriggerBot->KeyBind, &g_Config.TriggerBot->KeyBindState);
                                if (g_Config.TriggerBot->ShowFov) {
                                    ImGui::PushFont(InterRegular);
                                    ImGui::AlignTextToFramePadding();
                                    ImGui::Text(xorstr("Fov Color"));
                                    ImGui::SameLine();
                                    ImGui::SetCursorPosX(fixedX);
                                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
                                    if (ImGui::ColorButton(xorstr("Fov Color"), g_Config.TriggerBot->FovColor)) {
                                        ImGui::OpenPopup(xorstr("FovColorPicker"));
                                    }
                                    if (ImGui::BeginPopup(xorstr("FovColorPicker"))) {
                                        ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.TriggerBot->FovColor, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                        ImGui::EndPopup();
                                    }
                                }
                            }
                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                break;
            case VISUALS_PLAYER:
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Features"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                    {
                        ImGui::Checkbox(xorstr("Enable"), &g_Config.ESP->Enabled);
                        if (ImGui::Checkbox(xorstr("Box"), &g_Config.ESP->Box)) {
                            if (!g_Config.ESP->Box) {
                                g_Config.ESP->FilledBox = false;
                            }
                        }
                        if (ImGui::Checkbox(xorstr("Filled Box"), &g_Config.ESP->FilledBox)) {
                            if (!g_Config.ESP->Box) {
                                g_Config.ESP->Box = true;
                            }

                        }
                        ImGui::Checkbox(xorstr("Skeleton"), &g_Config.ESP->Skeleton);
                        ImGui::Checkbox(xorstr("PlayerNames"), &g_Config.ESP->UserNames);
                        ImGui::Checkbox(xorstr("Distance"), &g_Config.ESP->DistanceFromMe);
                        ImGui::Checkbox(xorstr("Weapon"), &g_Config.ESP->WeaponName);
                        ImGui::Checkbox(xorstr("SnapLines"), &g_Config.ESP->SnapLines);
                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {
                        float fixedX = 188.f;

                        ImGui::CustomChild(xorstr("Settings"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {
                            ImGui::Checkbox(xorstr("Health Bar"), &g_Config.ESP->HealthBar);
                            ImGui::Checkbox(xorstr("Armor Bar"), &g_Config.ESP->ArmorBar);
                            ImGui::Checkbox(xorstr("Show LocalPlayer"), &g_Config.ESP->ShowLocalPlayer);
                            ImGui::Checkbox(xorstr("Ignore NPCs"), &g_Config.ESP->IgnoreNPCs);
                            ImGui::SliderInt(xorstr("Max Distance"), &g_Config.ESP->MaxDistance, 0, 1000, xorstr(" % dm"));

                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                break;
            case VISUALS_VEHICLES:
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Features"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                    {
                        ImGui::Checkbox(xorstr("Enabled"), &g_Config.VehicleESP->Enabled);
                        
                        if (ImGui::Checkbox(xorstr("Vehicle GodMode"), &g_Config.Player->VehicleGodMode)) {
                            if (!InVehicle) {
                                g_Config.Player->VehicleGodMode = false;
                            }
                            else {
                                CurrentVehicle->SetGodMode(g_Config.Player->VehicleGodMode);
                            }
                        }
                        ImGui::Checkbox(xorstr("Name"), &g_Config.VehicleESP->VehName);
                        ImGui::Checkbox(xorstr("SnapLines"), &g_Config.VehicleESP->SnapLines);
                        ImGui::Checkbox(xorstr("Distance"), &g_Config.VehicleESP->DistanceFromMe);
                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {

                        ImGui::CustomChild(xorstr("Settings"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {
                            ImGui::SliderInt(xorstr("Max Distance"), &g_Config.VehicleESP->MaxDistance, 0, 1000, xorstr(" % dm"));
                            float fixedX = 188.0f;
                            if (g_Config.VehicleESP->SnapLines) {
                                ImGui::PushFont(InterRegular);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text(xorstr("Lines Color"));
                                ImGui::SameLine();
                                ImGui::SetCursorPosX(fixedX);
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
                                if (ImGui::ColorButton(xorstr("Lines Color"), g_Config.VehicleESP->SnapLinesCol)) {
                                    ImGui::OpenPopup(xorstr("LinesColorPicker"));
                                }
                                if (ImGui::BeginPopup(xorstr("LinesColorPicker"))) {
                                    ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.VehicleESP->SnapLinesCol, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                    ImGui::EndPopup();
                                }
                            }
                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                break;
            case VISUALS_CONFIG:
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Preview"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                    {
                        Custom::g_EspPreview.DragDropHandler();
                        Custom::g_EspPreview.Draw();

                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {
                        ImGui::CustomChild(xorstr("Settings"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {
                            float fixedX = 188.f;
                            if (g_Config.ESP->Box) {
                                ImGui::PushFont(InterRegular);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text(xorstr("Box Color"));
                                ImGui::SameLine();
                                ImGui::SetCursorPosX(fixedX);
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);
                                if (ImGui::ColorButton(xorstr("Box Color"), g_Config.ESP->BoxCol)) {
                                    ImGui::OpenPopup(xorstr("BoxColorPicker"));
                                }
                                if (ImGui::BeginPopup(xorstr("BoxColorPicker"))) {
                                    ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.ESP->BoxCol, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                    ImGui::EndPopup();
                                }
                            }


                            if (g_Config.ESP->Skeleton) {
                                ImGui::PushFont(InterRegular);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text(xorstr("Skeleton Color"));
                                ImGui::SameLine();
                                ImGui::SetCursorPosX(fixedX);
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1);

                                if (ImGui::ColorButton(xorstr("##SkeletonColor"), g_Config.ESP->SkeletonCol)) {
                                    ImGui::OpenPopup(xorstr("SkeletonColorPicker"));
                                }

                                if (ImGui::BeginPopup(xorstr("SkeletonColorPicker"))) {
                                    ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.ESP->SkeletonCol,
                                        ImGuiColorEditFlags_NoSidePreview |
                                        ImGuiColorEditFlags_AlphaBar |
                                        ImGuiColorEditFlags_NoSmallPreview |
                                        ImGuiColorEditFlags_PickerHueWheel);
                                    ImGui::EndPopup();
                                }
                            }

                            if (g_Config.ESP->DistanceFromMe) {
                                ImGui::PushFont(InterRegular);
                                ImGui::Text(xorstr("Distance Color"));
                                ImGui::AlignTextToFramePadding();
                                ImGui::SameLine();
                                ImGui::SetCursorPosX(fixedX); // Espa�o de 5px � direita do texto
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1); // Espa�o de 5px � direita do texto
                                if (ImGui::ColorButton(xorstr("Distance Color"), g_Config.ESP->DistanceCol)) {
                                    ImGui::OpenPopup(xorstr("DistanceColorPicker"));
                                }
                                if (ImGui::BeginPopup(xorstr("DistanceColorPicker"))) {
                                    ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.ESP->DistanceCol, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                    ImGui::EndPopup();
                                }
                            }

                            if (g_Config.ESP->WeaponName) {
                                ImGui::PushFont(InterRegular);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text(xorstr("Weapon Color"));
                                ImGui::SameLine();
                                ImGui::SetCursorPosX(fixedX); // Espa�o de 5px � direita do texto
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1); // Espa�o de 5px � direita do texto
                                if (ImGui::ColorButton(xorstr("Weapon Color"), g_Config.ESP->WeaponNameCol)) {
                                    ImGui::OpenPopup(xorstr("WeaponColorPicker"));
                                }
                                if (ImGui::BeginPopup(xorstr("WeaponColorPicker"))) {
                                    ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.ESP->WeaponNameCol, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                    ImGui::EndPopup();
                                }

                            }

                            if (g_Config.ESP->SnapLines) {
                                ImGui::PushFont(InterRegular);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text(xorstr("Lines Color"));
                                ImGui::SameLine();
                                ImGui::SetCursorPosX(fixedX); // Espa�o de 5px � direita do texto
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1); // Espa�o de 5px � direita do texto
                                if (ImGui::ColorButton(xorstr("Lines Color"), g_Config.ESP->SnapLinesCol)) {
                                    ImGui::OpenPopup(xorstr("LinesColorPicker"));
                                }
                                if (ImGui::BeginPopup(xorstr("LinesColorPicker"))) {
                                    ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.ESP->SnapLinesCol, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                    ImGui::EndPopup();
                                }
                            }

                            if (g_Config.ESP->FilledBox) {
                                ImGui::PushFont(InterRegular);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text(xorstr("FilledBox Color"));
                                ImGui::SameLine();
                                ImGui::SetCursorPosX(fixedX); // Espa�o de 5px � direita do texto
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1); // Espa�o de 5px � direita do texto
                                if (ImGui::ColorButton(xorstr("Filled Box Color"), g_Config.ESP->FilledBoxCol)) {
                                    ImGui::OpenPopup(xorstr("FilledBoxColorPicker"));
                                }
                                if (ImGui::BeginPopup(xorstr("FilledBoxColorPicker"))) {
                                    ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.ESP->FilledBoxCol, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                    ImGui::EndPopup();
                                }

                            }

                            if (g_Config.ESP->UserNames) {
                                ImGui::PushFont(InterRegular);
                                ImGui::AlignTextToFramePadding();
                                ImGui::Text(xorstr("PlayerNames Color"));
                                ImGui::SameLine();
                                ImGui::SetCursorPosX(fixedX); // Espa�o de 5px � direita do texto
                                ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 1); // Espa�o de 5px � direita do texto
                                if (ImGui::ColorButton(xorstr("UserNames Color"), g_Config.ESP->UserNamesCol)) {
                                    ImGui::OpenPopup(xorstr("UserNamesColorPicker"));
                                }
                                if (ImGui::BeginPopup(xorstr("UserNamesColorPicker"))) {
                                    ImGui::ColorPicker4(xorstr("##picker"), (float*)&g_Config.ESP->UserNamesCol, ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSmallPreview | ImGuiColorEditFlags_PickerHueWheel);
                                    ImGui::EndPopup();
                                }

                            }

                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                break;
            case MISC_EXPLOITS:
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Exploits"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                    {

                        static bool AntiHSActive = false;

                        if (ImGui::Checkbox(xorstr("God Mode"), &g_Config.Player->EnableGodMode)) {
                            Core::SDK::Pointers::pLocalPlayer->SetGodMode(g_Config.Player->EnableGodMode);
                        }

                        if (ImGui::Checkbox(xorstr("NoClip"), &g_Config.Player->NoClipEnabled)) {
                            Core::SDK::Pointers::pLocalPlayer->FreezePed(g_Config.Player->NoClipEnabled);
                        }

                        ImGui::Checkbox(xorstr("Fast Run"), &g_Config.Player->FastRun);

                        if (ImGui::Checkbox(xorstr("Shrink"), &g_Config.Player->ShrinkEnabled)) {
                            Core::SDK::Pointers::pLocalPlayer->SetConfigFlag(ePedConfigFlag::Shrink, g_Config.Player->ShrinkEnabled);
                        }

                        if (ImGui::Checkbox(xorstr("No Recoil"), &g_Config.Player->NoRecoilEnabled)) {
                            if (g_Config.Player->NoRecoilEnabled) {
                                Core::SDK::Pointers::pLocalPlayer->GetWeaponManager()->SetRecoil(0.0f);
                            }
                            else {
                                Core::SDK::Pointers::pLocalPlayer->GetWeaponManager()->SetRecoil(g_Config.Player->RecoilValue);
                            }
                        }

                        if (ImGui::Checkbox(xorstr("No Reload"), &g_Config.Player->NoReloadEnabled)) {
                            if (g_Config.Player->NoReloadEnabled) {
                                Mem.PatchFunc(g_Offsets.m_InfiniteAmmo0, 3);
                            }
                            else {
                                Mem.WriteBytes(g_Offsets.m_InfiniteAmmo0, { 0x41, 0x2B, 0xC9, 0x3B, 0xC8, 0x0F, 0x4D, 0xC8 });
                            }
                        }

                        if (ImGui::Checkbox(xorstr("No Spread"), &g_Config.Player->NoSpreadEnabled)) {
                            if (g_Config.Player->NoSpreadEnabled) {
                                Core::SDK::Pointers::pLocalPlayer->GetWeaponManager()->SetSpread(0.0f);
                            }
                            else {
                                Core::SDK::Pointers::pLocalPlayer->GetWeaponManager()->SetSpread(g_Config.Player->SpreadValue);
                            }
                        }

                        if (ImGui::Checkbox(xorstr("Infinite CombatRoll"), &g_Config.Player->InfiniteCombatRoll)) {
                            std::thread InfiniteCombatRoll([]() { Core::SDK::Pointers::pLocalPlayer->SetInfCombatRoll(g_Config.Player->InfiniteCombatRoll); });
                            InfiniteCombatRoll.detach();
                        }

                        if (ImGui::Checkbox(xorstr("Infinite Stamina"), &g_Config.Player->InfiniteStamina)) {
                            Core::SDK::Pointers::pLocalPlayer->SetInfStamina(g_Config.Player->InfiniteStamina);
                        }

                        if (ImGui::Checkbox(xorstr("Steal Car"), &g_Config.Player->StealCarEnabled))
                        {
                            if (g_Config.Player->StealCarEnabled)
                            {
                                Core::SDK::Pointers::pLocalPlayer->SetConfigFlag(ePedConfigFlag::NotAllowedToJackAnyPlayers, false);
                                Core::SDK::Pointers::pLocalPlayer->SetConfigFlag(ePedConfigFlag::PlayerCanJackFriendlyPlayers, true);
                                Core::SDK::Pointers::pLocalPlayer->SetConfigFlag(ePedConfigFlag::WillJackAnyPlayer, true);
                            }
                            else
                            {
                                Core::SDK::Pointers::pLocalPlayer->SetConfigFlag(ePedConfigFlag::NotAllowedToJackAnyPlayers, true);
                                Core::SDK::Pointers::pLocalPlayer->SetConfigFlag(ePedConfigFlag::PlayerCanJackFriendlyPlayers, false);
                                Core::SDK::Pointers::pLocalPlayer->SetConfigFlag(ePedConfigFlag::WillJackAnyPlayer, false);
                            }
                        }


                    }
                    ImGui::EndCustomChild();
                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {
                        ImGui::CustomChild(xorstr("Settings"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {

                            static int KeyModeNC = 1;

                            ImGui::KeyBind(xorstr("NoClip Key"), &g_Config.Player->NoClipKey, &KeyModeNC);

                            g_Config.Player->CurrentHealthValue = Core::SDK::Pointers::pLocalPlayer->GetHealth() - 100.f >
                                Core::SDK::Pointers::pLocalPlayer->GetMaxHealth() - 100.f ?
                                Core::SDK::Pointers::pLocalPlayer->GetHealth() - 100.f :
                                Core::SDK::Pointers::pLocalPlayer->GetHealth() - 99.f;
                            g_Config.Player->CurrentArmorValue = Core::SDK::Pointers::pLocalPlayer->GetArmor();

                            if (ImGui::SliderFloat(xorstr("Health Value"), &g_Config.Player->CurrentHealthValue, -1.f,
                                Core::SDK::Pointers::pLocalPlayer->GetMaxHealth() - 100.f, xorstr("%1.f"))) {
                                Core::SDK::Pointers::pLocalPlayer->SetHealth(g_Config.Player->CurrentHealthValue + 500.0f);
                            }

                            if (ImGui::SliderFloat(xorstr("Armor Value"), &g_Config.Player->CurrentArmorValue, 0.f, 300.f, xorstr("%1.f"))) {
                                Core::SDK::Pointers::pLocalPlayer->SetArmor(g_Config.Player->CurrentArmorValue);
                            }

                            ImGui::SliderFloat(xorstr("NoClip Speed"), &g_Config.Player->NoClipSpeed, 0.1f, 100.f, xorstr("%1.1fm/s"));

                            if (ImGui::SliderFloat(xorstr("Run Speed"), &g_Config.Player->RunSpeed, 1.f, 10.f, xorstr("%1.1fm/s"))) {
                                Core::SDK::Pointers::pLocalPlayer->SetSpeed(g_Config.Player->FastRun ? g_Config.Player->RunSpeed : 1.f);
                            }

                            if (g_Config.Player->NoRecoilEnabled) {
                                if (ImGui::SliderFloat(xorstr("Recoil Value"), &g_Config.Player->RecoilValue, 0.0f, 10.f, xorstr("%1.2fx"))) {
                                    Core::SDK::Pointers::pLocalPlayer->GetWeaponManager()->SetRecoil(g_Config.Player->RecoilValue);
                                }
                            }

                            if (g_Config.Player->NoSpreadEnabled) {
                                if (ImGui::SliderFloat(xorstr("Spread Value"), &g_Config.Player->SpreadValue, 0.0f, 10.f, xorstr("%1.2fx"))) {
                                    Core::SDK::Pointers::pLocalPlayer->GetWeaponManager()->SetSpread(g_Config.Player->SpreadValue);
                                }
                            }

                            ImGui::PushFont(InterRegular);
                            if (ImGui::Button(xorstr("Fix Vehicle"), ImVec2(-1, 30)))
                            {
                                Core::SDK::Pointers::pLocalPlayer->GetLastVehicle()->Fix();
                            }

                        }
                        ImGui::EndCustomChild();
                    }

                }
                ImGui::EndGroup();
                break;
            case MISC_PLAYERLIST:
                {
                std::vector<Core::SDK::Game::EntityStruct> entSnap;
                SnapshotEntityList( entSnap );

                Core::SDK::Game::EntityStruct plSelected{};
                bool plHasSel = false;
                if ( s_PlayerTargetPed ) {
                    for ( const auto& e : entSnap ) {
                        if ( e.Ped && reinterpret_cast<uintptr_t>( e.Ped ) == s_PlayerTargetPed ) {
                            plSelected = e;
                            plHasSel = true;
                            break;
                        }
                    }
                    if ( !plHasSel )
                        s_PlayerTargetPed = 0;
                }

                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Players List"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                    {
                        ImGui::TextColored(ImColor(140, 150, 170), xorstr("Session players (PlayerInfo + slot)"));
                        ImGui::BeginChild(xorstr("##pl_scroll"), ImVec2(-1, 0), ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));
                        for ( const auto& ent : entSnap )
                        {
                            if ( !Core::SDK::Game::IsRemoteSessionPlayer( ent.Ped ) )
                                continue;

                            std::string label = Core::SDK::Game::GetPlayerListLabel( ent );
                            uintptr_t pkey = reinterpret_cast<uintptr_t>( ent.Ped );
                            ImGui::PushID( reinterpret_cast<void*>( ent.Ped ) );
                            IsSelected = ( s_PlayerTargetPed == pkey );
                            if ( ImGui::Selectable( label.c_str( ), &IsSelected ) )
                                s_PlayerTargetPed = pkey;
                            ImGui::PopID( );
                        }
                        ImGui::PopStyleVar();
                        ImGui::EndChild();

                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {

                        ImGui::CustomChild(xorstr("Info"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 88));
                        {
                            if ( !plHasSel ) {
                                ImGui::TextColored(ImColor(200, 200, 200), xorstr("Select Player"));
                            }
                            else
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 4));

                                ImGui::TextColored(ImColor(g_Col.FeaturesText), xorstr("Name:")); ImGui::SameLine();
                                ImGui::TextColored(ImColor(g_Col.SecundaryText), Core::SDK::Game::GetPlayerListLabel(plSelected).c_str());
                                ImGui::TextColored(ImColor(g_Col.FeaturesText), xorstr("Server Id:")); ImGui::SameLine();
                                ImGui::TextColored(ImColor(g_Col.SecundaryText), std::to_string(plSelected.Id).c_str());
                                ImGui::TextColored(ImColor(g_Col.FeaturesText), xorstr("Distance:")); ImGui::SameLine();
                                ImGui::TextColored(ImColor(g_Col.SecundaryText), std::to_string(plSelected.Distance).c_str());

                                ImGui::PopStyleVar();
                            }

                        }
                        ImGui::EndCustomChild();

                        ImGui::CustomChild(xorstr("Actions"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 242));
                        {
                            if ( !plHasSel ) {
                                ImGui::TextColored(ImColor(200, 200, 200), xorstr("Select Player"));
                            }
                            else
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 6));
                                bool Friend = plSelected.IsFriend;
                                if (ImGui::Checkbox(xorstr("Friend"), &Friend))
                                {
                                    Core::SDK::Game::FriendMap[plSelected.Ped] = Friend;
                                }

                                ImGui::PushFont(InterRegular);
                                if (ImGui::Button(xorstr("Go to player"), ImVec2(-1, 26)) && plSelected.Ped)
                                    MiscPedActions::TeleportLocalTo( plSelected.Pos );
                                if (ImGui::Button(xorstr("Bring to me"), ImVec2(-1, 26)) && plSelected.Ped)
                                    MiscPedActions::BringPedToMe( plSelected.Ped );
                                {
                                    float pairW = ( ImGui::GetContentRegionAvail( ).x - 6.f ) * 0.5f;
                                    if ( ImGui::Button( xorstr( "Freeze" ), ImVec2( pairW, 26 ) ) && plSelected.Ped )
                                        plSelected.Ped->FreezePed( true );
                                    ImGui::SameLine( );
                                    if ( ImGui::Button( xorstr( "Unfreeze" ), ImVec2( pairW, 26 ) ) && plSelected.Ped )
                                        plSelected.Ped->FreezePed( false );
                                }
                                {
                                    float pairW = ( ImGui::GetContentRegionAvail( ).x - 6.f ) * 0.5f;
                                    if ( ImGui::Button( xorstr( "Ragdoll" ), ImVec2( pairW, 26 ) ) && plSelected.Ped )
                                        MiscPedActions::SetPedRagdoll( plSelected.Ped, true );
                                    ImGui::SameLine( );
                                    if ( ImGui::Button( xorstr( "Stop ragdoll" ), ImVec2( pairW, 26 ) ) && plSelected.Ped )
                                        MiscPedActions::SetPedRagdoll( plSelected.Ped, false );
                                }
                                if (ImGui::Button(xorstr("Launch up"), ImVec2(-1, 26)) && plSelected.Ped)
                                    MiscPedActions::PushPedVelocityUp( plSelected.Ped, 24.f );

                                if (ImGui::Button(xorstr("Copy Clothes"), ImVec2(-1, 26)) && plSelected.Ped)
                                {
                                    uintptr_t pedAddress = reinterpret_cast<uintptr_t>(plSelected.Ped);
                                    uint64_t drawhandler = Mem.Read<uint64_t>(pedAddress + 0x48);

                                    uint64_t shoes = Mem.Read<uint64_t>(drawhandler + 0xF0);
                                    int shoes_tex = Mem.Read<int>(drawhandler + 0xF0 + 0x8);

                                    uint64_t leg = Mem.Read<uint64_t>(drawhandler + 0xF8);
                                    int leg_tex = Mem.Read<int>(drawhandler + 0xF8 + 0x8);

                                    uint64_t body = Mem.Read<uint64_t>(drawhandler + 0x100);
                                    int body_tex = Mem.Read<int>(drawhandler + 0x100 + 0x8);

                                    uint64_t tshirt = Mem.Read<uint64_t>(drawhandler + 0x108);
                                    int tshirt_tex = Mem.Read<int>(drawhandler + 0x108 + 0x8);

                                    uint64_t mask = Mem.Read<uint64_t>(drawhandler + 0x110);
                                    int mask_tex = Mem.Read<int>(drawhandler + 0x110 + 0x8);

                                    uintptr_t localPlayerAddr = reinterpret_cast<uintptr_t>(Core::SDK::Pointers::pLocalPlayer);
                                    uint64_t local_drawhandler = Mem.Read<uint64_t>(localPlayerAddr + 0x48);

                                    Mem.Write<uint64_t>(local_drawhandler + 0xF0, shoes);
                                    Mem.Write<int>(local_drawhandler + 0xF0 + 0x8, shoes_tex);

                                    Mem.Write<uint64_t>(local_drawhandler + 0xF8, leg);
                                    Mem.Write<int>(local_drawhandler + 0xF8 + 0x8, leg_tex);

                                    Mem.Write<uint64_t>(local_drawhandler + 0x100, body);
                                    Mem.Write<int>(local_drawhandler + 0x100 + 0x8, body_tex);

                                    Mem.Write<uint64_t>(local_drawhandler + 0x108, tshirt);
                                    Mem.Write<int>(local_drawhandler + 0x108 + 0x8, tshirt_tex);

                                    Mem.Write<uint64_t>(local_drawhandler + 0x110, mask);
                                    Mem.Write<int>(local_drawhandler + 0x110 + 0x8, mask_tex);

                                    Mem.Write<uint64_t>(localPlayerAddr + 0xAC, 1);
                                }

                                ImGui::PopFont();
                                ImGui::PopStyleVar();
                            }

                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                }
                break;
            case MISC_VEHICLELIST:
                
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Vehicles List"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                    {

                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));
                        {
                            std::lock_guard<std::mutex> lock(Core::Threads::g_VehicleList.vehicleListMutex); // Protege a lista durante a renderiza��o

                            for (int i = 0; i < Core::SDK::Game::VehicleList.size(); i++)
                            {
                                IsSelected = s_VehicleListSel == i;
                                std::string Name = Core::SDK::Game::VehicleList[i].Name + xorstr(" (") + std::to_string((int)Core::SDK::Game::VehicleList[i].Dist) + xorstr("m)");
                                if (ImGui::Selectable(Name.c_str(), &IsSelected)) s_VehicleListSel = i;
                            }
                        }

                        ImGui::PopStyleVar();

                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {

                        ImGui::CustomChild(xorstr("Info"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 150));
                        {
                            if (s_VehicleListSel == -1) {
                                ImGui::TextColored(ImColor(g_Col.SecundaryText), xorstr("Select Vehicle"));
                            }
                            else
                            {
                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 4));

                                ImGui::TextColored(ImColor(g_Col.FeaturesText), xorstr("Name:")); ImGui::SameLine();
                                ImGui::TextColored(ImColor(g_Col.SecundaryText), Core::SDK::Game::VehicleList[s_VehicleListSel].Name.c_str());
                                ImGui::TextColored(ImColor(g_Col.FeaturesText), xorstr("Distance:")); ImGui::SameLine();
                                ImGui::TextColored(ImColor(g_Col.SecundaryText), (std::to_string((int)Core::SDK::Game::VehicleList[s_VehicleListSel].Dist) + xorstr("m")).c_str());
                                ImGui::TextColored(ImColor(g_Col.FeaturesText), xorstr("Driver:")); ImGui::SameLine();
                                ImGui::TextColored(ImColor(g_Col.SecundaryText), Core::SDK::Game::VehicleList[s_VehicleListSel].Driver == 0 ? xorstr("Unknown") : Core::SDK::Game::VehicleList[s_VehicleListSel].Driver->GetPedType() != 2 ? xorstr("NPC") : Core::SDK::Game::GetPedName(Core::SDK::Game::VehicleList[s_VehicleListSel].Driver).c_str());
                                ImGui::TextColored(ImColor(g_Col.FeaturesText), xorstr("Doors Locked:")); ImGui::SameLine();
                                ImGui::TextColored(ImColor(g_Col.SecundaryText), Core::SDK::Game::VehicleList[s_VehicleListSel].IsLocked ? xorstr("Yes") : xorstr("No"));

                                ImGui::PopStyleVar();
                            }

                        }
                        ImGui::EndCustomChild();

                        ImGui::CustomChild(xorstr("Actions"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 180));
                        {
                            if (s_VehicleListSel == -1) {
                                ImGui::TextColored(ImColor(g_Col.SecundaryText), xorstr("Select Vehicle"));
                            }
                            else
                            {

                                auto Veh = Core::SDK::Game::VehicleList[s_VehicleListSel].Pointer;

                                if (Core::SDK::Game::VehicleList[s_VehicleListSel].Pointer->IsLocked())
                                {
                                    ImGui::PushFont(InterRegular);
                                    if (ImGui::Button(xorstr("Unlock"), ImVec2(-1, 30))) {
                                        Veh->DoorState(true);
                                    }
                                }
                                else {
                                    ImGui::PushFont(InterRegular);
                                    if (ImGui::Button(xorstr("Lock"), ImVec2(-1, 30))) {
                                        Veh->DoorState(false);
                                    }
                                }
                                ImGui::PushFont(InterRegular);
                                if (ImGui::Button(xorstr("Teleport"), ImVec2(-1, 30))) {
                                    Core::SDK::Pointers::pLocalPlayer->SetPos(Veh->GetPos());
                                }

                                ImGui::PopStyleVar();
                            }

                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                break;
            case MISC_TELEPORT:
                {
                std::vector<Core::SDK::Game::EntityStruct> tpSnap;
                SnapshotEntityList( tpSnap );
                Core::SDK::Game::EntityStruct tpSel{};
                bool tpHasSel = false;
                if ( s_TpTargetPed ) {
                    for ( const auto& e : tpSnap ) {
                        if ( e.Ped && reinterpret_cast<uintptr_t>( e.Ped ) == s_TpTargetPed ) {
                            tpSel = e;
                            tpHasSel = true;
                            break;
                        }
                    }
                    if ( !tpHasSel )
                        s_TpTargetPed = 0;
                }

                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Places & players"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                    {
                        ImGui::TextColored(ImColor(180, 180, 190), xorstr("Saved places"));
                        ImGui::BeginChild(xorstr("##tp_loc"), ImVec2(-1, 118), ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 6));
                        for (int i = 0; i < Locations.size(); i++)
                        {
                            bool isSelected = selectedItem == i;
                            if (ImGui::Selectable(Locations[i].Name.c_str(), isSelected))
                                selectedItem = i;
                        }
                        ImGui::PopStyleVar();
                        ImGui::EndChild();

                        ImGui::Spacing();
                        ImGui::TextColored(ImColor(180, 180, 190), xorstr("Session players"));
                        ImGui::BeginChild(xorstr("##tp_players"), ImVec2(-1, 148), ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 6));
                        for ( const auto& ent : tpSnap )
                        {
                            if ( !Core::SDK::Game::IsRemoteSessionPlayer( ent.Ped ) )
                                continue;
                            std::string plabel = Core::SDK::Game::GetPlayerListLabel( ent );
                            uintptr_t pkey = reinterpret_cast<uintptr_t>( ent.Ped );
                            ImGui::PushID( reinterpret_cast<void*>( ent.Ped ) );
                            bool sel = ( s_TpTargetPed == pkey );
                            if ( ImGui::Selectable( plabel.c_str( ), sel ) )
                                s_TpTargetPed = pkey;
                            ImGui::PopID();
                        }
                        ImGui::PopStyleVar();
                        ImGui::EndChild();
                    }
                    ImGui::EndCustomChild();


                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {

                        ImGui::CustomChild(xorstr("Actions"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {
                            std::string teleportButtonText = std::string(xorstr("Teleport to ")) + Locations[selectedItem].Name;
                            ImGui::PushFont(InterRegular);
                            if (ImGui::Button(teleportButtonText.c_str(), ImVec2(-1, 28))) {
                                if (selectedItem == 0) {
                                    Core::Features::Exploits::TpToWaypoint();
                                }
                                else {
                                    Core::SDK::Pointers::pLocalPlayer->SetPos(Locations[selectedItem].Coords);
                                }
                            }
                            ImGui::Spacing();
                            ImGui::Separator();
                            ImGui::TextColored(ImColor(150, 150, 160), xorstr("Selected player from list"));
                            if ( !tpHasSel ) {
                                ImGui::TextColored(ImColor(120, 120, 130), xorstr("Pick a player on the left."));
                            } else if ( !tpSel.Ped ) {
                                ImGui::TextColored(ImColor(120, 120, 130), xorstr("Invalid target."));
                            } else {
                                    ImGui::TextColored(ImColor(200, 200, 210), Core::SDK::Game::GetPlayerListLabel(tpSel).c_str());
                                    if (ImGui::Button(xorstr("Go to player"), ImVec2(-1, 26)))
                                        MiscPedActions::TeleportLocalTo(tpSel.Pos);
                                    if (ImGui::Button(xorstr("Bring to me"), ImVec2(-1, 26)))
                                        MiscPedActions::BringPedToMe(tpSel.Ped);
                                    {
                                        float pairW = (ImGui::GetContentRegionAvail().x - 6.f) * 0.5f;
                                        if (ImGui::Button(xorstr("Freeze"), ImVec2(pairW, 26)))
                                            tpSel.Ped->FreezePed(true);
                                        ImGui::SameLine();
                                        if (ImGui::Button(xorstr("Unfreeze"), ImVec2(pairW, 26)))
                                            tpSel.Ped->FreezePed(false);
                                    }
                                    {
                                        float pairW = (ImGui::GetContentRegionAvail().x - 6.f) * 0.5f;
                                        if (ImGui::Button(xorstr("Ragdoll"), ImVec2(pairW, 26)))
                                            MiscPedActions::SetPedRagdoll(tpSel.Ped, true);
                                        ImGui::SameLine();
                                        if (ImGui::Button(xorstr("Stop ragdoll"), ImVec2(pairW, 26)))
                                            MiscPedActions::SetPedRagdoll(tpSel.Ped, false);
                                    }
                                    if (ImGui::Button(xorstr("Launch up"), ImVec2(-1, 26)))
                                        MiscPedActions::PushPedVelocityUp(tpSel.Ped, 24.f);
                            }
                            ImGui::PopFont();
                        }
                        ImGui::EndCustomChild();


                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                }
                break;
            case MISC_TRIGGERFINDER:
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Trigger Finder"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                    {
                        ImGui::TextColored(ImColor(180, 220, 255), xorstr("Lua / server event strings (memory scan)"));
                        ImGui::TextColored(ImColor(150, 150, 150), xorstr("Read-only scan of FiveM GTA process RAM."));
                        ImGui::Spacing();
                        ImGui::TextColored(ImColor(120, 140, 160), xorstr("Now: keyword + pattern scan"));
                        ImGui::TextColored(ImColor(120, 140, 160), xorstr("Planned: filter by resource, export list,"));
                        ImGui::TextColored(ImColor(120, 140, 160), xorstr("        incremental / faster scans"));

                        ImGui::PushFont(InterRegular);
                        ImGui::Spacing();

                        if (Core::Features::cTriggerFinder::IsScanning) {
                            ImGui::TextColored(ImColor(255, 200, 50), xorstr("[Scanning...] Please wait..."));
                        } else {
                            if (ImGui::Button(xorstr("Start Scan"), ImVec2(-1, 30))) {
                                Core::Features::cTriggerFinder::Clear();
                                Core::Features::cTriggerFinder::Scan();
                            }
                        }

                        if (!Core::Features::cTriggerFinder::IsScanning) {
                            if (ImGui::Button(xorstr("Clear Results"), ImVec2(-1, 28))) {
                                Core::Features::cTriggerFinder::Clear();
                            }
                        }

                        ImGui::Spacing();
                        if (Core::Features::cTriggerFinder::ScanDone) {
                            ImGui::TextColored(ImColor(100, 255, 120), xorstr("Scan complete!"));
                            std::string countStr = std::string("Found: ") + std::to_string(Core::Features::cTriggerFinder::TotalFound) + " strings";
                            ImGui::TextColored(ImColor(200, 200, 200), countStr.c_str());
                        }
                        ImGui::PopFont();
                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {
                        ImGui::CustomChild(xorstr("Results"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {
                            std::lock_guard<std::mutex> lock(Core::Features::cTriggerFinder::ResultsMutex);

                            if (Core::Features::cTriggerFinder::Results.empty()) {
                                ImGui::TextColored(ImColor(150, 150, 150), xorstr("No results yet. Press 'Start Scan'."));
                            } else {
                                ImGui::BeginChild(xorstr("##tf_scroll"), ImVec2(0, 0), ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 3));
                                for (auto& r : Core::Features::cTriggerFinder::Results) {
                                    if (r.Flagged) {
                                        ImGui::TextColored(ImColor(255, 170, 50), r.Text.c_str());
                                    } else {
                                        ImGui::TextColored(ImColor(200, 200, 200), r.Text.c_str());
                                    }
                                }
                                ImGui::PopStyleVar();
                                ImGui::EndChild();
                            }
                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                break;
            case MISC_LUAEXECUTOR:
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Lua / server scripting"), ImVec2(ImGui::GetWindowSize().x - 40, 330));
                    {
                        ImGui::TextColored(ImColor(100, 200, 255), xorstr("Workspace for events & snippets"));
                        ImGui::TextColored(ImColor(150, 150, 150), xorstr("This build is external: it cannot run Lua inside the game."));
                        ImGui::TextColored(ImColor(120, 140, 160), xorstr("Now: edit text, load strings from Trigger Finder, copy to clipboard."));
                        ImGui::TextColored(ImColor(120, 140, 160), xorstr("Planned: internal executor hook, server event testing,"));
                        ImGui::TextColored(ImColor(120, 140, 160), xorstr("        resource dump helpers"));
                        ImGui::Separator();

                        static char luaBuf[4096] = "";
                        ImGui::InputTextMultiline(xorstr("##Source"), luaBuf, IM_ARRAYSIZE(luaBuf), ImVec2(-1, 150), ImGuiInputTextFlags_AllowTabInput);

                        ImGui::PushFont(InterRegular);
                        if (ImGui::Button(xorstr("Copy to clipboard"), ImVec2(160, 28))) {
                            Utils::PasteClipboard(luaBuf);
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(xorstr("Clear"), ImVec2(100, 28))) {
                            luaBuf[0] = '\0';
                        }
                        ImGui::SameLine();
                        if (ImGui::Button(xorstr("Load from Trigger Finder"), ImVec2(200, 28))) {
                            std::lock_guard<std::mutex> lock(Core::Features::cTriggerFinder::ResultsMutex);
                            std::string allTriggers;
                            for (auto& r : Core::Features::cTriggerFinder::Results) {
                                allTriggers += r.Text + "\n";
                            }
                            strncpy_s(luaBuf, allTriggers.c_str(), sizeof(luaBuf) - 1);
                            luaBuf[sizeof(luaBuf) - 1] = '\0';
                        }
                        ImGui::PopFont();

                        ImGui::Spacing();
                        ImGui::TextColored(ImColor(150, 150, 150), xorstr("Use Trigger Finder to discover event names, paste into your own internal"));
                        ImGui::TextColored(ImColor(150, 150, 150), xorstr("tool or a trusted executor — never paste random server code blindly."));
                    }
                    ImGui::EndCustomChild();
                }
                ImGui::EndGroup();
                break;
            case MISC_TROLLING:
                {
                std::vector<Core::SDK::Game::EntityStruct> trollSnap;
                SnapshotEntityList( trollSnap );
                Core::SDK::Game::EntityStruct trollSel{};
                bool trollHasSel = false;
                if ( s_TrollTargetPed ) {
                    for ( const auto& e : trollSnap ) {
                        if ( e.Ped && reinterpret_cast<uintptr_t>( e.Ped ) == s_TrollTargetPed ) {
                            trollSel = e;
                            trollHasSel = true;
                            break;
                        }
                    }
                    if ( !trollHasSel )
                        s_TrollTargetPed = 0;
                }

                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {
                    ImGui::CustomChild(xorstr("Active Players"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                    {
                        ImGui::TextColored(ImColor(120, 140, 160), xorstr("Select a remote player (sync willing)."));
                        ImGui::TextColored(ImColor(120, 140, 160), xorstr("Planned: vehicle tools, anim spam, more."));
                        ImGui::Spacing();
                        ImGui::BeginChild(xorstr("##troll_scroll"), ImVec2(-1, 0), ImGuiChildFlags_Border, ImGuiWindowFlags_AlwaysVerticalScrollbar);
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(14, 8));
                        for ( const auto& ent : trollSnap ) {
                            if ( !Core::SDK::Game::IsRemoteSessionPlayer( ent.Ped ) )
                                continue;
                            std::string label = Core::SDK::Game::GetPlayerListLabel( ent );
                            uintptr_t pkey = reinterpret_cast<uintptr_t>( ent.Ped );
                            ImGui::PushID( reinterpret_cast<void*>( ent.Ped ) );
                            IsSelected = ( s_TrollTargetPed == pkey );
                            if ( ImGui::Selectable( label.c_str( ), &IsSelected ) )
                                s_TrollTargetPed = pkey;
                            ImGui::PopID();
                        }
                        ImGui::PopStyleVar();
                        ImGui::EndChild();
                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 - 25 + 35, 0));
                    ImGui::BeginGroup();
                    {
                        ImGui::CustomChild(xorstr("Trolling Actions"), ImVec2(ImGui::GetWindowSize().x / 2 - 25, 330));
                        {
                            if ( !trollHasSel ) {
                                ImGui::TextColored(ImColor(150, 150, 150), xorstr("Select a player"));
                            } else if ( !trollSel.Ped ) {
                                ImGui::TextColored(ImColor(150, 150, 150), xorstr("Invalid target"));
                            } else {
                                ImGui::TextColored(ImColor(255, 100, 100), Core::SDK::Game::GetPlayerListLabel(trollSel).c_str());
                                ImGui::Separator();
                                ImGui::Spacing();

                                ImGui::PushFont(InterRegular);
                                if (ImGui::Button(xorstr("Go to player"), ImVec2(-1, 26)))
                                    MiscPedActions::TeleportLocalTo(trollSel.Pos);
                                if (ImGui::Button(xorstr("Bring to me"), ImVec2(-1, 26)))
                                    MiscPedActions::BringPedToMe(trollSel.Ped);
                                {
                                    float pairW = (ImGui::GetContentRegionAvail().x - 6.f) * 0.5f;
                                    if (ImGui::Button(xorstr("Freeze"), ImVec2(pairW, 26)))
                                        trollSel.Ped->FreezePed(true);
                                    ImGui::SameLine();
                                    if (ImGui::Button(xorstr("Unfreeze"), ImVec2(pairW, 26)))
                                        trollSel.Ped->FreezePed(false);
                                }
                                {
                                    float pairW = (ImGui::GetContentRegionAvail().x - 6.f) * 0.5f;
                                    if (ImGui::Button(xorstr("Ragdoll"), ImVec2(pairW, 26)))
                                        MiscPedActions::SetPedRagdoll(trollSel.Ped, true);
                                    ImGui::SameLine();
                                    if (ImGui::Button(xorstr("Stop ragdoll"), ImVec2(pairW, 26)))
                                        MiscPedActions::SetPedRagdoll(trollSel.Ped, false);
                                }
                                if (ImGui::Button(xorstr("Launch up"), ImVec2(-1, 26)))
                                    MiscPedActions::PushPedVelocityUp(trollSel.Ped, 24.f);
                                ImGui::PopFont();

                                ImGui::Spacing();
                                ImGui::TextColored(ImColor(120, 140, 160), xorstr("Effects need server sync; in-vehicle targets use vehicle TP."));
                            }
                        }
                        ImGui::EndCustomChild();
                    }
                    ImGui::EndGroup();
                }
                ImGui::EndGroup();
                }
                break;
            case OTHERS_SETTINGS:
                ImGui::SetCursorPos(ImVec2(20, 0));
                ImGui::BeginGroup();
                {

                    ImGui::SetCursorPos(ImVec2(20, 0));
                    ImGui::CustomChild(xorstr("Configs"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 330));
                    {

                        ImGui::PushFont(InterRegular);
                        if (ImGui::Button(xorstr("Import Config"), ImVec2(-1, 30)))
                        {
                            std::thread cfgImport([] {
                                std::string msg = g_Config.LoadCfg(xorstr("..."), Utils::GetClipboard());
                                });
                            cfgImport.detach();
                        }

                        ImGui::PushFont(InterRegular);
                        if (ImGui::Button(xorstr("Export Config"), ImVec2(-1, 30)))
                        {
                            std::thread cfgExport([] {
                                std::string CfgMsg = g_Config.SaveCurrentConfig(xorstr("..."));
                                });
                            cfgExport.detach();
                        }

                        ImGui::PushFont(InterRegular);
                        if (ImGui::Button(xorstr("Unload"), ImVec2(-1, 30)))
                        {
                            ExitProcess(0);
                        }
                    }
                    ImGui::EndCustomChild();

                    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x / 2 + 10, 0));
                    ImGui::CustomChild(xorstr("Menu Settings"), ImVec2(ImGui::GetWindowSize().x / 2 - 30, 330));
                    {
                        int KeyModeMenu = 1;
                        ImGui::KeyBind(xorstr("Menu bind"), &g_Config.General->MenuKey, &KeyModeMenu);
                        ImGui::Checkbox(xorstr("Second Monitor"), &g_Config.General->SecondMonitor);
                        ImGui::Checkbox(xorstr("VSync"), &g_Config.General->VSync);
                        if (ImGui::Checkbox(xorstr("Stream Proof"), &g_Config.General->StreamProof))
                        {
                            if (g_Config.General->StreamProof)
                                SetWindowDisplayAffinity(g_Variables.g_hCheatWindow, WDA_EXCLUDEFROMCAPTURE);
                            else
                                SetWindowDisplayAffinity(g_Variables.g_hCheatWindow, WDA_NONE);
                        }
                    }
                    ImGui::EndCustomChild();


                }
                ImGui::EndGroup();
                break;
            }

            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }


     

    void Rendering() {
        ImGui::SetNextWindowSize(g_MenuInfo.MenuSize);

       
        if (!PauseLoop) { ImGui::SetNextWindowPos(g_Variables.g_vGameWindowSize / 2 - g_MenuInfo.MenuSize / 2); PauseLoop++; }

        ImGui::PushFont(g_Variables.m_FontNormal);

        if (g_MenuInfo.IsOpen)
            Render();
        
        HWND ActiveWindow = GetForegroundWindow();
        {
            
            std::lock_guard<std::mutex> Lock(DrawMtx);

            
            static bool showed = false;

            if (ActiveWindow == g_Variables.g_hGameWindow)
            {

                if (GetAsyncKeyState(g_Config.Player->GodModeKey) & 1)
                {
                    g_Config.Player->EnableGodMode = !g_Config.Player->EnableGodMode;

                    Core::SDK::Pointers::pLocalPlayer->SetGodMode(g_Config.Player->EnableGodMode);

                }

                if (GetAsyncKeyState(g_Config.ESP->KeyBind) & 1)
                {
                    g_Config.ESP->Enabled = !g_Config.ESP->Enabled;
                }

                if (GetAsyncKeyState(g_Config.Player->NoClipKey) & 1)
                {
                    g_Config.Player->NoClipEnabled = !g_Config.Player->NoClipEnabled;

                    Core::SDK::Pointers::pLocalPlayer->FreezePed(g_Config.Player->NoClipEnabled);

                }

                if (g_Config.Player->NoClipEnabled)
                    Features::Exploits::NoClip();
            }
         

            

            if (ActiveWindow == g_Variables.g_hGameWindow || ActiveWindow == g_Variables.g_hCheatWindow)
            {

                struct FovFuncs_t {
                    bool* Enabled;
                    int* FovSize;
                    ImVec4 FovColor;
                };

                std::vector<FovFuncs_t> FovDrawList = {
                    FovFuncs_t(&g_Config.Aimbot->ShowFov, &g_Config.Aimbot->FOV, g_Config.Aimbot->FovColor),
                    FovFuncs_t(&g_Config.SilentAim->ShowFov, &g_Config.SilentAim->FOV, g_Config.SilentAim->FovColor),
                    FovFuncs_t(&g_Config.TriggerBot->ShowFov, &g_Config.TriggerBot->FOV, g_Config.TriggerBot->FovColor),
                };

                static std::vector<float> Alphas(FovDrawList.size(), 0.0f);
                static std::vector<float> Sizes(FovDrawList.size(), 0.0f);

                for (int i = 0; i < FovDrawList.size(); ++i)
                {
                    auto& Fov = FovDrawList[i];

                    Alphas[i] = ImClamp(ImLerp(Alphas[i], *Fov.Enabled ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 10.f), 0.f, 1.f);
                    Sizes[i] = ImLerp(Sizes[i], (float)*Fov.FovSize, ImGui::GetIO().DeltaTime * 12.f);

                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, Alphas[i]);

                    ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(g_Variables.g_vGameWindowCenter.x, g_Variables.g_vGameWindowCenter.y), Sizes[i], ImGui::GetColorU32(Fov.FovColor), 999);

                    ImGui::PopStyleVar();
                }

                ImGui::PushFont(g_Variables.m_DrawFont);

                Features::g_Esp.Draw();
                Features::g_Esp.DrawVehicle();
               

                ImGui::PopFont();
            }
            else {
                if (ImGui::GetStyle().Alpha >= 0.9f)
                {
                    g_MenuInfo.IsOpen = false;
                    SetWindowLong(g_Variables.g_hCheatWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT);
                }
            }
        }
        ImGui::PopFont();
    }
}

    

