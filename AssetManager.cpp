#include "AssetManager.h"

#include <iostream>

AssetManager::AssetManager(std::unique_ptr<AssetsDB> db) : _db(std::move(db)) {}

sf::Font*
AssetManager::get_font(std::string font_name) {
	auto maybe = _fonts.find(font_name);
	if (maybe != _fonts.end()) {
		return &maybe->second;
	}

	auto asset = _db->get<FontConfig>(font_name);
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

	auto asset = _db->get<TextureConfig>(texture_name).value();

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
	auto asset = _db->get<SpriteSheetConfig>(spritesheet).value();
	return get_texture(asset.texture);
}

SpriteSheetEntryConfig
AssetManager::get_spritesheet_entry(std::string spritesheet, std::string entry) {
	auto asset = _db->get<SpriteSheetConfig>(spritesheet).value();
	return asset.entries[entry];
}
