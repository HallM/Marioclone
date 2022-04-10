#include "MapManager.h"

#include <iostream>

#include "toml.hpp"

#include "Components.h"

void
move_parallax_layer(Transform& t, const CTilemapParallaxLayer& pl, float camera_x, float camera_y) {
	t.position.x = camera_x - (camera_x * pl.parallax);
}

//bool
//generate_components(const Tilemap& tmap, MattECS::EntityManager& em, AssetManager& am) {
//	for (unsigned int i = 0; i < tmap.layers.size(); i++) {
//		const auto& layer = tmap.layers[i];
//
//		if (layer.tileset.texture.length() > 0) {
//			auto mape = em.entity();
//			sf::Texture& t = am.get_texture(layer.tileset.texture);
//			em.add<CTilemapRenderLayer>(mape, tmap, i, t);
//			em.add<Transform>(mape, 0.0f, 0.0f);
//			em.add<ZIndex>(mape, i);
//			if (layer.parallax != 1.0f) {
//				em.add<CTilemapParallaxLayer>(mape, layer.parallax);
//			}
//		}
//
//		float half_w = (float)tmap.tile_width / 2.0f;
//		float half_h = (float)tmap.tile_height / 2.0f;
//
//		// add AABBs
//		for (const auto& tile : layer.tiles) {
//			if (tile.id <= 0) {
//				continue;
//			}
//			const auto& tile_info = layer.tileset.tiles[tile.id - 1];
//			if (tile_info.aabb.width > 0 && tile_info.aabb.height > 0) {
//				AABB::Material m;
//				if (tile_info.passage) {
//					m = AABB::Material::Permeable;
//				}
//				else {
//					m = AABB::Material::Solid;
//				}
//				auto e = em.entity();
//				em.add<Transform>(e, (float)(tile.x * tmap.tile_width) + half_w, (float)(tile.y * tmap.tile_height) + half_h);
//				em.add<AABB>(
//					e,
//					sf::Vector2f(tile_info.aabb.width, tile_info.aabb.height),
//					m, tile_info.damage, tile_info.hardness, tile_info.piercing);
//			}
//		}
//	}
//	return true;
//}

ElementAABB
MapManager::parse_aabb(toml::node_view<toml::node> n) {
	return ElementAABB{
		n["x"].value_or<float>(0.0f),
		n["y"].value_or<float>(0.0f),
		n["width"].value_or<float>(0.0f),
		n["height"].value_or<float>(0.0f)
	};
}

PlayerConfig
MapManager::parse_player(toml::node_view<toml::node> n) {
	return PlayerConfig{
		parse_aabb(n["aabb"]),
		n["run_speed"].value_or<float>(0.0f),
		n["jump_speed"].value_or<float>(0.0f),
		n["fall_speed"].value_or<float>(0.0f),
		n["layer"].value_or<int>(0)
	};
}

MilestoneConfig
MapManager::parse_milestone(toml::node_view<toml::node> n) {
	return MilestoneConfig{
		n["x"].value_or<unsigned int>(0),
		n["y"].value_or<unsigned int>(0)
	};
}

Script
MapManager::parse_script(toml::node_view<toml::node> n) {
	std::vector<std::string> events;
	if (auto arr = n["events"].as_array()) {
		for (auto& v : *arr) {
			events.push_back(v.value_or<std::string>(""));
		}
	}

	std::unordered_map<std::string, std::variant<float,int>> vars;
	if (auto varconfig = n["vars"].as_table()) {
		for (auto it = varconfig->begin(); it != varconfig->end(); it++) {
			std::string key(it->first.str());
			if (it->second.is_floating_point()) {
				vars[key] = it->second.value_or<float>(0.0f);
			}
			else {
				vars[key] = it->second.value_or<int>(0);
			}
		
		}
	}

	return Script{
		n["path"].value_or<std::string>(""),
		events,
		vars
	};
}

