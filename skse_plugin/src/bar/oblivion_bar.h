#pragma once
#include "hotbar.h"

namespace SpellHotbar::Bars {


	class OblivionBar {

    public:
        OblivionBar() = default;
        ~OblivionBar() = default;

        void serialize(SKSE::SerializationInterface* serializer, uint32_t key) const;
        void deserialize(SKSE::SerializationInterface* serializer, uint32_t type, uint32_t version, uint32_t length);

        void draw_in_hud(ImFont* font, float screensize_x, float screensize_y, int highlight_slot, float highlight_factor, key_modifier mod,
            bool hightlight_isred, float alpha, float shout_cd, float shout_cd_dur);

    private:
        SlottedSkill m_spell_slot;
        SlottedSkill m_potion_slot;
	};
}