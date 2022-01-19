#pragma once

#include <optional>
#include <string>
#include <unordered_map>

struct FontConfig {
	std::string name;
	std::string path;
};

struct TextureConfig {
	std::string name;
	std::string path;
};

struct SpriteConfig {
	std::string name;
	std::string texture;
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
};

struct AnimationConfig {
	std::string name;
	std::string texture;
	unsigned int total_frames;
	// Number of game ticks to show each animation frame
	unsigned int frame_ticks;
};

struct SpriteAnimationConfig {
	std::string name;
	std::string texture;
	unsigned int total_frames;
	// Number of game ticks to show each animation frame
	unsigned int frame_ticks;
	unsigned int x;
	unsigned int y;
	unsigned int w;
	unsigned int h;
};

struct SoundConfig {
	std::string name;
	std::string path;
};

struct AABBConfig {
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
};

struct TileSetTileConfig {
	std::string name;
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int passage = false;
	// use negative numbers to heal. this is damage per fixed-update!
	int damage = 0;
	bool destructable = false;
	int hardness = 0;
	int piercing = 0;
	AABBConfig aabb;
	// set to 0 or 1 for no animation
	unsigned int animation_frames = 0;
	// ticks per frame
	unsigned int animation_rate = 0;
};

struct TileSetConfig {
	std::string name;
	std::string texture;
	std::vector<TileSetTileConfig> tiles;
};

std::optional<TileSetConfig> LoadTileSet(std::string path);

class Assets {
public:
	Assets();
	~Assets();

	bool Load(std::string path);

	std::optional<FontConfig> GetFont(std::string name);
	std::optional<TextureConfig> GetTexture(std::string name);
	std::optional<SpriteConfig> GetSprite(std::string name);
	std::optional<AnimationConfig> GetAnimation(std::string name);
	std::optional<SpriteAnimationConfig> GetAnimatedSprite(std::string name);
	std::optional<SoundConfig> GetSound(std::string name);
	std::optional<TileSetConfig> GetTileSet(std::string name);

private:
	std::unordered_map<std::string, FontConfig> _fonts;
	std::unordered_map<std::string, TextureConfig> _textures;
	std::unordered_map<std::string, SpriteConfig> _sprites;
	std::unordered_map<std::string, AnimationConfig> _animations;
	std::unordered_map<std::string, SpriteAnimationConfig> _animated_sprites;
	std::unordered_map<std::string, SoundConfig> _sounds;
	std::unordered_map<std::string, TileSetConfig> _tilesets;
};
