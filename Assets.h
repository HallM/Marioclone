#pragma once

#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

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

class AssetsDB {
public:
	AssetsDB();
	~AssetsDB();

	bool load(std::string path);

	template <typename T>
	std::optional<T> get(std::string name) {
		std::unordered_map<std::string, T>& values = std::get<std::unordered_map<std::string, T>>(_configs);
		auto it = values.find(name);
		if (it == values.end()) {
			return {};
		}
		return it->second;
	}
private:
	std::tuple<
		std::unordered_map<std::string, FontConfig>,
		std::unordered_map<std::string, SoundConfig>,
		std::unordered_map<std::string, TextureConfig>,
		std::unordered_map<std::string, SpriteSheetConfig>
	> _configs;
};
