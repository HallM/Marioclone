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

	sf::RenderWindow window(sf::VideoMode(config.window.width, config.window.height, 32), "Not Mario I swear");
	//window.setFramerateLimit(config.window.framerate);

	Assets assets;
	assets.Load("assets.txt");
	AssetManager asset_manager(&assets);

	Levels levels;
	levels.Load("levels.txt");

	GameManager game(&window);

	MenuScene* menu = new MenuScene(&asset_manager, &levels);
	game.PushScene(menu);

	// This is the main game loop that runs until quit.
	game.RunLoop();

	return 0;
}
