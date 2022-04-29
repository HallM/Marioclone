#include "AssetManager.h"

#include <iostream>

#include "toml.hpp"

FontConfig
parse_font(toml::node_view<toml::node> n) {
	return FontConfig{
		n["name"].value_or<std::string>(""),
		n["path"].value_or<std::string>("")
	};
}

SoundConfig
parse_sound(toml::node_view<toml::node> n) {
	return SoundConfig{
		n["name"].value_or<std::string>(""),
		n["path"].value_or<std::string>("")
	};
}

TextureConfig
parse_texture(toml::node_view<toml::node> n) {
	return TextureConfig{
		n["name"].value_or<std::string>(""),
		n["path"].value_or<std::string>("")
	};
}

SpriteSheetEntryConfig
parse_spritesheet_entry(toml::node_view<toml::node> n) {
	return SpriteSheetEntryConfig{
		n["name"].value_or<std::string>(""),
		n["x"].value_or<unsigned int>(0),
		n["y"].value_or<unsigned int>(0),
		n["width"].value_or<unsigned int>(0),
		n["height"].value_or<unsigned int>(0),
		n["animation_frames"].value_or<unsigned int>(0),
		n["animation_rate"].value_or<unsigned int>(0),
		n["animation_offset_x"].value_or<unsigned int>(0),
		n["animation_offset_y"].value_or<unsigned int>(0),
	};
}

SpriteSheetConfig
parse_spritesheet(toml::node_view<toml::node> n, AssetDbContainer<TextureConfig>& textures) {
	std::string name = n["name"].value_or<std::string>("");
	std::string texture = n["texture"].value_or<std::string>("");

	SpriteSheetConfig c = { name, texture, textures.lookup_id(texture).value(), {} };

	if (auto arr = n["sprites"].as_array()) {
		for (auto& element : *arr) {
			auto node = toml::node_view<toml::node>(element);
			auto parsed = parse_spritesheet_entry(node);
			c.entry_ids[parsed.name] = c.entries.size();
			c.entries.push_back(parsed);
		}
	}
	return c;
}

AssetManager::AssetManager() {}


bool
AssetManager::load_db(std::shared_ptr<IFileManager> file_manager, std::string path) {
	try {
		toml::table config = toml::parse(file_manager->load_file(path));

		auto& fonts = std::get<AssetDbContainer<FontConfig>>(_db);
		if (auto arr = config["fonts"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_font(node);
				fonts.add(parsed.name, parsed);
			}
		}

		auto& sounds = std::get<AssetDbContainer<SoundConfig>>(_db);
		if (auto arr = config["sounds"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_sound(node);
				sounds.add(parsed.name, parsed);
			}
		}

		auto& textures = std::get<AssetDbContainer<TextureConfig>>(_db);
		if (auto arr = config["textures"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_texture(node);
				textures.add(parsed.name, parsed);
			}
		}

		auto& spritesheets = std::get<AssetDbContainer<SpriteSheetConfig>>(_db);
		if (auto arr = config["spritesheets"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_spritesheet(node, textures);
				spritesheets.add(parsed.name, parsed);
			}
		}
	}
	catch (const toml::parse_error& err) {
		std::cerr << "Failed to parse asset DB " << path << ":\n" << err << "\n";
		return false;
	}

	return true;
}

std::unordered_map<std::string, int>&
AssetManager::all_fonts() {
	return std::get<AssetDbContainer<FontConfig>>(_db).all();
}

std::unordered_map<std::string, int>&
AssetManager::all_textures() {
	return std::get<AssetDbContainer<TextureConfig>>(_db).all();
}

std::unordered_map<std::string, int>&
AssetManager::all_spritesheets() {
	return std::get<AssetDbContainer<SpriteSheetConfig>>(_db).all();
}

std::unordered_map<std::string, int>&
AssetManager::all_spritesheet_entries(int spritesheet_id) {
	auto sheet = std::get<AssetDbContainer<SpriteSheetConfig>>(_db).get(spritesheet_id).value();
	return sheet->entry_ids;
}

int
AssetManager::lookup_font_id(std::string font_name) {
	return std::get<AssetDbContainer<FontConfig>>(_db).lookup_id(font_name).value();
}

int
AssetManager::lookup_texture_id(std::string texture_name) {
	return std::get<AssetDbContainer<TextureConfig>>(_db).lookup_id(texture_name).value();
}

int
AssetManager::lookup_spritesheet_id(std::string spritesheet) {
	return std::get<AssetDbContainer<SpriteSheetConfig>>(_db).lookup_id(spritesheet).value();
}

int
AssetManager::lookup_spritesheet_entry_id(int spritesheet, std::string entry) {
	auto sheet = std::get<AssetDbContainer<SpriteSheetConfig>>(_db).get(spritesheet).value();
	return sheet->entry_ids.at(entry);
}

sf::Font*
AssetManager::get_font(int font_id) {
	auto maybe = _fonts.find(font_id);
	if (maybe != _fonts.end()) {
		return &maybe->second;
	}

	auto asset = std::get<AssetDbContainer<FontConfig>>(_db).get(font_id).value();

	sf::Font f;
	_fonts[font_id] = f;
	_fonts[font_id].loadFromFile(asset->path);
	return &_fonts[font_id];
}

sf::Texture&
AssetManager::get_texture(int texture_id) {
	auto maybe = _textures.find(texture_id);
	if (maybe != _textures.end()) {
		return maybe->second;
	}

	auto asset = std::get<AssetDbContainer<TextureConfig>>(_db).get(texture_id).value();

	_textures.emplace(std::piecewise_construct,
		std::forward_as_tuple(texture_id),
		std::forward_as_tuple());
	if (!_textures[texture_id].loadFromFile(asset->path)) {
		std::cerr << "Failed to load " << asset->path << "\n";
	}
	return _textures[texture_id];
}

sf::Texture&
AssetManager::get_spritesheet_texture(int spritesheet_id) {
	auto asset = std::get<AssetDbContainer<SpriteSheetConfig>>(_db).get(spritesheet_id).value();
	return get_texture(asset->texture_id);
}

SpriteSheetEntryConfig&
AssetManager::get_spritesheet_entry(int spritesheet_id, int entry_id) {
	auto asset = std::get<AssetDbContainer<SpriteSheetConfig>>(_db).get(spritesheet_id).value();
	return asset->entries[entry_id];
}
