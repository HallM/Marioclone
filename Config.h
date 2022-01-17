#pragma once

#include <string>
#include <variant>

struct WindowConfig {
	unsigned int width;
	unsigned int height;
	unsigned int framerate;
	bool enable_fullscreen;
};

struct Config {
	WindowConfig window;
};

struct ConfigParseError {
	std::string_view error;
};
struct ConfigValueError {
	std::string error;
};

std::variant<ConfigParseError, ConfigValueError, Config> readConfig(std::string file_path);
