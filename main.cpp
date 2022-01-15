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
#include "Levels.h"
#include "GameManager.h"
#include "MenuScene.h"

int main(int argc, char* argv[]) {
	srand(0);

	Config config;
	config.window.width = 640;
	config.window.height = 480;
	try {
		config = readConfig("config.txt");
	}
	catch (...) {
		std::cerr << "Failed to read the config file\n";
		exit(-1);
	}

	std::unique_ptr<sf::RenderWindow> window = std::make_unique<sf::RenderWindow>(sf::VideoMode(config.window.width, config.window.height, 32), "Not Mario I swear");
	// window->setFramerateLimit(config.window.framerate);

	Assets assets;
	assets.Load("assets.txt");
	std::unique_ptr<AssetManager> asset_manager = std::make_unique<AssetManager>(&assets);

	Levels levels;
	levels.Load("levels.txt");

	GameManager game(std::move(asset_manager), std::move(window));
	game.PushScene(std::make_unique<MenuScene>(&levels));

	// This is the main game loop that runs until quit.
	game.RunLoop();

	return 0;
}
