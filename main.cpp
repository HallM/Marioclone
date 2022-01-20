#include <SFML/Graphics.hpp>
#include <cmath>
#include <cstdlib>

#include <iostream>
#include <functional>
#include <optional>
#include <sstream>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "Assets.h"
#include "Config.h"
#include "AssetManager.h"
#include "GameManager.h"
#include "MapManager.h"
#include "MenuScene.h"

int main(int argc, char* argv[]) {
	srand(0);

	Config config;
	auto maybe_config = readConfig("config.txt");
	if (std::holds_alternative<Config>(maybe_config)) {
		config = std::get<Config>(maybe_config);
	}
	else if (std::holds_alternative<ConfigParseError>(maybe_config)) {
		std::cerr << "Failed to parse config: " << std::get<ConfigParseError>(maybe_config).error << "\n";
		return -1;
	}
	else if (std::holds_alternative<ConfigValueError>(maybe_config)) {
		std::cerr << "Failed to parse config: " << std::get<ConfigValueError>(maybe_config).error << "\n";
		return -1;
	}
	else {
		std::cerr << "General error trying to process config\n";
		return -1;
	}

	std::unique_ptr<sf::RenderWindow> window = std::make_unique<sf::RenderWindow>(sf::VideoMode(config.window.width, config.window.height, 32), "Not Mario I swear");
	//window->setFramerateLimit(config.window.framerate);

	AssetsDB assets;
	assets.load("assets.txt");
	std::unique_ptr<AssetsDB> asset_db = std::make_unique<AssetsDB>(assets);
	std::unique_ptr<AssetManager> asset_manager = std::make_unique<AssetManager>(std::move(asset_db));

	//Levels levels;
	//if (!levels.Load("levels.txt")) {
	//	return -1;
	//}

	MapManager maps;
	if (!maps.load("levels.txt")) {
		return -1;
	}
	std::unique_ptr<MapManager> map_manager = std::make_unique<MapManager>(maps);

	GameManager game(std::move(asset_manager), std::move(map_manager), std::move(window));
	game.PushScene(std::make_unique<MenuScene>());

	// This is the main game loop that runs until quit.
	game.RunLoop();

	return 0;
}
