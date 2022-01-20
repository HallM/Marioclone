#include "Assets.h"

#include <fstream>
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

AssetsDB::AssetsDB() {}
AssetsDB::~AssetsDB() {}

bool AssetsDB::load(std::string path) {
	try {
		toml::table config = toml::parse_file(path);

		auto& fonts = std::get<std::unordered_map<std::string, FontConfig>>(_configs);
		if (auto arr = config["fonts"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_font(node);
				fonts[parsed.name] = parsed;
			}
		}

		auto& sounds = std::get<std::unordered_map<std::string, SoundConfig>>(_configs);
		if (auto arr = config["sounds"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_sound(node);
				sounds[parsed.name] = parsed;
			}
		}

		auto& textures = std::get<std::unordered_map<std::string, TextureConfig>>(_configs);
		if (auto arr = config["textures"].as_array()) {
			for (auto& element : *arr) {
				auto node = toml::node_view<toml::node>(element);
				auto parsed = parse_texture(node);
				textures[parsed.name] = parsed;
			}
		}

		auto& spritesheets = std::get<std::unordered_map<std::string, SpriteSheetConfig>>(_configs);
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
