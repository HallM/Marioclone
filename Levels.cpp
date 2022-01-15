#include "Levels.h"

#include <fstream>
#include <iostream>
#include <sstream>

Levels::Levels() {}
Levels::~Levels() {}

std::optional<TileMap> load_tilemap(std::string path) {
	std::ifstream file_stream(path);

	TileMap tmap;
	while (file_stream.good()) {
		std::string cmd;
		file_stream >> cmd;
		if (!file_stream.good()) {
			break;
		}

		if ("TileMap" == cmd) {
			file_stream >> tmap.tile_width;
			file_stream >> tmap.tile_height;
			file_stream >> tmap.width;
			file_stream >> tmap.height;
			int total_tiles = tmap.width * tmap.height;
			tmap.tilemap.reserve(total_tiles);
			for (int i = 0; i < total_tiles; i++) {
				unsigned int v;
				file_stream >> v;
				tmap.tilemap.push_back(v);
			}
		}
		else if ("TileInfo" == cmd) {
			TileInfo info;
			file_stream >> info.animation_name;
			file_stream >> info.aabb_width;
			file_stream >> info.aabb_height;
			file_stream >> info.z_index;
			tmap.tile_types.push_back(info);
		}
		else if ("Player" == cmd) {
			file_stream >> tmap.player.aabb_width;
			file_stream >> tmap.player.aabb_height;
			file_stream >> tmap.player.horizontal_speed;
			file_stream >> tmap.player.jump_speed;
			file_stream >> tmap.player.terminal_velocity;
			file_stream >> tmap.player.gravity;
			file_stream >> tmap.player.fire_animation;
		}
		else if ("Milestone" == cmd) {
			Milestone m;
			file_stream >> m.x;
			file_stream >> m.y;
			tmap.milestones.push_back(m);
		}
		else {
			std::cerr << "Unknown command: " << cmd << "\n";
			return {};
		}
	}
	return tmap;
}

bool Levels::Load(std::string config) {
	std::ifstream file_stream(config);

	std::vector<std::string> level_paths;

	while (file_stream.good()) {
		std::string cmd;
		file_stream >> cmd;
		if (!file_stream.good()) {
			break;
		}

		if ("Level" == cmd) {
			std::string path;
			file_stream >> path;
			level_paths.push_back(path);
		}
		else {
			std::cerr << "Unknown command: " << cmd << "\n";
			return false;
		}
	}

	for (unsigned int i = 0; i < level_paths.size(); i++) {
		std::stringstream ss;
		ss << "Level " << (i + 1);
		auto l = load_tilemap(level_paths[i]);
		if (!l.has_value()) {
			return false;
		}
		_levels[ss.str()] = l.value();
	}

	return true;
}

std::vector<std::string> Levels::GetLevelNames() {
	std::vector<std::string> l;
	for (auto it : _levels) {
		l.push_back(it.first);
	}
	return l;
}

std::optional<TileMap*> Levels::GetLevel(std::string name) {
	auto it = _levels.find(name);
	if (it == _levels.end()) {
		return {};
	}
	return &it->second;
}
