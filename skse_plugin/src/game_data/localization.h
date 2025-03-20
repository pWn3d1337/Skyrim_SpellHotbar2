#pragma once
namespace SpellHotbar {
	extern std::unordered_map<std::string, std::string> translations;

	std::string translate(const std::string& key);
	const char* translate_c(const std::string& key);

	std::string translate_id(const std::string& key);
}