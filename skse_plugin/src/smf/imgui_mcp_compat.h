#pragma once

#include <algorithm>
#include <cassert>

#include "../../third_party/skse-menu-framework/SKSEMenuFramework.h"

// SMF exposes Dear ImGui through a generated C-style MCP header. This narrow
// adapter preserves the source vocabulary used by SH2's renderer without
// linking another ImGui copy or creating another context.
using namespace ImGuiMCP;

#ifndef IM_ASSERT
#    define IM_ASSERT(_EXPR) assert(_EXPR)
#endif

struct SpellHotbarImColor {
    ImGuiMCP::ImVec4 Value{};

    constexpr SpellHotbarImColor() = default;

    constexpr SpellHotbarImColor(int red, int green, int blue, int alpha = 255) :
        Value{red / 255.0F, green / 255.0F, blue / 255.0F, alpha / 255.0F}
    {}

    constexpr SpellHotbarImColor(float red, float green, float blue, float alpha = 1.0F) :
        Value{red, green, blue, alpha}
    {}

    constexpr SpellHotbarImColor(ImGuiMCP::ImU32 rgba) :
        Value{
            static_cast<float>((rgba >> IM_COL32_R_SHIFT) & 0xFFU) / 255.0F,
            static_cast<float>((rgba >> IM_COL32_G_SHIFT) & 0xFFU) / 255.0F,
            static_cast<float>((rgba >> IM_COL32_B_SHIFT) & 0xFFU) / 255.0F,
            static_cast<float>((rgba >> IM_COL32_A_SHIFT) & 0xFFU) / 255.0F}
    {}

    constexpr SpellHotbarImColor(const ImGuiMCP::ImVec4& value) : Value(value) {}

    constexpr operator ImGuiMCP::ImVec4() const { return Value; }

    constexpr operator ImGuiMCP::ImU32() const
    {
        return (to_byte(Value.x) << IM_COL32_R_SHIFT) |
               (to_byte(Value.y) << IM_COL32_G_SHIFT) |
               (to_byte(Value.z) << IM_COL32_B_SHIFT) |
               (to_byte(Value.w) << IM_COL32_A_SHIFT);
    }

private:
    static constexpr ImGuiMCP::ImU32 to_byte(float value)
    {
        const auto clamped = std::clamp(value, 0.0F, 1.0F);
        return static_cast<ImGuiMCP::ImU32>(clamped * 255.0F + 0.5F);
    }
};

class SpellHotbarImGuiListClipper {
public:
    int DisplayStart{0};
    int DisplayEnd{0};

    SpellHotbarImGuiListClipper() : impl_(ImGuiMCP::ImGuiListClipperManager::Create()) {}

    ~SpellHotbarImGuiListClipper()
    {
        if (impl_) {
            ImGuiMCP::ImGuiListClipperManager::Destroy(impl_);
        }
    }

    SpellHotbarImGuiListClipper(const SpellHotbarImGuiListClipper&) = delete;
    SpellHotbarImGuiListClipper& operator=(const SpellHotbarImGuiListClipper&) = delete;

    void Begin(int items_count, float items_height = -1.0F)
    {
        ImGuiMCP::ImGuiListClipperManager::Begin(impl_, items_count, items_height);
        sync();
    }

    bool Step()
    {
        const bool result = ImGuiMCP::ImGuiListClipperManager::Step(impl_);
        sync();
        return result;
    }

    void End()
    {
        ImGuiMCP::ImGuiListClipperManager::End(impl_);
        sync();
    }

private:
    ImGuiMCP::ImGuiListClipper* impl_{nullptr};

    void sync()
    {
        if (impl_) {
            DisplayStart = impl_->DisplayStart;
            DisplayEnd = impl_->DisplayEnd;
        }
    }
};

class ImDrawListProxy {
public:
    void reset(ImGuiMCP::ImDrawList* draw_list) { draw_list_ = draw_list; }

    void AddImage(
        ImGuiMCP::ImTextureID texture,
        ImGuiMCP::ImVec2 min,
        ImGuiMCP::ImVec2 max,
        ImGuiMCP::ImVec2 uv_min = {0.0F, 0.0F},
        ImGuiMCP::ImVec2 uv_max = {1.0F, 1.0F},
        ImGuiMCP::ImU32 color = IM_COL32_WHITE)
    {
        ImGuiMCP::ImDrawListManager::AddImage(draw_list_, texture, min, max, uv_min, uv_max, color);
    }

    void AddText(
        ImGuiMCP::ImVec2 position,
        ImGuiMCP::ImU32 color,
        const char* text_begin,
        const char* text_end = nullptr)
    {
        ImGuiMCP::ImDrawListManager::AddText(draw_list_, position, color, text_begin, text_end);
    }

    void AddText(
        const ImGuiMCP::ImFont* font,
        float font_size,
        ImGuiMCP::ImVec2 position,
        ImGuiMCP::ImU32 color,
        const char* text_begin,
        const char* text_end = nullptr,
        float wrap_width = 0.0F,
        const ImGuiMCP::ImVec4* clip_rect = nullptr)
    {
        ImGuiMCP::ImDrawListManager::AddText(
            draw_list_, font, font_size, position, color, text_begin, text_end, wrap_width, clip_rect);
    }

private:
    ImGuiMCP::ImDrawList* draw_list_{nullptr};
};

namespace ImGui {
    using namespace ImGuiMCP;

    inline ImGuiMCP::ImGuiIO& GetIO()
    {
        return *ImGuiMCP::GetIO();
    }

    inline ImGuiMCP::ImGuiStyle& GetStyle()
    {
        return *ImGuiMCP::GetStyle();
    }

    inline ::ImDrawListProxy* GetWindowDrawList()
    {
        static thread_local ::ImDrawListProxy proxy;
        proxy.reset(ImGuiMCP::GetWindowDrawList());
        return &proxy;
    }

    inline ::ImDrawListProxy* GetForegroundDrawList()
    {
        static thread_local ::ImDrawListProxy proxy;
        proxy.reset(ImGuiMCP::GetForegroundDrawList());
        return &proxy;
    }

    inline void SetItemKeyOwner(ImGuiMCP::ImGuiKey key)
    {
        ImGuiMCP::SetItemKeyOwner(key, ImGuiMCP::ImGuiInputFlags_None);
    }
}

inline constexpr ImGuiMCP::ImGuiKey ImGuiKey_ModCtrl =
    static_cast<ImGuiMCP::ImGuiKey>(ImGuiMCP::ImGuiMod_Ctrl);
inline constexpr ImGuiMCP::ImGuiKey ImGuiKey_ModShift =
    static_cast<ImGuiMCP::ImGuiKey>(ImGuiMCP::ImGuiMod_Shift);
inline constexpr ImGuiMCP::ImGuiKey ImGuiKey_ModAlt =
    static_cast<ImGuiMCP::ImGuiKey>(ImGuiMCP::ImGuiMod_Alt);
