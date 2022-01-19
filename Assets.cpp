#include "Assets.h"

#include <fstream>
#include <iostream>

#include "toml.hpp"

Assets::Assets() {}
Assets::~Assets() {}

AABBConfig ParseAABB(toml::node_view<toml::node> aabb_config) {
	float x = aabb_config["x"].value_or<float>(0);
	float y = aabb_config["y"].value_or<float>(0);
	float width = aabb_config["width"].value_or<float>(0);
	float height = aabb_config["height"].value_or<float>(0);
	return AABBConfig{ x, y, width, height };
}

TileSetTileConfig ParseTileSetTile(toml::node_view<toml::node> tile_config) {
	std::string name = tile_config["name"].value_or<std::string>("");
	unsigned int x = tile_config["x"].value_or<unsigned int>(0);
	unsigned int y = tile_config["y"].value_or<unsigned int>(0);
	unsigned int width = tile_config["width"].value_or<unsigned int>(0);
	unsigned int height = tile_config["height"].value_or<unsigned int>(0);
	unsigned int passage = tile_config["passage"].value_or<bool>(false);
	int damage = tile_config["damage"].value_or<int>(0);
	bool destructable = false;
	int hardness = tile_config["hardness"].value_or<int>(0);
	int piercing = tile_config["piercing"].value_or<int>(0);
	AABBConfig aabb = ParseAABB(tile_config["piercing"]);
	unsigned int animation_frames = tile_config["animation_frames"].value_or<unsigned int>(0);
	unsigned int animation_rate = tile_config["animation_rate"].value_or<unsigned int>(0);

	if (!passage || damage != 0 || destructable) {
		if (aabb.width == 0) {
			aabb.width = width;
		}
		if (aabb.height == 0) {
			aabb.height = height;
		}
	}

	return TileSetTileConfig{
		name,
		x, y, width, height,
		passage,
		damage, destructable, hardness, piercing,
		aabb,
		animation_frames, animation_rate
	};
}

TileSetConfig ParseTileSet(toml::table config) {
	std::string name = config["name"].value_or<std::string>("");
	std::string texture = config["texture"].value_or<std::string>("");

	TileSetConfig c = { name, texture, {} };

	if (auto arr = config["tiles"].as_array()) {
		for (auto& element: *arr) {
			auto node = toml::node_view<toml::node>(element);
			c.tiles.push_back(ParseTileSetTile(node));
		}
	}
	return c;
}

std::optional<TileSetConfig> LoadTileSet(std::string path) {
	try {
		toml::table config = toml::parse_file(path);
		return ParseTileSet(config);
	}
	catch (const toml::parse_error& err) {
		std::cerr << "Failed to parse tileset " << path << ":\n" << err << "\n";
		return {};
	}

	return {};
}

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
		else if ("Tileset" == cmd) {
			std::string tilesetpath;
			file_stream >> tilesetpath;
			auto maybe = LoadTileSet(tilesetpath);
			if (!maybe) {
				return false;
			}
			auto ts = maybe.value();
			_tilesets[ts.name] = ts;
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

std::optional<TileSetConfig> Assets::GetTileSet(std::string name) {
	auto it = _tilesets.find(name);
	if (it == _tilesets.end()) {
		return {};
	}
	return it->second;
}
