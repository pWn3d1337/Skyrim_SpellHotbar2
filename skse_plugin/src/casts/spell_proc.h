#pragma once
namespace SpellHotbar::casts {

	class SpellProc {
	public:
		SpellProc(float time);
		~SpellProc() = default;

		static void update_timer(float delta);
		static bool trigger_spellproc();

		static std::optional<float> has_matching_proc(RE::SpellItem* casted_spell);
		static float adjust_casttime(float original_ct, RE::SpellItem* casted_spell);
		static bool has_spell_proc(RE::SpellItem* spell);
		static float get_spell_proc_prog();
		static float get_spell_proc_timer();
		static float get_spell_proc_total();
		static void consume_proc();
	private:
		static std::unique_ptr<SpellProc> current_proc;

		void reproc(float timer);
		bool update(float delta);

		float m_timer;
		float m_total_time;
	};

}