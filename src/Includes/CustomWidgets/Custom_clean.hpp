#pragma once
#include <Includes/includes.hpp>
#include <Includes/Utils.hpp>
#include <map>




#include <Fontes/byteslouna.hpp>

#include <Globals.hpp>
#include <ImGui/Files/imgui.h>

namespace Custom {


	//bool SliderInt(const char* label, int* v, int v_min, int v_max);


	using namespace ImGui;

	inline double EaseInOutCirc(double t)
	{
		if (t < 0.5)
			return (1 - std::sqrt(1 - 2 * t)) * 0.5;
		else
			return (1 + std::sqrt(2 * t - 1)) * 0.5;
	}

	inline int rotation_start_index;
	inline void ImRotateStart()
	{
		rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
	}

	inline ImVec2 ImRotationCenter()
	{
		ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX); // bounds

		const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

		return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2); // or use _ClipRectStack?
	}

	inline void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
	{
		float s = sin(rad), c = cos(rad);
		center = ImRotate(center, s, c) - center;

		auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
		for (int i = rotation_start_index; i < buf.Size; i++)
			buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
	}

	inline void ProfileBar() {
		ImVec2 WindowPos = ImGui::GetWindowPos();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();

		std::string Username = "";
		std::string Role = "";
		std::string PrimeiraLetra = Username.substr(0, 1);
		std::transform(PrimeiraLetra.begin(), PrimeiraLetra.end(), PrimeiraLetra.begin(), [](unsigned char c) { return std::toupper(c); });

		if (Username.length() > 10) {
			Username = Username.substr(0, 7) + xorstr("...");
		}

		ImVec2 PrimeiraLetraNameTextSize = Utils::CalcTextSize(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, PrimeiraLetra.c_str());
		ImVec2 NameTextSize = Utils::CalcTextSize(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, Username.c_str());
		ImVec2 RoleTextSize = Utils::CalcTextSize(g_Variables.m_FontSmaller, g_Variables.m_FontSmaller->FontSize, Role.c_str());

		ImVec2 ProfilePos = { WindowPos.x + g_MenuInfo.MenuSize.x - 30, WindowPos.y + 30 };
		//DrawList->AddCircleFilled( ProfilePos, 20, ImGui::GetColorU32( ( ImVec4 ) ImColor( 17, 17, 20 ) ), 100 );
		//DrawList->AddCircle( ProfilePos, 20, ImGui::GetColorU32( ( ImVec4 ) ( g_Col.BorderCol ) ), 100 );


		ImVec2 ProfileLetra = { ProfilePos.x - PrimeiraLetraNameTextSize.x / 2, ProfilePos.y - PrimeiraLetraNameTextSize.y / 2 - 1 };
		//DrawList->AddText( g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, ProfileLetra, ImGui::GetColorU32( ( ImVec4 ) ImColor( 163, 163, 163 ) ), PrimeiraLetra.c_str( ) );

		ImVec2 UsernamePos = { ProfilePos.x - 9 - NameTextSize.x, ProfileLetra.y - 6 };
		DrawList->AddText(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, UsernamePos, ImGui::GetColorU32((ImVec4)ImColor(163, 163, 163)), Username.c_str());

		ImVec2 RolePos = { ProfilePos.x - 9 - RoleTextSize.x, ProfileLetra.y + 8 };
		DrawList->AddText(g_Variables.m_FontSmaller, g_Variables.m_FontSmaller->FontSize, RolePos, ImGui::GetColorU32((ImVec4)ImColor(80, 80, 80)), Role.c_str());
	}




	inline bool SubTab(const char* label, bool active, bool first = false, bool last = false)
	{
		struct SubTab_t {
			ImVec4 BackgroundColor;
			ImVec4 TextColor;
			float HoverAnim;
		};

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImDrawList* DrawList = window->DrawList;

		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		std::string IdStr = std::string(label);
		const ImGuiID id = window->GetID(IdStr.c_str());
		const ImGuiIO& IO = g.IO;

		const ImVec2 pos = window->DC.CursorPos;
		auto TextSize = Utils::CalcTextSize(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, label);
		const ImVec2 size = ImVec2(TextSize.x + 20, 28);
		const ImRect rect(pos, pos + size);

		ImGui::ItemSize(rect, style.FramePadding.y);
		if (!ImGui::ItemAdd(rect, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(rect, id, &hovered, &held);
		if (pressed)
			ImGui::MarkItemEdited(id);

		static std::map<ImGuiID, SubTab_t> anim;
		auto& a = anim[id];

		// Animações
		float speed = IO.DeltaTime * 10.f;
		a.BackgroundColor = ImLerp(a.BackgroundColor,
			active ? ImVec4(1, 1, 1, 1.0f) : ImVec4(0, 0, 0, 0.0f),
			Custom::EaseInOutCirc(speed));
		a.TextColor = ImLerp(a.TextColor,
			active ? ImVec4(0.1f, 0.1f, 0.1f, 1.0f) : ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
			Custom::EaseInOutCirc(speed));
		a.HoverAnim = ImLerp(a.HoverAnim, hovered && !active ? 2.0f : 0.0f, IO.DeltaTime * 5.f);

		// Desenhar fundo arredondado somente no ativo
		if (active) {
			ImDrawFlags rounding_flags = ImDrawFlags_None;
			if (first && last)
				rounding_flags = ImDrawFlags_RoundCornersAll;
			else if (first)
				rounding_flags = ImDrawFlags_RoundCornersLeft;
			else if (last)
				rounding_flags = ImDrawFlags_RoundCornersRight;

			DrawList->AddRectFilled(
				rect.Min,
				rect.Max,
				ImGui::GetColorU32(a.BackgroundColor),
				6.0f,
				rounding_flags
			);
		}

		// Render texto centralizado
		DrawList->AddText(
			g_Variables.m_FontSecundary,
			g_Variables.m_FontSecundary->FontSize,
			ImVec2(pos.x + ((size.x - TextSize.x) / 2) + a.HoverAnim,
				pos.y + ((size.y - TextSize.y) / 2) - 1),
			ImGui::GetColorU32(a.TextColor),
			label
		);

		ImGui::SameLine(0, 0); // Cola os botões
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
		return pressed;
	}

	inline void InvisibleShadow(const float alpha, const ImVec2 pos_min, const ImVec2 pos_max, ImU32 color, float shadow_tickness)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
		ImGui::PushClipRect(pos_min, pos_max, true);
		ImGui::GetWindowDrawList()->AddShadowCircle(ImVec2(ImGui::GetMousePos()), 1.f, ImGui::GetColorU32(color), shadow_tickness, ImVec2(0, 0), 1000.f);
		ImGui::PopClipRect();
		ImGui::PopStyleVar();
	}

	inline void ToolTip(const char* Label, const char* Desc, const char* Icon, bool State) {
		//bool Check = CheckBox( Label, Checked );
		bool Hovered = State;

		ImVec2 Pos = ImGui::GetMousePos() + ImVec2(20, 0);
		auto DrawList = ImGui::GetForegroundDrawList();
		float Padding = 12;
		float Rounding = 4;
		float Spacing = 6;

		struct ToolTip_t {
			ImVec4 BackGroundColor = ImColor(20, 20, 22);
			ImVec4 TextColor = ImColor(200, 200, 200, 0);
			ImVec4 IconColor = ImColor(245, 158, 66, 0);

			float GlobalAlpha = 0.f;
			float SlideX = 0.f;
		};

		static std::map<std::string, ToolTip_t> anim;
		std::string a1 = std::string(Label) + std::string(Desc);
		auto ToolTipAnim = anim.find(a1);

		if (ToolTipAnim == anim.end()) {
			anim.insert({ a1, ToolTip_t() });
		}

		ImVec2 TextSize = Utils::CalcTextSize(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, Desc);
		ImVec2 IconTextSize = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, Icon);

		ToolTipAnim->second.GlobalAlpha = ImLerp(ToolTipAnim->second.GlobalAlpha, Hovered ? 1.f : 0.f, ImGui::GetIO().DeltaTime * 8);
		ToolTipAnim->second.SlideX = ImLerp(ToolTipAnim->second.SlideX, Hovered ? (IconTextSize.x + TextSize.x + Padding + Spacing) : 0.f, ImGui::GetIO().DeltaTime * 8);
		ToolTipAnim->second.TextColor = ImLerp(ToolTipAnim->second.TextColor, Hovered ? ImColor(180, 180, 180) : ImColor(180, 180, 180, 0), ImGui::GetIO().DeltaTime * 12);
		ToolTipAnim->second.IconColor = ImLerp(ToolTipAnim->second.IconColor, Hovered ? ImVec4(g_Col.Base) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f / 255.f), ImGui::GetIO().DeltaTime * 8);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ToolTipAnim->second.GlobalAlpha);


		ImVec2 RectEndPos = ImVec2(Pos.x + ToolTipAnim->second.SlideX, Pos.y + TextSize.y + Padding);
		DrawList->AddRectFilled(Pos, RectEndPos, ImGui::GetColorU32(ImVec4(ImColor(16, 16, 18))), Rounding);
		DrawList->AddRect(Pos, RectEndPos, ImGui::GetColorU32(ImVec4(ImColor(22, 22, 24))), Rounding);


		DrawList->AddText(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, ImVec2(Pos.x + Padding / 2, Pos.y + ((TextSize.y + Padding) / 2 - IconTextSize.y / 2)), ImGui::GetColorU32(ToolTipAnim->second.IconColor), Icon);

		if (ToolTipAnim->second.SlideX < TextSize.x + Padding) {

			//ChatGPT patrocina
			std::string ClippedDesc = Desc;
			while (ImGui::CalcTextSize(ClippedDesc.c_str()).x > ToolTipAnim->second.SlideX - Padding && !ClippedDesc.empty()) {
				ClippedDesc.pop_back();
			}

			DrawList->AddText(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, ImVec2(Pos.x + Padding / 2 + (IconTextSize.x + Spacing), Pos.y + Padding / 2), ImGui::GetColorU32(ToolTipAnim->second.TextColor), ClippedDesc.c_str());
		}
		else {
			DrawList->AddText(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, ImVec2(Pos.x + Padding / 2 + (IconTextSize.x + Spacing), Pos.y + Padding / 2), ImGui::GetColorU32(ToolTipAnim->second.TextColor), Desc);
		}

		ImGui::PopStyleVar();

		//return Check;
	}

	inline bool CheckBox(const char* Label, bool* Checked, bool bToolTip = false, const char* ToolTipMsg = "", const char* ToolTipIcon = "") {
		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		if (Window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		std::string UniqueID = (std::string)Label + std::to_string((int)Checked);
		const ImGuiID id = Window->GetID(UniqueID.c_str());

		float Width = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - (Window->ScrollbarY ? 5.f : 0.f);
		//if ( CustomWidth != 0.f ) {
		//	Width = CustomWidth - ImGui::GetWindowContentRegionMin( ).x - ( Window->ScrollbarY ? 5.f : 0.f );
		//}

		ImVec2 TextSize = ImGui::CalcTextSize(Label);
		const ImVec2 CheckBoxSize(22, 22);

		const ImVec2 Pos = Window->DC.CursorPos;
		const ImRect Rect(Pos, Pos + ImVec2(Width, CheckBoxSize.y - 4));
		const ImRect Clickable(Pos, Pos + ImVec2(CheckBoxSize.x + TextSize.x + 8, CheckBoxSize.y));

		ImGui::ItemSize(Rect, style.FramePadding.y);

		if (!ImGui::ItemAdd(Rect, id, &Clickable)) {
			IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
			return false;
		}

		bool Hovered, Held;
		bool Pressed = ImGui::ButtonBehavior(Clickable, id, &Hovered, &Held);
		if (Pressed) {
			*Checked = !(*Checked);
			ImGui::MarkItemEdited(id);
		}

		struct WidCheckBox_t {
			ImVec4 BackGroundColor = ImColor(18, 18, 20);
			ImVec4 UnCheckedBackGroundColor = ImColor(18, 18, 20);
			ImVec4 CheckColor = ImColor(18, 18, 20);
			ImVec4 LabelColor = g_Col.SecundaryText;

			ImVec2 BackGroundSize = ImVec2(0, 0);
			//ImVec2 BackGroundSizeTwo = ImVec2( 0, 0 );

			float CheckUp = 0.f;
			float AnimKeyBind = 0.f;

			float SlideX = 0.f;
			ImVec4 IconColor = ImColor(245, 158, 66, 0);
		};

		static std::map<ImGuiID, WidCheckBox_t> anim;
		auto CheckBoxAnim = anim.find(id);

		if (CheckBoxAnim == anim.end())
		{
			anim.insert({ id, WidCheckBox_t() });
			CheckBoxAnim = anim.find(id);
		}

		ImVec2 IconTextSize = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, ICON_FA_TRIANGLE_EXCLAMATION);

		//CheckBoxAnim->second.SlideX = ImLerp( CheckBoxAnim->second.SlideX, Hovered || *Checked ? ( TextSize.x + 8 ) : 0.f, ImGui::GetIO( ).DeltaTime * 6 );
		CheckBoxAnim->second.IconColor = ImLerp(CheckBoxAnim->second.IconColor, Hovered || *Checked ? ImVec4(g_Col.Base) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f / 255.f), ImGui::GetIO().DeltaTime * 12);
		CheckBoxAnim->second.LabelColor = ImLerp(CheckBoxAnim->second.LabelColor, *Checked ? ImColor(g_Col.FeaturesText) : ImColor(g_Col.SecundaryFeaturesText), g.IO.DeltaTime * 8.f);
		CheckBoxAnim->second.BackGroundColor = ImLerp(CheckBoxAnim->second.BackGroundColor, *Checked ? ImColor(g_Col.Base) : Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 10.f);
		CheckBoxAnim->second.UnCheckedBackGroundColor = ImLerp(CheckBoxAnim->second.UnCheckedBackGroundColor, Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 6.f);
		CheckBoxAnim->second.BackGroundSize = ImLerp(CheckBoxAnim->second.BackGroundSize, (*Checked) ? ImVec2(CheckBoxSize / 2) : ImVec2(0, 0), g.IO.DeltaTime * 8.f);

		if (CheckBoxAnim->second.BackGroundSize.x > (CheckBoxSize.x / 2) - 2.f && (*Checked)) {
			CheckBoxAnim->second.CheckColor = ImLerp(CheckBoxAnim->second.CheckColor, ImColor(21, 21, 23, 255), g.IO.DeltaTime * 14.f);
			CheckBoxAnim->second.CheckUp = ImLerp(CheckBoxAnim->second.CheckUp, CheckBoxSize.x / 4, g.IO.DeltaTime * 6.f);
		}
		else {
			CheckBoxAnim->second.CheckColor = ImLerp(CheckBoxAnim->second.CheckColor, ImColor(21, 21, 23, 0), g.IO.DeltaTime * 14.f);
			CheckBoxAnim->second.CheckUp = ImLerp(CheckBoxAnim->second.CheckUp, 0.f, g.IO.DeltaTime * 6.f);
		}

		ImVec2 CheckBoxCenter(Rect.Min + CheckBoxSize / 2);
		Window->DrawList->AddRectFilled(Rect.Min, Rect.Min + CheckBoxSize, ImGui::GetColorU32(CheckBoxAnim->second.UnCheckedBackGroundColor), 4.f);
		Window->DrawList->AddRect(Rect.Min, Rect.Min + CheckBoxSize, ImGui::GetColorU32(ImVec4(ImColor(24, 24, 26))), 4.f);
		Window->DrawList->AddRectFilled(CheckBoxCenter - CheckBoxAnim->second.BackGroundSize, CheckBoxCenter + CheckBoxAnim->second.BackGroundSize, ImGui::GetColorU32(CheckBoxAnim->second.BackGroundColor), 4.f);

		ImGui::RenderCheckMark(Window->DrawList, ImVec2(Pos.x + CheckBoxSize.x / 4, Pos.y + CheckBoxSize.x / 2 - CheckBoxAnim->second.CheckUp), ImGui::GetColorU32(CheckBoxAnim->second.CheckColor), CheckBoxSize.x / 2);
		//Window->DrawList->AddRectFilled( ImVec2( Rect.Min.x + CheckBoxAnim->second.BackGroundSizeTwo.x -3, Rect.Min.y + 4 ), ImVec2( Rect.Min.x + CheckBoxAnim->second.BackGroundSizeTwo.x, Rect.Min.y + CheckBoxSize.y - 2 ), ImGui::GetColorU32( CheckBoxAnim->second.BackGroundColor ), 4.f );

		//Window->DrawList->AddLine( CheckBoxCenter - ImVec2( 6, 0 ) + ImVec2( 0 , 0 ), CheckBoxCenter + ImVec2( 0, 4 ), ImGui::GetColorU32( ImVec4( ImColor( 200, 200, 200 ) ) ), 2 );
		//Window->DrawList->AddLine( CheckBoxCenter + ImVec2( 0, 4 ), CheckBoxCenter + ImVec2( 4, 0 ) - ImVec2( 0, 5 ), ImGui::GetColorU32( ImVec4( ImColor( 200, 200, 200 ) ) ), 2 );

		if (bToolTip)
		{
			ToolTip(Label, ToolTipMsg, ToolTipIcon, Hovered && g_MenuInfo.IsOpen);
			//Window->DrawList->AddText( g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, ImVec2( Rect.Min.x + CheckBoxSize.x + 8.f + ( TextSize.x + 8 ), Pos.y + Rect.Max.y / 2 - ( Pos.y + IconTextSize.y ) / 2 ), ImGui::GetColorU32( CheckBoxAnim->second.IconColor ), ICON_FA_TRIANGLE_EXCLAMATION );
		}

		Window->DrawList->AddText(ImVec2(Rect.Min.x + CheckBoxSize.x + 8.f, Pos.y + Rect.Max.y / 2 - (Pos.y + TextSize.y) / 2), ImGui::GetColorU32(CheckBoxAnim->second.LabelColor), Label);

		//Window->DrawList->AddText( g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize, ImVec2( Rect.Min.x + CheckBoxSize.x + 8.f, Pos.y + Rect.Max.y / 2 - ( Pos.y + TextSize.y ) / 2 ) + ImVec2( TextSize.x + 8, 0 ), ImGui::GetColorU32( ImVec4( ImColor( 204, 76, 67 ) ) ), ICON_FA_CIRCLE_EXCLAMATION );
		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*Checked ? ImGuiItemStatusFlags_Checked : 0));
		return Pressed;
	}


	inline bool CheckboxLounaN(const char* label, bool* v)
	{
		extern cColors g_Colors; // <- usa o g_Colors automaticamente

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		ImGuiStyle& style = g.Style;
		ImGuiID id = window->GetID(label);
		ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

		float w = ImGui::GetWindowWidth();
		ImVec2 size = { 30, 17 };

		ImVec2 pos = window->DC.CursorPos;
		ImVec2 frame_pos = pos + ImVec2(w - size.x, 0);
		ImRect frame_bb(frame_pos, frame_pos + size);
		ImRect total_bb(pos, pos + ImVec2(w, label_size.y));

		ImGui::ItemSize(total_bb, style.FramePadding.y);
		if (!ImGui::ItemAdd(total_bb, id))
			return false;

		bool hovered, held;
		bool pressed = ImGui::ButtonBehavior(frame_bb, id, &hovered, &held);
		if (pressed)
		{
			*v = !(*v);
			ImGui::MarkItemEdited(id);
		}

		static std::unordered_map<ImGuiID, float> animation;
		auto it = animation.find(id);
		if (it == animation.end())
			it = animation.insert({ id, *v ? 1.0f : 0.0f }).first;

		it->second = ImLerp(it->second, (*v ? 1.0f : 0.0f), 0.15f * g.IO.DeltaTime * 60.f);

		auto draw = ImGui::GetWindowDrawList();

		// Texto
		draw->AddText(
			ImVec2(total_bb.Min.x, frame_bb.GetCenter().y - label_size.y / 2),
			ImGui::GetColorU32(g_Col.PrimaryText),
			label
		);

		// Fundo do switch
		draw->AddRectFilled(
			frame_bb.Min, frame_bb.Max,
			ImGui::GetColorU32(g_Col.ChildCol),
			10.0f
		);

		// Overlay se ativado
		if (*v)
		{
			draw->AddRectFilled(
				frame_bb.Min, frame_bb.Max,
				ImGui::GetColorU32(g_Col.TestingTest),
				10.0f
			);
		}

		// Borda
		draw->AddRect(
			frame_bb.Min, frame_bb.Max,
			ImGui::GetColorU32(g_Col.BorderCol),
			10.0f
		);

		// Circulo animado
		ImVec2 circle_pos = ImVec2(
			frame_bb.Min.x + 8 + (14.0f * it->second),
			frame_bb.GetCenter().y
		);
		draw->AddCircleFilled(
			circle_pos, 7.0f,
			ImGui::GetColorU32(g_Col.TestingTest),
			30
		);

		return pressed;
	}

	inline void RenderCheckMark(float t, ImVec2 pos, const ImVec4& color) {
		// Cria um ponteiro para a lista de desenho do ImGui
		ImDrawList* draw_list = ImGui::GetWindowDrawList();

		// Definindo o tamanho do "check mark"
		float check_size = 12.0f;  // O tamanho do "check mark"

		// Posição de cada ponto que forma o "check mark"
		ImVec2 p0 = pos;
		ImVec2 p1 = ImVec2(pos.x + check_size * 0.6f, pos.y + check_size * 0.6f);  // Parte superior do "check mark"
		ImVec2 p2 = ImVec2(pos.x + check_size, pos.y);  // Parte inferior direita do "check mark"

		// Desenha a animação (uma variação simples com base em 't')
		float offset = check_size * t;  // O valor de 't' é usado para animar a posição

		// Adiciona as linhas para desenhar o "check mark"
		draw_list->AddLine(p0, p1 + ImVec2(0, offset), ImColor(color), 2.0f);  // Linha superior
		draw_list->AddLine(p1 + ImVec2(0, offset), p2 + ImVec2(0, offset), ImColor(color), 2.0f);  // Linha inferior

		// Para adicionar mais detalhes ou formas, você pode usar funções adicionais, como AddBezierCurve
	}


	inline bool CheckboxLouna(const char* label, bool* v) {
		ImGui::NewLine();
		SetCursorPosX(9);
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);

		const ImRect check_bb(ImVec2(window->DC.CursorPos.x + 5, window->DC.CursorPos.y + 5), window->DC.CursorPos - ImVec2(5, 5) + ImVec2(label_size.y + style.FramePadding.y * 2, label_size.y + style.FramePadding.y * 2));
		ItemSize(check_bb, style.FramePadding.y);

		ImRect total_bb = check_bb;
		if (label_size.x > 0)
			SameLine(0, style.ItemInnerSpacing.x);
		const ImRect text_bb(window->DC.CursorPos + ImVec2(0, style.FramePadding.y), window->DC.CursorPos + ImVec2(0, style.FramePadding.y) + label_size);
		if (label_size.x > 0)
		{
			ItemSize(ImVec2(text_bb.GetWidth(), check_bb.GetHeight()), style.FramePadding.y);
			total_bb = ImRect(ImMin(check_bb.Min, text_bb.Min), ImMax(check_bb.Max, text_bb.Max));
		}

		if (!ItemAdd(total_bb, id))
			return false;

		bool hovered, held;
		bool pressed = ButtonBehavior(check_bb, id, &hovered, &held, 0);
		if (pressed)
		{
			*v = !(*v);
			MarkItemEdited(id);
		}

		float t = *v ? 1.0f : 0.0f;

		// Animation for hover
		float deltatime = 1.5f * ImGui::GetIO().DeltaTime;
		static std::map<ImGuiID, float> hover_animation;
		auto it_hover = hover_animation.find(id);
		if (it_hover == hover_animation.end())
		{
			hover_animation.insert({ id, 0.f });
			it_hover = hover_animation.find(id);
		}

		if (*v)
			it_hover->second = clamp(it_hover->second + (3.f * ImGui::GetIO().DeltaTime * (hovered ? 1.f : -1.f)), 1.0f, 1.f);
		else
			it_hover->second = clamp(it_hover->second + (3.f * ImGui::GetIO().DeltaTime), 1.0f, 1.f);

		const ImVec4 hover_dis = ImColor(141, 158, 255, 255);
		const ImVec4 hover_act = ImColor(10, 10, 10, 255);

		static std::map<ImGuiID, ImVec4> hover_color;
		auto it_hcolor = hover_color.find(id);
		if (it_hcolor == hover_color.end())
		{
			hover_color.insert({ id, hover_dis });
			it_hcolor = hover_color.find(id);
		}

		// Animating the hover color
		if (*v)
		{
			ImVec4 to = hover_dis;
			if (it_hcolor->second.x != to.x)
			{
				if (it_hcolor->second.x < to.x)
					it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
				else if (it_hcolor->second.x > to.x)
					it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
			}

			if (it_hcolor->second.y != to.y)
			{
				if (it_hcolor->second.y < to.y)
					it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
				else if (it_hcolor->second.y > to.y)
					it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
			}

			if (it_hcolor->second.z != to.z)
			{
				if (it_hcolor->second.z < to.z)
					it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
				else if (it_hcolor->second.z > to.z)
					it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
			}
		}
		else
		{
			ImVec4 to = hovered ? hover_dis : hover_act;
			if (it_hcolor->second.x != to.x)
			{
				if (it_hcolor->second.x < to.x)
					it_hcolor->second.x = ImMin(it_hcolor->second.x + deltatime, to.x);
				else if (it_hcolor->second.x > to.x)
					it_hcolor->second.x = ImMax(to.x, it_hcolor->second.x - deltatime);
			}

			if (it_hcolor->second.y != to.y)
			{
				if (it_hcolor->second.y < to.y)
					it_hcolor->second.y = ImMin(it_hcolor->second.y + deltatime, to.y);
				else if (it_hcolor->second.y > to.y)
					it_hcolor->second.y = ImMax(to.y, it_hcolor->second.y - deltatime);
			}

			if (it_hcolor->second.z != to.z)
			{
				if (it_hcolor->second.z < to.z)
					it_hcolor->second.z = ImMin(it_hcolor->second.z + deltatime, to.z);
				else if (it_hcolor->second.z > to.z)
					it_hcolor->second.z = ImMax(to.z, it_hcolor->second.z - deltatime);
			}
		}

		const ImVec4 text_dis = ImVec4(180 / 255.f, 180 / 255.f, 180 / 255.f, 1.f);
		const ImVec4 text_act = ImVec4(252 / 255.f, 252 / 255.f, 252 / 255.f, 1.f);

		static std::map<ImGuiID, ImVec4> text_animation;
		auto it_text = text_animation.find(id);
		if (it_text == text_animation.end())
		{
			text_animation.insert({ id, text_dis });
			it_text = text_animation.find(id);
		}

		// Animate text color
		if (*v)
		{
			ImVec4 to = hovered ? text_dis : text_act;
			if (it_text->second.x != to.x)
			{
				if (it_text->second.x < to.x)
					it_text->second.x = ImMin(it_text->second.x + deltatime, to.x);
				else if (it_text->second.x > to.x)
					it_text->second.x = ImMax(to.x, it_text->second.x - deltatime);
			}

			if (it_text->second.y != to.y)
			{
				if (it_text->second.y < to.y)
					it_text->second.y = ImMin(it_text->second.y + deltatime, to.y);
				else if (it_text->second.y > to.y)
					it_text->second.y = ImMax(to.y, it_text->second.y - deltatime);
			}

			if (it_text->second.z != to.z)
			{
				if (it_text->second.z < to.z)
					it_text->second.z = ImMin(it_text->second.z + deltatime, to.z);
				else if (it_text->second.z > to.z)
					it_text->second.z = ImMax(to.z, it_text->second.z - deltatime);
			}
		}
		else
		{
			ImVec4 to = text_dis;
			if (it_text->second.x != to.x)
			{
				if (it_text->second.x < to.x)
					it_text->second.x = ImMin(it_text->second.x + deltatime, to.x);
				else if (it_text->second.x > to.x)
					it_text->second.x = ImMax(to.x, it_text->second.x - deltatime);
			}

			if (it_text->second.y != to.y)
			{
				if (it_text->second.y < to.y)
					it_text->second.y = ImMin(it_text->second.y + deltatime, to.y);
				else if (it_text->second.y > to.y)
					it_text->second.y = ImMax(to.y, it_text->second.y - deltatime);
			}

			if (it_text->second.z != to.z)
			{
				if (it_text->second.z < to.z)
					it_text->second.z = ImMin(it_text->second.z + deltatime, to.z);
				else if (it_text->second.z > to.z)
					it_text->second.z = ImMax(to.z, it_text->second.z - deltatime);
			}
		}

		if (*v) {
			// Render the checkbox frame
			ImGui::RenderFrame(check_bb.Min, check_bb.Max, ImColor(29, 29, 29), true, 2.5f);

			// Render the checkmark
			window->DrawList->AddLine(
				ImVec2(total_bb.Min.x + 1, (total_bb.Min.y + total_bb.Max.y) / 2 - 6.5),
				ImVec2(total_bb.Min.x + 5, (total_bb.Min.y + total_bb.Max.y) / 2),
				ImColor(29, 29, 29),
				2.5f
			);
		}
		else {
			ImGui::RenderFrame(check_bb.Min, check_bb.Max, ImColor(29, 29, 29), true, 2.5f);
		}

		// Render the label
		if (*v) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(202 / 255.f, 202 / 255.f, 202 / 255.f, 1.f));
		}
		else {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(123 / 255.f, 123 / 255.f, 123 / 255.f, 1.f));
		}

		if (label_size.x > 0.0f)
			RenderText(text_bb.GetTL() + ImVec2(8, 0) - ImVec2(0, 2.1), label);
		ImGui::PopStyleColor();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | ImGuiItemStatusFlags_AllowKeyboardFocus);
		return pressed;
	}


	inline bool CheckBoxPage(const char* Label, bool* Checked, std::function<void()> Code, bool bToolTip = false, const char* ToolTipMsg = "") {
		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		if (Window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		std::string UniqueID = (std::string)Label + std::to_string((int)Checked);
		const ImGuiID id = Window->GetID(UniqueID.c_str());

		float Width = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - (Window->ScrollbarY ? 5.f : 0.f);

		ImVec2 TextSize = ImGui::CalcTextSize(Label);
		const ImVec2 CheckBoxSize(22, 22);

		const ImVec2 Pos = Window->DC.CursorPos;
		const ImRect Rect(Pos, Pos + ImVec2(Width, CheckBoxSize.y - 4));
		const ImRect Clickable(Pos, Pos + ImVec2(CheckBoxSize.x + TextSize.x + 8, CheckBoxSize.y));

		ImGui::ItemSize(Rect, style.FramePadding.y);

		if (!ImGui::ItemAdd(Rect, id, &Clickable)) {
			IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
			return false;
		}

		bool Hovered, Held;
		bool Pressed = ImGui::ButtonBehavior(Clickable, id, &Hovered, &Held);
		if (Pressed) {
			*Checked = !(*Checked);
			ImGui::MarkItemEdited(id);
		}

		struct WidCheckBox_t {
			ImVec4 BackGroundColor = ImColor(18, 18, 20);
			ImVec4 UnCheckedBackGroundColor = ImColor(18, 18, 20);
			ImVec4 CheckColor = ImColor(18, 18, 20);
			ImVec4 LabelColor = g_Col.SecundaryText;

			ImVec2 BackGroundSize = ImVec2(0, 0);

			float CheckUp = 0.f;
			float AnimKeyBind = 0.f;

			float SlideUp = 0.f;
			ImVec4 IconColor = ImColor(245, 158, 66, 0);
		};

		static std::map<ImGuiID, WidCheckBox_t> anim;
		auto CheckBoxAnim = anim.find(id);

		if (CheckBoxAnim == anim.end())
		{
			anim.insert({ id, WidCheckBox_t() });
			CheckBoxAnim = anim.find(id);
		}

		CheckBoxAnim->second.LabelColor = ImLerp(CheckBoxAnim->second.LabelColor, *Checked ? ImColor(g_Col.FeaturesText) : ImColor(g_Col.SecundaryFeaturesText), g.IO.DeltaTime * 8.f);
		CheckBoxAnim->second.BackGroundColor = ImLerp(CheckBoxAnim->second.BackGroundColor, *Checked ? ImColor(g_Col.Base) : Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 10.f);
		CheckBoxAnim->second.UnCheckedBackGroundColor = ImLerp(CheckBoxAnim->second.UnCheckedBackGroundColor, Hovered ? ImColor(20, 20, 22) : ImColor(18, 18, 20), g.IO.DeltaTime * 6.f);
		CheckBoxAnim->second.BackGroundSize = ImLerp(CheckBoxAnim->second.BackGroundSize, (*Checked) ? ImVec2(CheckBoxSize / 2) : ImVec2(0, 0), g.IO.DeltaTime * 8.f);

		if (CheckBoxAnim->second.BackGroundSize.x > (CheckBoxSize.x / 2) - 2.f && (*Checked)) {
			CheckBoxAnim->second.CheckColor = ImLerp(CheckBoxAnim->second.CheckColor, ImColor(21, 21, 23, 255), g.IO.DeltaTime * 14.f);
			CheckBoxAnim->second.CheckUp = ImLerp(CheckBoxAnim->second.CheckUp, CheckBoxSize.x / 4, g.IO.DeltaTime * 6.f);
		}
		else {
			CheckBoxAnim->second.CheckColor = ImLerp(CheckBoxAnim->second.CheckColor, ImColor(21, 21, 23, 0), g.IO.DeltaTime * 14.f);
			CheckBoxAnim->second.CheckUp = ImLerp(CheckBoxAnim->second.CheckUp, 0.f, g.IO.DeltaTime * 6.f);
		}

		ImVec2 CheckBoxCenter(Rect.Min + CheckBoxSize / 2);

		Window->DrawList->AddRectFilled(Rect.Min, Rect.Min + CheckBoxSize, ImGui::GetColorU32(CheckBoxAnim->second.UnCheckedBackGroundColor), 4.f);
		Window->DrawList->AddRect(Rect.Min, Rect.Min + CheckBoxSize, ImGui::GetColorU32(ImVec4(ImColor(24, 24, 26))), 4.f);


		Window->DrawList->AddRectFilled(CheckBoxCenter - CheckBoxAnim->second.BackGroundSize, CheckBoxCenter + CheckBoxAnim->second.BackGroundSize, ImGui::GetColorU32(CheckBoxAnim->second.BackGroundColor), 4.f);

		ImGui::RenderCheckMark(Window->DrawList, ImVec2(Pos.x + CheckBoxSize.x / 4, Pos.y + CheckBoxSize.x / 2 - CheckBoxAnim->second.CheckUp), ImGui::GetColorU32(CheckBoxAnim->second.CheckColor), CheckBoxSize.x / 2);


		ImVec2 TextPos = ImVec2(Rect.Min.x + CheckBoxSize.x + 8.f, Pos.y + Rect.Max.y / 2 - (Pos.y + TextSize.y) / 2);
		Window->DrawList->AddText(TextPos, ImGui::GetColorU32(CheckBoxAnim->second.LabelColor), Label);

		////////////////////////////
		// Link Obj
		////////////////////////////
		static bool IconHovered = false;
		ImVec2 IconTextSize = Utils::CalcTextSize(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize - 4, ICON_FA_SHARE);
		ImVec2 MousePos = ImGui::GetMousePos();
		ImVec2 MinPos = TextPos + ImVec2(TextSize.x, (CheckBoxSize.y / 2 - TextSize.y / 2) - 4);
		ImVec2 MaxPos = TextPos + ImVec2(TextSize.x + (10 * 2) + IconTextSize.x, (MinPos.y - TextPos.y) + IconTextSize.y + 4);

		if (MousePos.x > MinPos.x && MousePos.x < MaxPos.x && MousePos.y > MinPos.y && MousePos.y < MaxPos.y) {
			IconHovered = true;
		}
		else {
			IconHovered = false;
		}

		if (bToolTip) {
			ToolTip(Label, ToolTipMsg, ICON_FA_SHARE, IconHovered && g_MenuInfo.IsOpen);
		}

		CheckBoxAnim->second.SlideUp = ImLerp(CheckBoxAnim->second.SlideUp, IconHovered ? 2.f : 0.f,
			g.IO.DeltaTime * 10.f);
		CheckBoxAnim->second.IconColor = ImLerp(CheckBoxAnim->second.IconColor, IconHovered
			? ImColor(g_Col.Base) : ImColor(60, 60, 60), g.IO.DeltaTime * 10.f);

		Window->DrawList->AddText(g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize - 4
			, TextPos + ImVec2(TextSize.x + 10, (CheckBoxSize.y / 2 - TextSize.y / 2) - CheckBoxAnim->second.SlideUp),
			ImGui::GetColorU32(CheckBoxAnim->second.IconColor), ICON_FA_SHARE);

		if (IconHovered && g.IO.MouseClicked[0]) {
			Code();
		}

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*Checked ? ImGuiItemStatusFlags_Checked : 0));
		return Pressed;
	}

	inline void TextCentered(const char* text, int m) {

		ImVec2 textSize = ImGui::CalcTextSize(text);
		float posX = (g_MenuInfo.MenuSize.x - textSize.x) * 0.5f;
		float posY = (g_MenuInfo.MenuSize.y - textSize.y) * 0.5f;

		switch (m) {

		case 0:
			ImGui::SetCursorPos({ posX, posY });
			ImGui::Text(text);
			break;
		case 1:
			ImGui::SetCursorPosX(posX);
			ImGui::Text(text);
			break;
		case 2:
			ImGui::SetCursorPosY(posY);
			ImGui::Text(text);
			break;
		default:
			ImGui::SetCursorPos({ posX, posY });
			ImGui::Text(text);
			break;

		}
	}


	inline bool ButtonWithIcon(const char* icon, const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags) {
		struct button_struct {
			ImVec4 BorderCol;
			ImVec4 background;
			ImVec4 LabelColor;
			float UpBackground;
		};

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = g_Variables.m_FontNormal->CalcTextSizeA(g_Variables.m_FontNormal->FontSize, FLT_MAX, 0, label);
		const ImVec2 IconTextSize = g_Variables.FontAwesomeSolid->CalcTextSizeA(g_Variables.FontAwesomeSolid->FontSize - 4, FLT_MAX, 0, icon);
		const ImVec2 pos = window->DC.CursorPos;

		static std::map<ImGuiID, button_struct> anim;
		auto it_anim = anim.find(id);

		if (it_anim == anim.end()) {
			anim.insert({ id, button_struct() });
			it_anim = anim.find(id);
		}

		ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + IconTextSize.x + style.FramePadding.x * 3.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ImGui::ItemSize(size, 0.f);

		if (!ImGui::ItemAdd(bb, id))
			return false;

		bool Hovered, Held, Pressed = ImGui::ButtonBehavior(bb, id, &Hovered, &Held, flags);

		it_anim->second.BorderCol = ImLerp(it_anim->second.BorderCol, Hovered ? ImVec4(g_Col.Base) : ImVec4(ImColor(18, 18, 20)), g.IO.DeltaTime * 6.f);
		it_anim->second.background = ImLerp(it_anim->second.background, Hovered ? ImVec4(ImColor(14, 14, 16)) : ImVec4(ImColor(14, 14, 16)), g.IO.DeltaTime * 6.f);
		it_anim->second.LabelColor = ImLerp(it_anim->second.LabelColor, Hovered ? ImVec4(ImColor(225, 225, 225)) : ImVec4(ImColor(225, 225, 225)), g.IO.DeltaTime * 6.f);
		it_anim->second.UpBackground = ImLerp(it_anim->second.UpBackground, Hovered ? (bb.Min.y - pos.y) - (bb.Max.y - pos.y) : 0.f, g.IO.DeltaTime * 6.f);

		float totalWidth = label_size.x + IconTextSize.x + style.FramePadding.x;
		ImVec2 TextPos = { pos.x + (size.x - totalWidth) * 0.5f + IconTextSize.x + style.FramePadding.x, pos.y + (size.y - label_size.y) * 0.5f - 0.5f };
		ImVec2 TextWithoutIconPos = { pos.x + (size.x - label_size.x) * 0.5f + style.FramePadding.x, pos.y + (size.y - label_size.y) * 0.5f - 0.5f };
		ImVec2 IconPos = { pos.x + (size.x - totalWidth) * 0.5f, pos.y + (size.y - IconTextSize.y) * 0.5f - 0.5f };

		window->DrawList->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(it_anim->second.background), 4);
		window->DrawList->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(it_anim->second.BorderCol), 4);

		window->DrawList->AddRectFilled(ImVec2(bb.Min.x, bb.Max.y), ImVec2(bb.Max.x, bb.Max.y + it_anim->second.UpBackground), ImGui::GetColorU32(g_Col.Base), 4);

		//window->DrawList->AddText( g_Variables.FontAwesomeSolid, g_Variables.FontAwesomeSolid->FontSize - 4, IconPos, ImGui::GetColorU32( it_anim->second.LabelColor ), icon );
		window->DrawList->AddText(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, TextWithoutIconPos, ImGui::GetColorU32(it_anim->second.LabelColor), label);

		return Pressed;
	}




	inline bool ButtonHeld(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
	{
		struct button13Anims {
			float closing_anim;
			float closing_alpha;
			float label_alpha;
			bool animation_complete;
		};

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = window->GetID(label);
		const ImVec2 label_size = CalcTextSize(label, NULL, true);
		ImDrawList* draw = GetWindowDrawList();

		static std::map<ImGuiID, button13Anims> anim; // #include <map> on top of this file, where is "imgui.h" located.
		auto it_anim = anim.find(id);
		if (it_anim == anim.end())
		{
			anim.insert({ id, button13Anims() });
			it_anim = anim.find(id);
		}

		ImVec2 pos = window->DC.CursorPos;
		if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
			pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
		ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

		const ImRect bb(pos, pos + size);
		ItemSize(size, style.FramePadding.y);
		if (!ItemAdd(bb, id))
			return false;

		if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
			flags |= ImGuiButtonFlags_Repeat;

		bool hovered, held;
		bool pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

		it_anim->second.closing_anim = ImLerp(it_anim->second.closing_anim, (held ? size.y : 0), g.IO.DeltaTime * 8.f);

		if (held || pressed) {
			if (it_anim->second.label_alpha < 255.f)
				it_anim->second.label_alpha += 5.f / GetIO().Framerate * 160.f;

			if (it_anim->second.closing_alpha < 255.f)
				it_anim->second.closing_alpha += 15.f / GetIO().Framerate * 160.f;
		}
		else {
			if (it_anim->second.label_alpha > 0.f)
				it_anim->second.label_alpha -= 5.f / GetIO().Framerate * 160.f;

			if (it_anim->second.closing_alpha > 0.f)
				it_anim->second.closing_alpha -= 10.f / GetIO().Framerate * 160.f;
		}

		// Render
		const ImU32 inside_solid_col = GetColorU32(ImVec4(ImColor(16, 16, 18)));
		const ImU32 outside_solid_col = GetColorU32(ImVec4(ImColor(24, 24, 26)));
		const ImU32 inside_hover_col = ImColor(180, 180, 180, (int)it_anim->second.closing_alpha);

		draw->AddRectFilled(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y), inside_solid_col, 6);
		draw->AddRect(bb.Min, bb.Max, outside_solid_col, 6);
		draw->AddRectFilled(ImVec2(bb.Min.x, bb.Max.y - it_anim->second.closing_anim), ImVec2(bb.Max.x, bb.Max.y), inside_hover_col, 6);

		PushStyleColor(ImGuiCol_Text, ColorConvertFloat4ToU32(ImColor(140, 140, 140, 255 - (int)it_anim->second.label_alpha)));
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		PopStyleColor();

		PushStyleColor(ImGuiCol_Text, ColorConvertFloat4ToU32(ImColor(225, 225, 225, (int)it_anim->second.label_alpha)));
		RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);
		PopStyleColor();

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);

		bool completed = it_anim->second.closing_anim >= (size.y - 1.f);
		if (completed && !it_anim->second.animation_complete) {
			it_anim->second.animation_complete = true;
			return true;
		}

		if (pressed) {
			it_anim->second.animation_complete = false;
		}

		return false;
	}

	inline bool WeaponButtonHeld(ImTextureID Icon, const char* label, ImGuiButtonFlags flags)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImVec2 LabelSize = Utils::CalcTextSize(g_Variables.m_FontSecundary, g_Variables.m_FontSecundary->FontSize, label);
		const ImGuiID id = window->GetID(label);
		ImDrawList* Draw = window->DrawList;

		struct WeaponButtonHeld_t {
			ImVec4 ProgressCol = ImColor();
			ImVec4 ShadowProgressCol = ImColor();
			ImVec4 SlideCol = ImColor();
			float Alpha = 0.f;
			float SlideX = 0.f;
			float SlideXShadow = 0.f;
			float IconSize = 0.f;
			bool Completed = false;
		};

		static std::map<ImGuiID, WeaponButtonHeld_t> anim;
		auto WeaponButtonAnim = anim.find(id);
		if (WeaponButtonAnim == anim.end())
		{
			anim.insert({ id, WeaponButtonHeld_t() });
			WeaponButtonAnim = anim.find(id);
		}

		const float Width = 100;
		const float Height = Width;
		ImVec2 Pos = window->DC.CursorPos;
		const ImRect Rect(Pos, Pos + ImVec2(Width, Height));
		ItemSize(ImVec2(Width, Height), style.FramePadding.y);
		if (!ItemAdd(Rect, id))
			return false;

		if (g.LastItemData.InFlags & ImGuiItemFlags_ButtonRepeat)
			flags |= ImGuiButtonFlags_Repeat;

		bool Hovered, Held;
		bool Pressed = ButtonBehavior(Rect, id, &Hovered, &Held, flags);

		bool Condition = WeaponButtonAnim->second.Completed ? false : Held;

		WeaponButtonAnim->second.Alpha = ImLerp(WeaponButtonAnim->second.Alpha, Condition ? 0.8f : Hovered ? 1.f : 0.6f, g.IO.DeltaTime * 8.f);
		WeaponButtonAnim->second.IconSize = ImLerp(WeaponButtonAnim->second.IconSize, Condition ? 4.f : 0.f, g.IO.DeltaTime * 8.f);
		WeaponButtonAnim->second.SlideX = ImLerp(WeaponButtonAnim->second.SlideX, Condition ? Width : 0.f, g.IO.DeltaTime * 2.2f);
		WeaponButtonAnim->second.SlideXShadow = ImLerp(WeaponButtonAnim->second.SlideXShadow, WeaponButtonAnim->second.SlideX >= 1.f ? WeaponButtonAnim->second.SlideX + 1.f : WeaponButtonAnim->second.SlideX, g.IO.DeltaTime * 3.f);
		WeaponButtonAnim->second.ProgressCol = ImLerp(WeaponButtonAnim->second.ProgressCol, WeaponButtonAnim->second.SlideX >= 0.6f ? ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 255.f / 255.f) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f), g.IO.DeltaTime * 18.f);
		WeaponButtonAnim->second.ShadowProgressCol = ImLerp(WeaponButtonAnim->second.ShadowProgressCol, WeaponButtonAnim->second.SlideXShadow >= 0.6f ? ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.4f) : ImVec4(g_Col.Base.x, g_Col.Base.y, g_Col.Base.z, 0.f), g.IO.DeltaTime * 18.f);

		float Size = (10 + WeaponButtonAnim->second.IconSize);
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g.Style.Alpha * WeaponButtonAnim->second.Alpha);
		{
			Draw->AddRectFilled(Rect.Min, Rect.Max, GetColorU32(ImVec4(ImColor(20, 20, 22))), 8);
			//Draw->AddRectFilledMultiColor( Rect.Min, Rect.Max, GetColorU32( ImVec4( ImColor( g_Col.Base ) ), WeaponButtonAnim->second.Alpha ), GetColorU32( ImVec4( ImColor( g_Col.Base ) ), WeaponButtonAnim->second.Alpha ), GetColorU32( ImVec4( ImColor( 20, 20, 22, 0 ) ) ), GetColorU32( ImVec4( ImColor( 20, 20, 22, 0 ) ) ), 8 );
			Draw->AddRectFilled(Pos + ImVec2(0, Height - 4), Pos + ImVec2(WeaponButtonAnim->second.SlideXShadow, Height), GetColorU32(WeaponButtonAnim->second.ShadowProgressCol), 8);
			Draw->AddRectFilled(Pos + ImVec2(0, Height - 4), Pos + ImVec2(WeaponButtonAnim->second.SlideX, Height), GetColorU32(WeaponButtonAnim->second.ProgressCol), 8);
			Draw->AddImage(Icon, ImVec2(Rect.Min.x + Size, Rect.Min.y + Size), ImVec2(Rect.Max.x - Size, Rect.Max.y - Size), { 0,0 }, { 1,1 }, GetColorU32(ImVec4(ImColor(255, 255, 255))));
		}
		ImGui::PopStyleVar();

		Draw->AddRect(Rect.Min - ImVec2(4, 4), Rect.Max + ImVec2(4, 4), GetColorU32(g_Col.BackgroundCol), 8, 0, 8.f);

		IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);

		bool Completed = WeaponButtonAnim->second.SlideX >= (Width - 1.f);

		if (WeaponButtonAnim->second.Completed && Held) {
			return false;
		}


		if (Completed) {
			WeaponButtonAnim->second.Completed = true;
			return true;
		}

		if (!Held) {
			WeaponButtonAnim->second.Completed = false;
		}

		return false;
	}

	inline bool ResourceListButton(const char* Label, uint32_t ResourceState, ImVec2 SizeArg, ImGuiButtonFlags flags) {


		ImGuiWindow* Window = ImGui::GetCurrentWindow();
		if (Window->SkipItems)
			return false;

		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID id = Window->GetID(Label);
		const ImVec2 LabelSize = g_Variables.m_FontNormal->CalcTextSizeA(g_Variables.m_FontNormal->FontSize, FLT_MAX, 0, Label);
		const ImVec2 TwoTextSize = g_Variables.m_FontSecundary->CalcTextSizeA(g_Variables.m_FontSecundary->FontSize, FLT_MAX, 0, xorstr("Stop"));
		const ImVec2 IconTextSize = g_Variables.FontAwesomeSolid->CalcTextSizeA(g_Variables.FontAwesomeSolid->FontSize, FLT_MAX, 0, ICON_FA_CIRCLE_STOP);
		const ImVec2 pos = Window->DC.CursorPos;


		float Width = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x - (Window->ScrollbarY ? 12.f : 0.f);
		SizeArg.x = Width;
		ImVec2 size = ImGui::CalcItemSize(SizeArg, LabelSize.x + IconTextSize.x + style.FramePadding.x * 3.0f, LabelSize.y + style.FramePadding.y * 2.0f);

		const ImRect Rect(pos, ImVec2(pos.x + size.x, pos.y + size.y));
		ImGui::ItemSize(size, 0.f);

		if (!ImGui::ItemAdd(Rect, id))
			return false;

		bool Hovered, Held, Pressed = ImGui::ButtonBehavior(Rect, id, &Hovered, &Held, flags);

		struct ResourceBtn_t {
			float Alpha = 0.f;
			float SlideHeld = 0.f;
			float Size = 0.f;
			bool Completed = false;
		};

		static std::map<ImGuiID, ResourceBtn_t> anim;
		auto ResourceBtnAnim = anim.find(id);

		if (ResourceBtnAnim == anim.end()) {
			anim.insert({ id, ResourceBtn_t() });
			ResourceBtnAnim = anim.find(id);
		}

		auto TextPos = ImVec2(Rect.Min.x + 8, pos.y + (size.y / 2 - LabelSize.y / 2));

		ResourceBtnAnim->second.Alpha = ImLerp(ResourceBtnAnim->second.Alpha, ResourceState == 3 ? 1.f : 0.4f, g.IO.DeltaTime * 8);
		ResourceBtnAnim->second.SlideHeld = ImLerp(ResourceBtnAnim->second.SlideHeld, Held && ResourceState == 3 ? Rect.Max.x - pos.x : TextPos.x - pos.x + LabelSize.x + 8, g.IO.DeltaTime * 4);
		ResourceBtnAnim->second.Size = ImLerp(ResourceBtnAnim->second.Size, ResourceState == 3 && Held ? 4.f : 3.f, g.IO.DeltaTime * 4);

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * ResourceBtnAnim->second.Alpha);

		Window->DrawList->AddRectFilled(Rect.Min, Rect.Max, GetColorU32((ImVec4)ImColor(16, 16, 18)), 6);
		Window->DrawList->AddRectFilled(Rect.Min, ImVec2(pos.x + ResourceBtnAnim->second.SlideHeld, Rect.Max.y), GetColorU32((ImVec4)ImColor(24, 24, 26)), 6);
		Window->DrawList->AddText(g_Variables.m_FontNormal, g_Variables.m_FontNormal->FontSize, TextPos, GetColorU32(ImVec4(ImColor(g_Col.FeaturesText))), Label);

		ImVec2 CirclePos(Rect.Max.x - 16, pos.y + (size.y) / 2);

		if (ResourceState == 3) {
			Window->DrawList->AddCircleFilled(CirclePos, ResourceBtnAnim->second.Size, GetColorU32(ImVec4(ImColor(55, 237, 125, 180))), 99);
		}
		else {
			Window->DrawList->AddCircleFilled(CirclePos, ResourceBtnAnim->second.Size, GetColorU32(ImVec4(ImColor(237, 55, 55, 180))), 99);
		}

		ImGui::PopStyleVar();

		if (ResourceState == 3) {
			if (Held && ResourceBtnAnim->second.SlideHeld >= (Rect.Max.x - pos.x) - 1.f)
			{
				return true;
			}
			else {
				return false;
			}
		}
		else {
			return false;
		}
	}

	inline bool ListSelectableCustom(const char* Label, bool* Selected, int Type)
	{
		if (Type == 0) //Players
		{
			//if ( SelectableCustomResources( Label, *Selected, 3 ) ) {
				//*Selected = !*Selected;
				//return true;
			//}
		}
		else if (Type == 1) //Vehicles
		{
			//if ( SelectableCustomResources( Label, *Selected, 3 ) ) {
				//*Selected = !*Selected;
				//return true;
			//}
		}

		return false;
	}

}
