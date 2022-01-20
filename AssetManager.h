#pragma once

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

#include <SFML/Graphics.hpp>

#include "Assets.h"
#include "Components.h"

// TODO: freeing unneeded assets from former scenes.

class AssetManager {
public:
	AssetManager(std::unique_ptr<AssetsDB> db);

	sf::Font* get_font(std::string font_name);
	sf::Texture& get_texture(std::string texture_name);

	sf::Texture& get_spritesheet_texture(std::string spritesheet);

	SpriteSheetEntryConfig get_spritesheet_entry(std::string spritesheet, std::string entry);

private:
	std::unique_ptr<AssetsDB> _db;
	std::unordered_map<std::string, sf::Font> _fonts;
	std::unordered_map<std::string, sf::Texture> _textures;
};