Entity
MapManager::parse_entity(toml::node_view<toml::node> n) {
	std::vector<Script> scripts;
	if (n["scripts"].is_array_of_tables()) {
		if (auto arr = n["scripts"].as_array()) {
			for (auto& v : *arr) {
				auto node = toml::node_view<toml::node>(v);
				scripts.push_back(parse_script(node));
			}
		}
	}

	return Entity{
		n["spritesheet"].value_or<std::string>(""),
		n["sprite"].value_or<std::string>(""),
		parse_aabb(n["aabb"]),
		n["x"].value_or<unsigned int>(0),
		n["y"].value_or<unsigned int>(0),
		scripts
	};
}

TileConfig
MapManager::parse_tile(toml::node_view<toml::node> n) {
	return TileConfig{
		n["id"].value_or<unsigned int>(0),
		n["x"].value_or<unsigned int>(0),
		n["y"].value_or<unsigned int>(0)
	};
}

std::vector<TileConfig>
MapManager::parse_tiles(toml::node_view<toml::node> n, unsigned int width, unsigned int height) {
	std::vector<TileConfig> tiles;
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
	return tiles;
}

std::vector<Entity>
MapManager::parse_entities(toml::node_view<toml::node> n) {
	std::vector<Entity> entities;

	if (auto arr = n.as_array()) {
		for (auto& e : *arr) {
			auto node = toml::node_view<toml::node>(e);
			entities.push_back(parse_entity(node));
		}
	}
	return entities;
}

TileSetTileConfig
MapManager::parse_tileset_tile(toml::node_view<toml::node> tile_config) {
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
	ElementAABB aabb = parse_aabb(tile_config["piercing"]);
	unsigned int animation_frames = tile_config["animation_frames"].value_or<unsigned int>(0);
	unsigned int animation_rate = tile_config["animation_rate"].value_or<unsigned int>(0);

	if (!passage || damage != 0 || destructable) {
		if (aabb.width == 0) {
			aabb.width = (float)width;
		}
		if (aabb.height == 0) {
			aabb.height = (float)height;
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

TileSetConfig
MapManager::parse_tileset(toml::table config) {
	std::string name = config["name"].value_or<std::string>("");
	std::string texture = config["texture"].value_or<std::string>("");

	TileSetConfig c = { name, texture, {} };

	if (auto arr = config["tiles"].as_array()) {
		for (auto& element : *arr) {
			auto node = toml::node_view<toml::node>(element);
			c.tiles.push_back(parse_tileset_tile(node));
		}
	}
	return c;
}

std::optional<TileSetConfig>
MapManager::load_tilesetfile(std::string path) {
	try {
		toml::table config = toml::parse(_file_manager->load_file(path));
		return parse_tileset(config);
	}
	catch (const toml::parse_error& err) {
		std::cerr << "Failed to parse tileset " << path << ":\n" << err << "\n";
		return {};
	}

	return {};
}

LayerConfig
MapManager::parse_layer(toml::node_view<toml::node> n, unsigned int width, unsigned int height) {
	float parallax = n["parallax"].value_or<float>(1.0f);
	if (parallax <= 0) {
		parallax = 1.0;
	}

	TileSetConfig tsc;
	std::string tileset_path = n["tileset"].value_or<std::string>("");
	if (tileset_path.length() > 0) {
		tsc = load_tilesetfile(tileset_path).value();
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
	
	auto entities = parse_entities(n["entities"]);

	return LayerConfig{
		parallax,
		layer_width,
		layer_height,
		tsc,
		tiles,
		entities
	};
}

Map
MapManager::parse_tilemap(toml::table config) {
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

	return Map{
		gravity, width, height, tile_width, tile_height,
		player,
		milestones, layers
	};
}

MapManager::MapManager(std::shared_ptr<IFileManager> file_manager) : _file_manager(file_manager) {}

bool
MapManager::load(std::string config_path) {
	try {
		toml::table toml_config = toml::parse(_file_manager->load_file(config_path));

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

std::optional<Map>
MapManager::get_level(std::string name) {
	auto it = _levels.find(name);
	if (it == _levels.end()) {
		return {};
	}

	try {
		toml::table toml_config = toml::parse(_file_manager->load_file(it->second));
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
