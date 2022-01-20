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

#include "AssetManager.h"
#include "Config.h"
#include "IFileManager.h"
#include "FstreamFileManager.h"
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

	std::shared_ptr<IFileManager> file_manager = std::make_shared<FstreamFileManager>();

	std::unique_ptr<AssetManager> asset_manager = std::make_unique<AssetManager>();
	if (!asset_manager->load_db(file_manager, "assets.txt")) {
		return -1;
	}

	std::unique_ptr<MapManager> map_manager = std::make_unique<MapManager>(file_manager);
	if (!map_manager->load("levels.txt")) {
		return -1;
	}

	GameManager game(file_manager, std::move(asset_manager), std::move(map_manager), std::move(window));
	game.PushScene(std::make_unique<MenuScene>());

	// This is the main game loop that runs until quit.
	game.RunLoop();

	return 0;
}
