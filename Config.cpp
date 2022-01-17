#include "Config.h"

#include "toml.hpp"
#include <iostream>

std::variant<ConfigParseError, ConfigValueError, Config> readConfig(std::string file_path) {
	Config c = { 640, 480, 30, 0 };

	try {
		toml::table toml_config = toml::parse_file(file_path);

		auto window_config = toml_config["window"];

		auto w = window_config["width"].value<int>();
		if (!w || w.value() <= 0) {
			return ConfigValueError{ "window.width must be a positive integer" };
		}
		c.window.width = (unsigned int)w.value();

		auto h = window_config["height"].value<int>();
		if (!h || h.value() <= 0) {
			return ConfigValueError{ "window.height must be a positive integer" };
		}
		c.window.height = (unsigned int)h.value();

		auto maxfps = window_config["maxfps"].value<int>();
		if (!maxfps || maxfps.value() <= 0) {
			return ConfigValueError{ "window.maxfps must be a positive integer" };
		}
		c.window.framerate = (unsigned int)maxfps.value();

		auto windowed = window_config["windowed"].value<bool>();
		if (!windowed) {
			return ConfigValueError{ "window.height must be true or false" };
		}
		c.window.enable_fullscreen = windowed.value();
	}
	catch (const toml::parse_error& err) {
		return ConfigParseError{err.description()};
	}

	return c;
}
