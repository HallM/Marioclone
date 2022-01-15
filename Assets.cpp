#include "Assets.h"

#include <fstream>
#include <iostream>

Assets::Assets() {}
Assets::~Assets() {}

bool Assets::Load(std::string path) {
	std::ifstream file_stream(path);

	while (file_stream.good()) {
		std::string cmd;
		file_stream >> cmd;
		if (!file_stream.good()) {
			break;
		}

		if ("Texture" == cmd) {
			TextureConfig t;
			file_stream >> t.name;
			file_stream >> t.path;
			_textures[t.name] = t;
		}
		else if ("Sprite" == cmd) {
			SpriteConfig s;
			file_stream >> s.name;
			file_stream >> s.texture;
			file_stream >> s.x;
			file_stream >> s.y;
			file_stream >> s.w;
			file_stream >> s.h;
			_sprites[s.name] = s;
		}
		else if ("Animation" == cmd) {
			AnimationConfig a;
			file_stream >> a.name;
			file_stream >> a.texture;
			file_stream >> a.total_frames;
			file_stream >> a.frame_ticks;
			_animations[a.name] = a;
		}
		else if ("AnimatedSprite" == cmd) {
			SpriteAnimationConfig s;
			file_stream >> s.name;
			file_stream >> s.texture;
			file_stream >> s.total_frames;
			file_stream >> s.frame_ticks;
			file_stream >> s.x;
			file_stream >> s.y;
			file_stream >> s.w;
			file_stream >> s.h;
			_animated_sprites[s.name] = s;
		}
		else if ("Font" == cmd) {
			FontConfig f;
			file_stream >> f.name;
			file_stream >> f.path;
			_fonts[f.name] = f;
		}
		else {
			std::cerr << "Unknown command: " << cmd << "\n";
			return false;
		}
	}
	return true;
}

std::optional<FontConfig> Assets::GetFont(std::string name) {
	auto it = _fonts.find(name);
	if (it == _fonts.end()) {
		return {};
	}
	return it->second;
}

std::optional<TextureConfig> Assets::GetTexture(std::string name) {
	auto it = _textures.find(name);
	if (it == _textures.end()) {
		return {};
	}
	return it->second;
}

std::optional<SpriteConfig> Assets::GetSprite(std::string name) {
	auto it = _sprites.find(name);
	if (it == _sprites.end()) {
		return {};
	}
	return it->second;
}

std::optional<AnimationConfig> Assets::GetAnimation(std::string name) {
	auto it = _animations.find(name);
	if (it == _animations.end()) {
		return {};
	}
	return it->second;
}

std::optional<SpriteAnimationConfig> Assets::GetAnimatedSprite(std::string name) {
	auto it = _animated_sprites.find(name);
	if (it == _animated_sprites.end()) {
		return {};
	}
	return it->second;
}

std::optional<SoundConfig> Assets::GetSound(std::string name) {
	auto it = _sounds.find(name);
	if (it == _sounds.end()) {
		return {};
	}
	return it->second;
}
