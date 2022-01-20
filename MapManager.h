#pragma once

#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <SFML/Graphics.hpp>
#include "toml.hpp"

#include "AssetManager.h"
#include "Components.h"
#include "EntityManager.h"
#include "IFileManager.h"

struct ElementAABB {
	float x;
	float y;
	float width;
	float height;
};

struct PlayerConfig {
	ElementAABB aabb;
	float run_speed;
	float jump_speed;
	float fall_speed;
	int layer;
};

struct MilestoneConfig {
	unsigned int x;
	unsigned int y;
};

struct TileConfig {
	unsigned int id;
	unsigned int x;
	unsigned int y;
	// events
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
	ElementAABB aabb;
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

struct LayerConfig {
	float parallax;
	// the w/h are computed from w/h of the map * parallax and rounded down
	unsigned int width;
	unsigned int height;
	TileSetConfig tileset;
	std::vector<TileConfig> tiles;
};

struct Tilemap {
	float gravity;
	unsigned int width;
	unsigned int height;
	unsigned int tile_width;
	unsigned int tile_height;

	PlayerConfig player;
	std::vector<MilestoneConfig> milestones;
	std::vector<LayerConfig> layers;
	// scripts
	// global events
	// entities
};

struct AnimatedTile {
	unsigned int animation_frames;
	unsigned int animation_rate;
	unsigned int vert;

	float start_tx;
	float ty;
	float width;
	float height;
};

struct CTilemapParallaxLayer {
	float parallax;
	CTilemapParallaxLayer() : parallax(1.0f) {}
	CTilemapParallaxLayer(float p) : parallax(p) {}
};

void move_parallax_layer(Transform& t, const CTilemapParallaxLayer& pl, float camera_x, float camera_y);

// A component that can be used in the ECS
struct CTilemapRenderLayer {
public:
	sf::VertexArray verts;
	sf::Texture* texture;
	unsigned int animation_tick;
	unsigned int ani_multiple;
	std::vector<AnimatedTile> animated_tiles;

	CTilemapRenderLayer() : verts(), texture(nullptr), animation_tick(0), ani_multiple(1) {}
	CTilemapRenderLayer(const Tilemap& map, unsigned int l, sf::Texture& t) :
		verts(), texture(&t), animation_tick(0), ani_multiple(1)
	{
		const LayerConfig& layer = map.layers[l];
		std::unordered_map<unsigned int, bool> ani_frames;
		unsigned int tile_count = 0;
		for (unsigned int i = 0; i < layer.tiles.size(); i++) {
			if (layer.tiles[i].id > 0) {
				tile_count++;

				const TileSetTileConfig& tconf = layer.tileset.tiles[layer.tiles[i].id - 1];
				if (tconf.animation_frames > 1 && tconf.animation_rate > 0) {
					auto ticks = tconf.animation_frames * tconf.animation_rate;
					ani_frames[ticks] = true;
				}
			}
		}

		// find the multiplier for the animation ticks
		for (auto& it : ani_frames) {
			ani_multiple = ani_multiple * it.first;
		}

		verts.setPrimitiveType(sf::PrimitiveType::Quads);
		verts.resize(4 * tile_count);

		float twidth = (float)map.tile_width;
		float theight = (float)map.tile_height;

		unsigned int vert = 0;
		for (unsigned int i = 0; i < layer.tiles.size(); i++) {
			const TileConfig& t = layer.tiles[i];
			if (t.id <= 0) {
				continue;
			}
			float x = (float)t.x * twidth;
			float y = (float)t.y * theight;
			float r = x + twidth;
			float b = y + theight;

			sf::Vertex* quad = &verts[vert];

			quad[0].position = sf::Vector2f(x, y);
			quad[1].position = sf::Vector2f(r, y);
			quad[2].position = sf::Vector2f(r, b);
			quad[3].position = sf::Vector2f(x, b);

			const TileSetTileConfig& tconf = layer.tileset.tiles[t.id - 1];
			float tx = (float)tconf.x;
			float ty = (float)tconf.y;
			float tr = tx + (float)tconf.width;
			float tb = ty + (float)tconf.height;

			quad[0].texCoords = sf::Vector2f(tx, ty);
			quad[1].texCoords = sf::Vector2f(tr, ty);
			quad[2].texCoords = sf::Vector2f(tr, tb);
			quad[3].texCoords = sf::Vector2f(tx, tb);

			if (tconf.animation_frames > 1 && tconf.animation_rate > 0) {
				animated_tiles.push_back(
					AnimatedTile{
						tconf.animation_frames,
						tconf.animation_rate,
						vert,
						tx, ty,
						(float)tconf.width, (float)tconf.height
					}
				);
			}
			vert += 4;
		}
	}

	void animate() {
		animation_tick++;
		if (animation_tick >= ani_multiple) {
			animation_tick -= ani_multiple;
		}

		for (auto& t : animated_tiles) {
			auto frame = (float)((animation_tick / t.animation_rate) % t.animation_frames);

			sf::Vertex* quad = &verts[t.vert];

			float tx = t.start_tx + (frame * t.width);
			float ty = t.ty;
			float tr = tx + t.width;
			float tb = ty + t.height;

			quad[0].texCoords = sf::Vector2f(tx, ty);
			quad[1].texCoords = sf::Vector2f(tr, ty);
			quad[2].texCoords = sf::Vector2f(tr, tb);
			quad[3].texCoords = sf::Vector2f(tx, tb);
		}
	}

	void render(sf::RenderTarget& target, const sf::Transform& transform) const {
		sf::RenderStates states;
		states.texture = texture;
		states.transform = transform;
		target.draw(verts, states);
	}
};

bool generate_components(const Tilemap& tmap, MattECS::EntityManager& em, AssetManager& am);

class MapManager {
public:
	MapManager(std::shared_ptr<IFileManager> file_manager);

	bool load(std::string config_path);

	std::optional<Tilemap> get_level(std::string name);
	std::vector<std::string> get_level_names() const;
private:
	std::shared_ptr<IFileManager> _file_manager;
	// map of level name to path
	std::unordered_map<std::string, std::string> _levels;

	ElementAABB parse_aabb(toml::node_view<toml::node> n);
	PlayerConfig parse_player(toml::node_view<toml::node> n);
	MilestoneConfig parse_milestone(toml::node_view<toml::node> n);
	TileConfig parse_tile(toml::node_view<toml::node> n);
	std::vector<TileConfig> parse_tiles(toml::node_view<toml::node> n, unsigned int width, unsigned int height);
	TileSetTileConfig parse_tileset_tile(toml::node_view<toml::node> tile_config);
	TileSetConfig parse_tileset(toml::table config);
	std::optional<TileSetConfig> load_tilesetfile(std::string path);
	LayerConfig parse_layer(toml::node_view<toml::node> n, unsigned int width, unsigned int height);
	Tilemap parse_tilemap(toml::table config);
};
