#include "AssetManager.h"

AssetManager::AssetManager(Assets* config) {
	_config = config;
}

sf::Font* AssetManager::GetFont(std::string font_name) {
	auto maybe = _fonts.find(font_name);
	if (maybe != _fonts.end()) {
		return &maybe->second;
	}

	auto asset = _config->GetFont(font_name);
	// I know... null ptr. terrible.
	if (!asset.has_value()) {
		return nullptr;
	}

	sf::Font f;
	_fonts[font_name] = f;
	_fonts[font_name].loadFromFile(asset->path);
	return &_fonts[font_name];
}

sf::Texture& AssetManager::GetTexture(std::string texture_name) {
	auto maybe = _textures.find(texture_name);
	if (maybe != _textures.end()) {
		return maybe->second;
	}

	auto asset = _config->GetTexture(texture_name).value();

	_textures.emplace(std::piecewise_construct,
		std::forward_as_tuple(texture_name),
		std::forward_as_tuple());
	if (!_textures[texture_name].loadFromFile(asset.path)) {
		std::cerr << "Failed to load " << asset.path << "\n";
	}
	return _textures[texture_name];
}

// Creates an animation component and sets up the sprite to use it.
// Does not add it to the entity though.
std::tuple<SpriteAnimationConfig, sf::Texture&> AssetManager::GetAnimation(std::string animation) {
	auto sac = _config->GetAnimatedSprite(animation).value();
	sf::Texture& texture = GetTexture(sac.texture);
	return {sac, texture};
}
