#pragma once
#include <string>
#include <Includes/Includes.hpp>
#include <Includes/Utils.hpp>
#include <chrono>
#include <map>
#include <ImGui/origens.h>
#include <newarch/Fonts.hpp>

namespace Notify
{
    class NotifyData
    {
    public:
        std::string Title;
        std::string Description;
        time_t ExpireTime = 4000;
        time_t CreationTime = 0;

        float Slide = 500.f;
        float Alpha1 = 0;
        float Alpha2 = 0;
        float Timer = 0.0f;
        bool Active = true;

        NotifyData(const std::string& desc, time_t expire)
        {
            Title = xorstr("Hwid e foda");
            Description = desc;
            ExpireTime = expire;
            CreationTime = GetNow();
        }

        static time_t GetNow()
        {
            using namespace std::chrono;
            return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
        }

        float GetElapsed() const
        {
            return (float)(GetNow() - CreationTime);
        }
    };

    inline std::vector<NotifyData> Queue;

    inline void Send(const std::string& desc, time_t expire = 4000)
    {
        Beep(100, 300);
        Queue.emplace_back(desc, expire);
    }

    inline void Render()
    {
        const auto draw = ImGui::GetForegroundDrawList(ImGui::GetMainViewport());
        const ImVec2 screen = ImGui::GetMainViewport()->Size;
        const ImFont* titleFont = FontAwesomeSolid;
        const ImFont* descFont = lexend_font;

        float spacingY = 10.f;
        float totalHeight = 0.f;

        for (int i = 0; i < Queue.size(); ++i)
        {
            auto& n = Queue[i];
            if (!n.Active)
                continue;

            ImVec2 TitleSize = ImGui::CalcTextSize(n.Title.c_str());
            ImVec2 DescSize = ImGui::CalcTextSize(n.Description.c_str());

            float width = TitleSize.x > DescSize.x ? TitleSize.x : DescSize.x;
            ImVec2 NotifySize(width + 20.f, TitleSize.y + DescSize.y + 20.f);
            ImVec2 Pos(screen.x - NotifySize.x - 10.f, screen.y - NotifySize.y - 10.f - totalHeight);
            ImVec2 End(Pos.x + NotifySize.x, Pos.y + NotifySize.y);

            // Animation logic
            n.Timer += ImGui::GetIO().DeltaTime;

            if (n.Timer < 3.0f)
            {
                n.Slide = ImLerp(n.Slide, 0.f, ImGui::GetIO().DeltaTime * 6.f);
                n.Alpha1 = ImLerp(n.Alpha1, 255.f, ImGui::GetIO().DeltaTime * 4.f);
                n.Alpha2 = ImLerp(n.Alpha2, 150.f, ImGui::GetIO().DeltaTime * 4.f);
            }
            else if (n.Timer < 6.0f)
            {
                n.Slide = ImLerp(n.Slide, 500.f, ImGui::GetIO().DeltaTime * 6.f);
                n.Alpha1 = ImLerp(n.Alpha1, 0.f, ImGui::GetIO().DeltaTime * 4.f);
                n.Alpha2 = ImLerp(n.Alpha2, 0.f, ImGui::GetIO().DeltaTime * 4.f);
            }
            else
            {
                n.Active = false;
                continue;
            }

            // Drawing
            draw->AddRectFilled(ImVec2(Pos.x + n.Slide, Pos.y), ImVec2(End.x + n.Slide, End.y), ImColor(15, 15, 16, (int)n.Alpha1), 5);
            draw->AddRect(ImVec2(Pos.x + n.Slide, Pos.y), ImVec2(End.x + n.Slide, End.y), ImColor(25, 25, 25, (int)n.Alpha1), 5);

            draw->AddText(titleFont, titleFont->FontSize, ImVec2(Pos.x + 7 + n.Slide, Pos.y + 7), ImColor(200, 200, 200, (int)n.Alpha2), ICON_FA_BUG);
            draw->AddText(ImVec2(Pos.x + 30 + n.Slide, Pos.y + 8), ImColor(255, 255, 255, (int)n.Alpha2 + 20), n.Title.c_str());

            draw->AddText(descFont, descFont->FontSize, ImVec2(Pos.x + 10 + n.Slide, Pos.y + 30), ImColor(255, 255, 255, (int)n.Alpha2), n.Description.c_str());

            totalHeight += NotifySize.y + spacingY;
        }

        // Remove expired notifications
        Queue.erase(std::remove_if(Queue.begin(), Queue.end(), [](const NotifyData& n) { return !n.Active; }), Queue.end());
    }
}
