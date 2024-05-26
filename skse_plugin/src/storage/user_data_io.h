#pragma once

namespace SpellHotbar::Storage::IO {

	constexpr int preset_save_version = 2;

	std::filesystem::path get_doc_dir();

	bool load_preset(const std::string& profile_name, bool include_user_dir);

	bool save_preset(const std::string& profile_name);

	std::vector<std::string> get_config_presets();

    std::vector<std::string> get_bar_presets();
}