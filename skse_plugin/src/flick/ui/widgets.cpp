#include "widgets.h"

#include "../flick_images.h"
#include "../../game_data/localization.h"

namespace SpellHotbar::FlickUi {
    namespace {
        ImVec2 plus(ImVec2 a, float dx, float dy) { return ImVec2(a.x + dx, a.y + dy); }

        unsigned pack(const ImVec4& c)
        {
            const auto byte = [](float v) {
                return static_cast<unsigned>(std::clamp(v, 0.0f, 1.0f) * 255.0f + 0.5f);
            };
            return rgba(static_cast<int>(byte(c.x)), static_cast<int>(byte(c.y)), static_cast<int>(byte(c.z)),
                        static_cast<int>(byte(c.w)));
        }
    }

    bool add_icon(const std::optional<UiBridge::IconRef>& a_ref, ImVec2 a_p0, ImVec2 a_p1, unsigned a_col)
    {
        if (!a_ref.has_value()) {
            return false;
        }
        FUCK::Image* tex = FlickImages::image_for(a_ref->path);
        if (tex == nullptr) {
            return false;
        }
        FUCK::AddImage(tex->GetID(), a_p0, a_p1, ImVec2(a_ref->u0, a_ref->v0), ImVec2(a_ref->u1, a_ref->v1),
                       FlickImages::unpack(a_col));
        return true;
    }

    bool icon_item(const std::optional<UiBridge::IconRef>& a_ref, float a_size, unsigned a_col)
    {
        const ImVec2 p = FUCK::GetCursorScreenPos();
        FUCK::Dummy(ImVec2(a_size, a_size));
        return add_icon(a_ref, p, plus(p, a_size, a_size), a_col);
    }

    bool draw_skill_at(RE::FormID a_form, ImVec2 a_pos, float a_size, unsigned a_col)
    {
        if (!add_icon(UiBridge::skill_icon(a_form), a_pos, plus(a_pos, a_size, a_size), a_col)) {
            return false;
        }
        draw_slot_overlay(a_pos, a_size);
        return true;
    }

    bool draw_skill_item(RE::FormID a_form, float a_size, unsigned a_col)
    {
        const ImVec2 p = FUCK::GetCursorScreenPos();
        FUCK::Dummy(ImVec2(a_size, a_size));
        return add_icon(UiBridge::skill_icon(a_form), p, plus(p, a_size, a_size), a_col);
    }

    bool draw_custom_icon_at(RE::FormID a_icon_form, const std::string& a_icon, ImVec2 a_pos, float a_size, unsigned a_col)
    {
        if (a_icon_form > 0) {
            return draw_skill_at(a_icon_form, a_pos, a_size, a_col);
        }
        if (!add_icon(UiBridge::named_icon(a_icon), a_pos, plus(a_pos, a_size, a_size), a_col)) {
            return false;
        }
        draw_slot_overlay(a_pos, a_size);
        return true;
    }

    void draw_default_icon_at(GameData::DefaultIconType a_type, ImVec2 a_pos, float a_size, unsigned a_col)
    {
        add_icon(UiBridge::default_icon(a_type), a_pos, plus(a_pos, a_size, a_size), a_col);
        draw_slot_overlay(a_pos, a_size);
    }

    void draw_extra_icon_at(const std::string& a_key, ImVec2 a_pos, float a_size, unsigned a_col)
    {
        add_icon(UiBridge::extra_icon(a_key), a_pos, plus(a_pos, a_size, a_size), a_col);
        draw_slot_overlay(a_pos, a_size);
    }

    void draw_slot_overlay(ImVec2 a_pos, float a_size, unsigned a_col)
    {
        add_icon(UiBridge::default_icon(GameData::DefaultIconType::BAR_OVERLAY), a_pos, plus(a_pos, a_size, a_size), a_col);
    }

    void draw_cd_overlay(ImVec2 a_pos, float a_size, float a_cd, unsigned a_col)
    {
        add_icon(UiBridge::cooldown_icon(a_cd), a_pos, plus(a_pos, a_size, a_size), a_col);
    }

    void draw_highlight_overlay(ImVec2 a_pos, float a_size, unsigned a_col)
    {
        add_icon(UiBridge::default_icon(GameData::DefaultIconType::BAR_HIGHLIGHT), a_pos, plus(a_pos, a_size, a_size), a_col);
    }

    void draw_icon_overlay(ImVec2 a_pos, float a_size, GameData::DefaultIconType a_type, unsigned a_col)
    {
        add_icon(UiBridge::default_icon(a_type), a_pos, plus(a_pos, a_size, a_size), a_col);
    }

