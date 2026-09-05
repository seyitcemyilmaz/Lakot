#ifndef LAKOT_UITHEME_H
#define LAKOT_UITHEME_H

#include <imgui.h>

namespace lakot::UiTheme
{

// A single palette every panel pulls from, instead of each one inventing
// its own colors - distilled from LoginPanel's original amber/dark look,
// which was the closest thing this project had to a design language.
constexpr ImVec4 kBackground(0.13f, 0.13f, 0.16f, 1.00f);
constexpr ImVec4 kPanelBg(0.17f, 0.17f, 0.20f, 1.00f);

constexpr ImVec4 kAccent(0.85f, 0.65f, 0.25f, 1.00f);        // gold
constexpr ImVec4 kAccentHovered(0.95f, 0.72f, 0.32f, 1.00f);
constexpr ImVec4 kAccentActive(0.72f, 0.54f, 0.18f, 1.00f);

constexpr ImVec4 kTextPrimary(0.92f, 0.92f, 0.94f, 1.00f);
constexpr ImVec4 kTextMuted(0.60f, 0.60f, 0.66f, 1.00f);

constexpr ImVec4 kInfo(0.55f, 0.75f, 0.95f, 1.00f); // status text, local chat lines
constexpr ImVec4 kWarn(0.85f, 0.80f, 0.55f, 1.00f); // other player's chat lines

constexpr ImVec4 kDanger(1.00f, 0.40f, 0.40f, 1.00f);    // errors, bright blink phase
constexpr ImVec4 kDangerDim(0.55f, 0.16f, 0.16f, 1.00f); // dim blink phase

// Applies the full ImGuiStyle (colors, rounding, padding/spacing) on top of
// ImGui::StyleColorsDark(). Call once from GuiLayer::initialize(), after
// ImGui::CreateContext() and before the font atlas is baked by the OpenGL
// backend.
inline void apply()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& tStyle = ImGui::GetStyle();
    ImVec4* tColors = tStyle.Colors;

    tColors[ImGuiCol_WindowBg] = kBackground;
    tColors[ImGuiCol_ChildBg] = kBackground;
    tColors[ImGuiCol_PopupBg] = kPanelBg;

    tColors[ImGuiCol_Text] = kTextPrimary;
    tColors[ImGuiCol_TextDisabled] = kTextMuted;

    tColors[ImGuiCol_Button] = kAccent;
    tColors[ImGuiCol_ButtonHovered] = kAccentHovered;
    tColors[ImGuiCol_ButtonActive] = kAccentActive;

    tColors[ImGuiCol_Header] = kAccent;
    tColors[ImGuiCol_HeaderHovered] = kAccentHovered;
    tColors[ImGuiCol_HeaderActive] = kAccentActive;

    tColors[ImGuiCol_CheckMark] = kAccent;
    tColors[ImGuiCol_SliderGrab] = kAccent;
    tColors[ImGuiCol_SliderGrabActive] = kAccentActive;

    tColors[ImGuiCol_FrameBg] = ImVec4(kPanelBg.x, kPanelBg.y, kPanelBg.z, 1.0f);
    tColors[ImGuiCol_FrameBgHovered] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.25f);
    tColors[ImGuiCol_FrameBgActive] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.40f);

    tColors[ImGuiCol_TitleBg] = kPanelBg;
    tColors[ImGuiCol_TitleBgActive] = ImVec4(kAccent.x * 0.35f, kAccent.y * 0.35f, kAccent.z * 0.35f, 1.0f);
    tColors[ImGuiCol_TitleBgCollapsed] = kPanelBg;

    tColors[ImGuiCol_Border] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.35f);
    tColors[ImGuiCol_Separator] = ImVec4(kAccent.x, kAccent.y, kAccent.z, 0.35f);

    tColors[ImGuiCol_Tab] = kPanelBg;
    tColors[ImGuiCol_TabHovered] = kAccentHovered;
    tColors[ImGuiCol_TabActive] = kAccentActive;

    tStyle.FrameRounding = 6.0f;
    tStyle.ChildRounding = 8.0f;
    tStyle.PopupRounding = 8.0f;
    tStyle.GrabRounding = 6.0f;
    tStyle.ScrollbarRounding = 8.0f;
    tStyle.TabRounding = 6.0f;

    tStyle.WindowPadding = ImVec2(12.0f, 12.0f);
    tStyle.FramePadding = ImVec2(8.0f, 5.0f);
    tStyle.ItemSpacing = ImVec2(8.0f, 6.0f);

    tStyle.WindowBorderSize = 1.0f;
    tStyle.FrameBorderSize = 1.0f;

    // Rounding a real OS-level borderless window's corners under
    // multi-viewport mode is a known ImGui/OS compositing glitch, not
    // something this reskin should reintroduce - kept exactly as GuiLayer
    // originally guarded it.
    if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        tStyle.WindowRounding = 0.0f;
        tColors[ImGuiCol_WindowBg].w = 1.0f;
    }
    else
    {
        tStyle.WindowRounding = 8.0f;
    }
}

}

#endif
