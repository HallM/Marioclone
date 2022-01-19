#include "Levels.h"

#include <iostream>

#include "toml.hpp"

Levels::Levels() {}
Levels::~Levels() {}

std::optional<std::shared_ptr<TileMap>> load_tilemap(std::string path) {
	std::shared_ptr<TileMap> tmap = std::make_shared<TileMap>();
	std::ifstream file_stream(path);

	while (file_stream.good()) {
		std::string cmd;
		file_stream >> cmd;
		if (!file_stream.good()) {
			break;
		}

		if ("TileMap" == cmd) {
			file_stream >> tmap->tile_width;
			file_stream >> tmap->tile_height;
			file_stream >> tmap->width;
			file_stream >> tmap->height;
			int total_tiles = tmap->width * tmap->height;
			tmap->tilemap.reserve(total_tiles);
			for (int i = 0; i < total_tiles; i++) {
				unsigned int v;
				file_stream >> v;
				tmap->tilemap.push_back(v);
			}
		}
		else if ("TileInfo" == cmd) {
			TileInfo info;
			file_stream >> info.animation_name;
			file_stream >> info.aabb_width;
			file_stream >> info.aabb_height;
			file_stream >> info.z_index;
			tmap->tile_types.push_back(info);
		}
		else if ("Player" == cmd) {
			file_stream >> tmap->player.aabb_width;
			file_stream >> tmap->player.aabb_height;
			file_stream >> tmap->player.horizontal_speed;
			file_stream >> tmap->player.jump_speed;
			file_stream >> tmap->player.terminal_velocity;
			file_stream >> tmap->player.gravity;
			file_stream >> tmap->player.fire_animation;
		}
		else if ("Milestone" == cmd) {
			Milestone m;
			file_stream >> m.x;
			file_stream >> m.y;
			tmap->milestones.push_back(m);
		}
		else {
			std::cerr << "Unknown command: " << cmd << "\n";
			return {};
		}
	}
	return tmap;
}

bool Levels::Load(std::string config) {
	try {
		toml::table toml_config = toml::parse_file(config);

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
			auto loaded = load_tilemap(path.value());
			if (!loaded) {
				return false;
			}
			_levels[name.value()] = loaded.value();

			i++;
		}

	}
	catch (const toml::parse_error& err) {
		std::cerr << "Failed to parse config " << config << ":\n" << err << "\n";
		return false;
	}

	return true;
}

std::vector<std::string> Levels::GetLevelNames() const {
	std::vector<std::string> l;
	for (auto it : _levels) {
		l.push_back(it.first);
	}
	return l;
}

std::optional<const std::shared_ptr<TileMap>> Levels::GetLevel(std::string name) const {
	auto it = _levels.find(name);
	if (it == _levels.end()) {
		return {};
	}
	return it->second;
}
