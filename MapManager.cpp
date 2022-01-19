#include "MapManager.h"

#include <iostream>

#include "toml.hpp"

#include "Assets.h"
#include "Components.h"

void move_parallax_layer(Transform& t, const CTilemapParallaxLayer& pl, float camera_x, float camera_y) {
	t.position.x = camera_x - (camera_x * pl.parallax);
}

bool generate_components(const Tilemap& tmap, MattECS::EntityManager& em, AssetManager& am) {
	for (unsigned int i = 0; i < tmap.layers.size(); i++) {
		const auto& layer = tmap.layers[i];

		if (layer.tileset.texture.length() > 0) {
			auto mape = em.entity();
			sf::Texture& t = am.GetTexture(layer.tileset.texture);
			em.add<CTilemapRenderLayer>(mape, tmap, i, t);
			em.add<Transform>(mape, 0.0f, 0.0f);
			em.add<ZIndex>(mape, i);
			if (layer.parallax != 1.0f) {
				em.add<CTilemapParallaxLayer>(mape, layer.parallax);
			}
		}

		float half_w = (float)tmap.tile_width / 2.0f;
		float half_h = (float)tmap.tile_height / 2.0f;

		// add AABBs
		for (const auto& tile : layer.tiles) {
			if (tile.id <= 0) {
				continue;
			}
			const auto& tile_info = layer.tileset.tiles[tile.id - 1];
			if (tile_info.aabb.width > 0 && tile_info.aabb.height > 0) {
				AABB::Material m;
				if (tile_info.passage) {
					m = AABB::Material::Permeable;
				}
				else {
					m = AABB::Material::Solid;
				}
				auto e = em.entity();
				em.add<Transform>(e, (float)(tile.x * tmap.tile_width) + half_w, (float)(tile.y * tmap.tile_height) + half_h);
				em.add<AABB>(
					e,
					sf::Vector2f(tile_info.aabb.width, tile_info.aabb.height),
					m, tile_info.damage, tile_info.hardness, tile_info.piercing);
			}
		}
	}
	return true;
}

ElementAABB
parse_aabb(toml::node_view<toml::node> n) {
	return ElementAABB{
		n["x"].value_or<float>(0.0f),
		n["y"].value_or<float>(0.0f),
		n["width"].value_or<float>(0.0f),
		n["height"].value_or<float>(0.0f)
	};
}

PlayerConfig
parse_player(toml::node_view<toml::node> n) {
	return PlayerConfig{
		parse_aabb(n["aabb"]),
		n["run_speed"].value_or<float>(0.0f),
		n["jump_speed"].value_or<float>(0.0f),
		n["fall_speed"].value_or<float>(0.0f),
		n["layer"].value_or<int>(0)
	};
}

MilestoneConfig
parse_milestone(toml::node_view<toml::node> n) {
	return MilestoneConfig{
		n["x"].value_or<unsigned int>(0),
		n["y"].value_or<unsigned int>(0)
	};
}

TileConfig
parse_tile(toml::node_view<toml::node> n) {
	return TileConfig{
		n["id"].value_or<unsigned int>(0),
		n["x"].value_or<unsigned int>(0),
		n["y"].value_or<unsigned int>(0)
	};
}

std::vector<TileConfig>
parse_tiles(toml::node_view<toml::node> n, unsigned int width, unsigned int height) {
	std::vector<TileConfig> tiles;

	if (n.is_array_of_tables()) {
		if (auto arr = n.as_array()) {
			for (auto& v : *arr) {
				auto node = toml::node_view<toml::node>(v);
				tiles.push_back(parse_tile(node));
			}
		}
	}
	else {
		if (auto arr = n.as_array()) {
			unsigned int y = 0;
			for (auto& hv : *arr) {
				if (auto arr = hv.as_array()) {
					unsigned int x = 0;
					for (auto& v : *arr) {
						unsigned int id = v.value_or<unsigned int>(0);
						tiles.push_back(TileConfig{ id, x, y });
						x++;
					}
				}
				y++;
			}
		}
	}
	return tiles;
}