    void draw_key_icons_at(ImVec2 a_pos, int a_tex_key, int a_tex_mod, float a_height, unsigned a_col)
    {
        float x = a_pos.x;
        if (const auto mod = UiBridge::key_icon(a_tex_mod); mod.has_value()) {
            const float w = a_height * mod->aspect;
            add_icon(mod, ImVec2(x, a_pos.y), ImVec2(x + w, a_pos.y + a_height), a_col);
            x += w;
        }
        if (const auto key = UiBridge::key_icon(a_tex_key); key.has_value()) {
            const float w = a_height * key->aspect;
            add_icon(key, ImVec2(x, a_pos.y), ImVec2(x + w, a_pos.y + a_height), a_col);
        }
    }

    float key_icons_length(int a_tex_key, int a_tex_mod)
    {
        float ret{ 0.0f };
        if (const auto key = UiBridge::key_icon(a_tex_key); key.has_value()) {
            ret += key->aspect;
        }
        if (const auto mod = UiBridge::key_icon(a_tex_mod); mod.has_value()) {
            ret += mod->aspect;
        }
        return ret;
    }

    void text_at(ImVec2 a_pos, unsigned a_col, const char* a_text, float a_font_size)
    {
        const ImVec2 save = FUCK::GetCursorScreenPos();
        FUCK::SetCursorScreenPos(a_pos);
        if (a_font_size > 0.0f) {
            FUCK::PushFont(FUCK::GetFont(FUCK::Font::kRegular), a_font_size);
        }
        FUCK::PushStyleColor(ImGuiCol_Text, FlickImages::unpack(a_col));
        FUCK::TextUnformatted(a_text);
        FUCK::PopStyleColor(1);
        if (a_font_size > 0.0f) {
            FUCK::PopFont();
        }
        FUCK::SetCursorScreenPos(save);
        // ImGui flags a SetCursorPos with no item after it ("code uses SetCursorPos() to extend
        // window/parent boundaries", seen in the Known Spells table when the overlay was a
        // cell's last call). A zero-size item clears that; SameLine(0, 0) then puts the cursor
        // back on `save` exactly, which the callers rely on.
        FUCK::Dummy(ImVec2(0.0f, 0.0f));
        FUCK::SameLine(0.0f, 0.0f);
    }

