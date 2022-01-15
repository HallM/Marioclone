#pragma once

#include <string>

struct WindowConfig {
	unsigned int width;
	unsigned int height;
	unsigned int framerate;
	unsigned int enable_fullscreen;
};

struct Config {
	WindowConfig window;
};

// extern Config DEFAULT_CONFIG = { WindowConfig{1024, 768, 60, 0} };

Config readConfig(std::string file_path);
