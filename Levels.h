#pragma once

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct Player {
	float aabb_width = 0.0f;
	float aabb_height = 0.0f;
	float horizontal_speed = 0.0f;
	float jump_speed = 0.0f;
	float terminal_velocity = 0.0f;
	float gravity = 0.0f;
	std::string fire_animation = "";

	std::string start_animation = "MarSmStand";
	std::string stand_animation = "MarSmStand";
	std::string run_animation = "MarSmRun";
	std::string fall_animation = "MarSmJump";
};

struct TileInfo {
	std::string animation_name = "";
	// width of the collider or 0 if no collider
	float aabb_width = 0;
	float aabb_height = 0;
	int z_index = 0;
};

struct Milestone {
	int x;
	int y;
};

struct TileMap {
	Player player;
	std::vector<Milestone> milestones;
	// number of tiles wide/high
	int width = 0;
	int height = 0;
	// size of each tile.
	int tile_width = 0;
	int tile_height = 0;
	std::vector<TileInfo> tile_types = {};
	// 0 is no tile at all, any other value is (index+1) to the tile_types.
	std::vector<unsigned int> tilemap = {};
};

class Levels {
public:
	Levels();
	~Levels();

	bool Load(std::string config);

	std::optional<TileMap*> GetLevel(std::string name);

	std::vector<std::string> GetLevelNames();
private:
	std::unordered_map<std::string, TileMap> _levels;
};