    bool radio_button(const char* a_label, int* a_v, int a_value)
    {
        const float h = FUCK::GetFrameHeight();
        const ImVec2 p = FUCK::GetCursorScreenPos();
        const ImVec2 label_sz = FUCK::CalcTextSize(a_label, nullptr, true);
        const ImVec2 spacing = FUCK::GetStyleVarVec(ImGuiStyleVar_ItemInnerSpacing);
        const bool has_label = label_sz.x > 0.0f;
        const float w = h + (has_label ? spacing.x + label_sz.x : 0.0f);

        FUCK::PushID(a_label);
        const bool pressed = FUCK::InvisibleButton("##radio", ImVec2(w, h));
        FUCK::PopID();
        const bool hovered = FUCK::IsItemHovered();
        if (pressed) {
            *a_v = a_value;
        }
        const bool on = *a_v == a_value;

        const ImVec2 center(p.x + h * 0.5f, p.y + h * 0.5f);
        const float radius = h * 0.5f - 1.0f;
        const ImVec4 bg = FUCK::GetStyleColorVec4(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        FUCK::DrawCircleFilled(center, radius, bg, 16);
        if (on) {
            FUCK::DrawCircleFilled(center, radius * 0.5f, FUCK::GetStyleColorVec4(ImGuiCol_CheckMark), 16);
        }
        FUCK::DrawCircle(center, radius, FUCK::GetStyleColorVec4(ImGuiCol_Border), 16, 1.0f);
        if (has_label) {
            text_at(ImVec2(p.x + h + spacing.x, p.y + (h - label_sz.y) * 0.5f),
                    pack(FUCK::GetStyleColorVec4(ImGuiCol_Text)), a_label);
        }
        return pressed;
    }

    void item_tooltip(const std::string& a_title, const std::string& a_desc)
    {
        if (!FUCK::IsItemHovered() || !FUCK::BeginTooltip()) {
            return;
        }
        push_large_font();
        FUCK::TextUnformatted(a_title.c_str());
        pop_font();
        if (!a_desc.empty()) {
            FUCK::PushTextWrapPos(FUCK::GetCursorPos().x + FUCK::GetDisplaySize().x * 0.35f);
            FUCK::TextUnformatted(a_desc.c_str());
            FUCK::PopTextWrapPos();
        }
        FUCK::EndTooltip();
    }

    void skill_tooltip(const RE::TESForm* a_form)
    {
        static RE::FormID last_form = 0;
        static std::string desc;
        if (a_form == nullptr || !FUCK::IsItemHovered()) {
            return;
        }
        if (last_form != a_form->GetFormID()) {
            desc = UiBridge::skill_tooltip(a_form);
            last_form = a_form->GetFormID();
        }
        item_tooltip(a_form->GetName(), desc);
    }

    bool combo(const char* a_label, int* a_index, const std::vector<std::string>& a_items)
    {
        return FUCK::Combo(a_label, a_index, a_items);
    }

    bool checkbox(const char* a_label, bool* a_v)
    {
        return FUCK::Checkbox(a_label, a_v, false, false);
    }

    RowClipper RowClipper::begin(int a_count, float a_row_h)
    {
        RowClipper c;
        c.count = a_count;
        c.row_h = std::max(a_row_h, 1.0f);
        const float scroll = FUCK::GetScrollY();
        const float visible = FUCK::GetWindowSize().y;
        c.first = std::clamp(static_cast<int>(std::floor(scroll / c.row_h)) - 1, 0, a_count);
        c.last = std::clamp(static_cast<int>(std::ceil((scroll + visible) / c.row_h)) + 1, c.first, a_count);
        if (c.first > 0) {
            FUCK::TableNextRow(0, c.row_h * static_cast<float>(c.first));
            FUCK::TableNextColumn();
        }
        return c;
    }

    void RowClipper::end() const
    {
        if (last < count) {
            FUCK::TableNextRow(0, row_h * static_cast<float>(count - last));
            FUCK::TableNextColumn();
        }
    }

    bool tab_button(const char* a_id, bool a_selected, float a_size, const char* a_tooltip,
                    const std::function<void(ImVec2, float)>& a_draw_icon)
    {
        const ImVec2 p = FUCK::GetCursorScreenPos();
        const bool pressed = FUCK::InvisibleButton(a_id, ImVec2(a_size, a_size));
        const bool hovered = FUCK::IsItemHovered();

        draw_default_icon_at(GameData::DefaultIconType::BAR_EMPTY, p, a_size);
        a_draw_icon(p, a_size);
        draw_slot_overlay(p, a_size);
        if (!a_selected) {
            draw_cd_overlay(p, a_size, 0.0f);
        }
        if (hovered) {
            draw_highlight_overlay(p, a_size, rgba(127, 127, 255));
            if (a_tooltip != nullptr && FUCK::BeginTooltip()) {
                push_large_font();
                FUCK::TextUnformatted(a_tooltip);
                pop_font();
                FUCK::EndTooltip();
            }
        }
        return pressed;
    }

    bool icon_picker(const char* a_id, RE::FormID& a_form, std::string& a_icon, unsigned a_tint)
    {
        bool picked{ false };
        const float button = scale(60.0f);
        const float inner_pad = std::max(button * 0.02f, 1.0f);
        const float icon = button - 2.0f * inner_pad;
        const ImVec2 spacing = FUCK::GetStyleVarVec(ImGuiStyleVar_ItemSpacing);
        const float visible_x2 = FUCK::GetWindowPos().x + FUCK::GetWindowSize().x;

        FUCK::PushID(a_id);
        const auto groups = UiBridge::editor_icon_groups();
        int group_id = 0;
        for (const auto& group : groups) {
            if (FUCK::CollapsingHeader(group.name.c_str())) {
                for (int n = 0; n < static_cast<int>(group.entries.size()); ++n) {
                    const auto& entry = group.entries[static_cast<std::size_t>(n)];
                    FUCK::PushID(group_id + n);
                    const ImVec2 bpos = FUCK::GetCursorScreenPos();
                    if (FUCK::InvisibleButton("##icon", ImVec2(button, button))) {
                        if (entry.form > 0) {
                            a_form = entry.form;
                            a_icon.clear();
                        } else if (!entry.icon.empty()) {
                            a_icon = entry.icon;
                            a_form = 0;
                        }
                        picked = true;
                    }
                    const bool hovered = FUCK::IsItemHovered();
                    const ImVec2 inner(bpos.x + inner_pad, bpos.y + inner_pad);
                    if (entry.form > 0) {
                        draw_skill_at(entry.form, inner, icon, a_tint);
                    } else if (!entry.icon.empty()) {
                        draw_custom_icon_at(0, entry.icon, inner, icon, a_tint);
                    }
                    if (hovered) {
                        draw_highlight_overlay(inner, icon, rgba(127, 127, 255));
                    }
                    const float next_x2 = FUCK::GetItemRectMax().x + spacing.x + button;
                    if (n + 1 < static_cast<int>(group.entries.size()) && next_x2 < visible_x2) {
                        FUCK::SameLine();
                    }
                    FUCK::PopID();
                }
            }
            group_id += 100000;
        }
        FUCK::PopID();
        return picked;
    }

    void push_large_font()
    {
        FUCK::PushFont(FUCK::GetFont(FUCK::Font::kLarge));
    }

    void pop_font()
    {
        FUCK::PopFont();
    }
}
