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
parse_spritesheet(toml::node_view<toml::node> n) {
	std::string name = n["name"].value_or<std::string>("");
	std::string texture = n["texture"].value_or<std::string>("");

	SpriteSheetConfig c = { name, texture, {} };

	if (auto arr = n["sprites"].as_array()) {
		for (auto& element : *arr) {
			auto node = toml::node_view<toml::node>(element);
			auto parsed = parse_spritesheet_entry(node);
			c.entries[parsed.name] = parsed;
		}
	}
	return c;
}

AssetManager::AssetManager() {}


bool
AssetManager::load_db(std::shared_ptr<IFileManager> file_manager, std::string path) {
	try {
		toml::table config = toml::parse(file_manager->load_file(path));

		auto& fonts = std::get<std::unordered_map<std::string, FontConfig>>(_db);
		if (auto arr = config["fonts"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_font(node);
				fonts[parsed.name] = parsed;
			}
		}

		auto& sounds = std::get<std::unordered_map<std::string, SoundConfig>>(_db);
		if (auto arr = config["sounds"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_sound(node);
				sounds[parsed.name] = parsed;
			}
		}

		auto& textures = std::get<std::unordered_map<std::string, TextureConfig>>(_db);
		if (auto arr = config["textures"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_texture(node);
				textures[parsed.name] = parsed;
			}
		}

		auto& spritesheets = std::get<std::unordered_map<std::string, SpriteSheetConfig>>(_db);
		if (auto arr = config["spritesheets"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_spritesheet(node);
				spritesheets[parsed.name] = parsed;
			}
		}
	}
	catch (const toml::parse_error& err) {
		std::cerr << "Failed to parse asset DB " << path << ":\n" << err << "\n";
		return false;
	}

	return true;
}


sf::Font*
AssetManager::get_font(std::string font_name) {
	auto maybe = _fonts.find(font_name);
	if (maybe != _fonts.end()) {
		return &maybe->second;
	}

	auto asset = _get_db<FontConfig>(font_name);
	// I know... null ptr. terrible.
	if (!asset.has_value()) {
		return nullptr;
	}

	sf::Font f;
	_fonts[font_name] = f;
	_fonts[font_name].loadFromFile(asset->path);
	return &_fonts[font_name];
}

sf::Texture& AssetManager::get_texture(std::string texture_name) {
	auto maybe = _textures.find(texture_name);
	if (maybe != _textures.end()) {
		return maybe->second;
	}

	auto asset = _get_db<TextureConfig>(texture_name).value();

	_textures.emplace(std::piecewise_construct,
		std::forward_as_tuple(texture_name),
		std::forward_as_tuple());
	if (!_textures[texture_name].loadFromFile(asset.path)) {
		std::cerr << "Failed to load " << asset.path << "\n";
	}
	return _textures[texture_name];
}

sf::Texture&
AssetManager::get_spritesheet_texture(std::string spritesheet) {
	auto asset = _get_db<SpriteSheetConfig>(spritesheet).value();
	return get_texture(asset.texture);
}

SpriteSheetEntryConfig
AssetManager::get_spritesheet_entry(std::string spritesheet, std::string entry) {
	auto asset = _get_db<SpriteSheetConfig>(spritesheet).value();
	return asset.entries[entry];
}
