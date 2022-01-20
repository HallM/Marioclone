#pragma once

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

#include <SFML/Graphics.hpp>

#include "IFileManager.h"

// TODO: freeing unneeded assets from former scenes.

struct FontConfig {
	std::string name;
	std::string path;
};

struct SoundConfig {
	std::string name;
	std::string path;
};

struct TextureConfig {
	std::string name;
	std::string path;
};

struct SpriteSheetEntryConfig {
	// this only needs to be unique within the spritesheet
	std::string name;
	unsigned int x;
	unsigned int y;
	unsigned int width;
	unsigned int height;
	// set to 0 or 1 for no animation
	unsigned int animation_frames = 0;
	// ticks per frame
	unsigned int animation_rate = 0;
};

struct SpriteSheetConfig {
	std::string name;
	std::string texture;
	std::unordered_map<std::string, SpriteSheetEntryConfig> entries;
};

class AssetManager {
public:
	AssetManager();

	bool load_db(std::shared_ptr<IFileManager> file_manager, std::string path);

	sf::Font* get_font(std::string font_name);
	sf::Texture& get_texture(std::string texture_name);

	sf::Texture& get_spritesheet_texture(std::string spritesheet);

	SpriteSheetEntryConfig get_spritesheet_entry(std::string spritesheet, std::string entry);

private:
	template <typename T>
	std::optional<T> _get_db(std::string name) {
		std::unordered_map<std::string, T>& values = std::get<std::unordered_map<std::string, T>>(_db);
		auto it = values.find(name);
		if (it == values.end()) {
			return {};
		}
		return it->second;
	}
	std::tuple<
		std::unordered_map<std::string, FontConfig>,
		std::unordered_map<std::string, SoundConfig>,
		std::unordered_map<std::string, TextureConfig>,
		std::unordered_map<std::string, SpriteSheetConfig>
	> _db;

	std::unordered_map<std::string, sf::Font> _fonts;
	std::unordered_map<std::string, sf::Texture> _textures;
};