LayerConfig
parse_layer(toml::node_view<toml::node> n, unsigned int width, unsigned int height) {
	float parallax = n["parallax"].value_or<float>(1.0f);
	if (parallax <= 0) {
		parallax = 1.0;
	}

	TileSetConfig tsc;
	std::string tileset_path = n["tileset"].value_or<std::string>("");
	if (tileset_path.length() > 0) {
		tsc = LoadTileSet(tileset_path).value();
	}

	// for each halving of parallax, we do w - w/4
	// parallax = 1, w. parallax approaching 0, w/2
	unsigned int layer_width = width;
	unsigned int layer_height = height;

	bool repeat_x = n["repeat_x"].value_or<bool>(false);

	auto tiles = parse_tiles(n["tiles"], layer_width, layer_height);

	if (repeat_x && tiles.size() > 0) {
		unsigned int max_x = 0;
		for (auto& t : tiles) {
			if (t.x > max_x) { max_x = t.x; }
		}

		if (max_x < layer_width) {
			unsigned int multi = layer_width / max_x;

			std::vector current_tiles(tiles);

			for (unsigned int i = 1; i < multi; i++) {
				for (auto& t : current_tiles) {
					tiles.push_back(
						TileConfig{
							t.id,
							t.x + (max_x * i),
							t.y
						}
					);
				}
			}
		}
	}
	

	return LayerConfig{
		parallax,
		layer_width,
		layer_height,
		tsc,
		tiles
	};
}

Tilemap
parse_tilemap(toml::table config) {
	float gravity = config["gravity"].value_or<float>(0.0f);
	unsigned int width = config["width"].value_or<unsigned int>(0);
	unsigned int height = config["height"].value_or<unsigned int>(0);
	unsigned int tile_width = config["tile_width"].value_or<unsigned int>(0);
	unsigned int tile_height = config["tile_height"].value_or<unsigned int>(0);

	PlayerConfig player = parse_player(config["player"]);

	std::vector<MilestoneConfig> milestones;
	if (auto arr = config["milestones"].as_array()) {
		for (auto& element : *arr) {
			auto node = toml::node_view<toml::node>(element);
			milestones.push_back(parse_milestone(node));
		}
	}

	std::vector<LayerConfig> layers;
	if (auto arr = config["layers"].as_array()) {
		for (auto& element : *arr) {
			auto node = toml::node_view<toml::node>(element);
			layers.push_back(parse_layer(node, width, height));
		}
	}

	return Tilemap{
		gravity, width, height, tile_width, tile_height,
		player,
		milestones, layers
	};
}

MapManager::MapManager() {}

bool
MapManager::load(std::string config_path) {
	try {
		toml::table toml_config = toml::parse_file(config_path);

		auto levels = toml_config["levels"].as_array();
		if (!levels) {
			std::cerr << "levels should be a list of toml objects\n";
			return false;
		}
		size_t i = 0;
		for (auto& l : *levels) {
			auto t = l.as_table();
			if (!t) {
				std::cerr << "levels[" << i << "] should be a list of toml objects\n";
				return false;
			}

			auto name = (*t)["name"].value<std::string>();
			if (!name || name.value().length() <= 0) {
				std::cerr << "levels[" << i << "] name should be a string of some length\n";
				return false;
			}
			if (_levels.find(name.value()) != _levels.end()) {
				std::cerr << "levels[" << i << "] tried to redefine level named " << name.value() << "\n";
				return false;
			}

			auto path = (*t)["file"].value<std::string>();
			if (!path || path.value().length() <= 0) {
				std::cerr << "levels[" << i << "] path should be a string of some length\n";
				return false;
			}
			_levels[name.value()] = path.value();
			i++;
		}
	}
	catch (const toml::parse_error& err) {
		std::cerr << "Failed to parse config " << config_path << ":\n" << err << "\n";
		return false;
	}

	return true;
}

std::optional<Tilemap>
MapManager::get_level(std::string name) const {
	auto it = _levels.find(name);
	if (it == _levels.end()) {
		return {};
	}

	try {

		toml::table toml_config = toml::parse_file(it->second);
		return parse_tilemap(toml_config);
	}
	catch (const toml::parse_error& err) {
		std::cerr << "Failed to parse level " << name << ":\n" << err << "\n";
		return {};
	}
	return {};
}

std::vector<std::string>
MapManager::get_level_names() const {
	std::vector<std::string> l;
	for (auto it : _levels) {
		l.push_back(it.first);
	}
	return l;
}
