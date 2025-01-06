#include "gui_tab_button.h"
#include "imgui.h"
#include "render_manager.h"
#include "../input/input.h"

namespace SpellHotbar::Rendering {


void GuiTabButton::draw(const char* id, int index, GameData::DefaultIconType icon_type, int icon_size, int & out_index, bool & changed, const char* tooltip_text)
{
    bool selected = index == out_index;
	ImVec2 b_size(static_cast<float>(icon_size), static_cast<float>(icon_size));
    ImVec2 p = ImGui::GetCursorScreenPos();
	if (ImGui::InvisibleButton(id, b_size)) {
        out_index = index;
        if (!selected) {
            changed = true;
        }
        RE::PlaySound(Input::sound_UIFavorite);
	}
    bool button_hovered = ImGui::IsItemHovered();

    RenderManager::draw_default_icon_in_editor(GameData::DefaultIconType::BAR_EMPTY, p, icon_size);
    RenderManager::draw_icon_overlay(p, icon_size, icon_type, IM_COL32_WHITE);

    RenderManager::draw_slot_overlay(p, icon_size, IM_COL32_WHITE);
    if (!selected) {
        RenderManager::draw_cd_overlay(p, icon_size, 0.0f, IM_COL32_WHITE);
    }
    if (button_hovered) {
        RenderManager::draw_highlight_overlay(p, icon_size, ImColor(127, 127, 255));

        float scalef = ImGui::GetIO().DisplaySize.y / 1080.0f;
        ImVec2 left_offset = ImVec2(32 * scalef, 0);
        if (tooltip_text != nullptr && ImGui::BeginItemTooltip())
        {
            ImGui::Dummy(left_offset); ImGui::SameLine();
            RenderManager::set_large_font();
            ImGui::TextUnformatted(tooltip_text);
            RenderManager::revert_font();
            ImGui::EndTooltip();
        }
    }
}
}