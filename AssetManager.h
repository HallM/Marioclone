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
	// offset per frame for animations
	unsigned int animation_offset_x = 0;
	unsigned int animation_offset_y = 0;
};

struct SpriteSheetConfig {
	std::string name;
	std::string texture;
	int texture_id;
	std::vector<SpriteSheetEntryConfig> entries;
	std::unordered_map<std::string, int> entry_ids;
};

template<typename ConfigType>
class AssetDbContainer {
public:
	AssetDbContainer() {}
	~AssetDbContainer() {}

	int add(std::string name, ConfigType v) {
		int id = _configs.size();
		_ids[name] = id;
		_configs.push_back(v);
		return id;
	}

	std::optional<int> lookup_id(std::string v) {
		auto maybe = _ids.find(v);
		if (maybe != _ids.end()) {
			return maybe->second;
		}
		return {};
	}

	std::optional<ConfigType*> get(int id) {
		if (id >= 0 && id < (int)_configs.size()) {
			return &_configs.at(id);
		}
		return {};
	}

	std::unordered_map<std::string, int>& all() {
		return _ids;
	}
private:
	std::unordered_map<std::string, int> _ids;
	std::vector<ConfigType> _configs;
};

class AssetManager {
public:
	AssetManager();

	bool load_db(std::shared_ptr<IFileManager> file_manager, std::string path);

	std::unordered_map<std::string, int>& all_fonts();
	std::unordered_map<std::string, int>& all_textures();
	std::unordered_map<std::string, int>& all_spritesheets();
	std::unordered_map<std::string, int>& all_spritesheet_entries(int spritesheet_id);

	int lookup_font_id(std::string font_name);
	int lookup_texture_id(std::string texture_name);
	int lookup_spritesheet_id(std::string spritesheet);
	int lookup_spritesheet_entry_id(int spritesheet, std::string entry);

	sf::Font* get_font(int font_id);
	sf::Texture& get_texture(int texture_id);
	sf::Texture& get_spritesheet_texture(int spritesheet_id);
	SpriteSheetEntryConfig& get_spritesheet_entry(int spritesheet_id, int entry_id);

private:
	std::tuple<
		AssetDbContainer<FontConfig>,
		AssetDbContainer<SoundConfig>,
		AssetDbContainer<TextureConfig>,
		AssetDbContainer<SpriteSheetConfig>
	> _db;

	std::unordered_map<int, sf::Font> _fonts;
	std::unordered_map<int, sf::Texture> _textures;
};
