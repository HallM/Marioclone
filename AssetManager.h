#pragma once

#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

#include <SFML/Graphics.hpp>

#include "Assets.h"
#include "Components.h"

class AssetManager {
public:
	AssetManager(Assets* config);

	sf::Font* GetFont(std::string font_name);
	sf::Texture& GetTexture(std::string texture_name);

	std::tuple<SpriteAnimationConfig, sf::Texture&> GetAnimation(std::string animation);

private:
	Assets* _config;
	std::unordered_map<std::string, sf::Font> _fonts;
	std::unordered_map<std::string, sf::Texture> _textures;
};
