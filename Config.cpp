#include "Config.h"

#include <fstream>
#include <iostream>

Config readConfig(std::string file_path) {
	std::ifstream file_stream(file_path);

	Config c;

	while (file_stream.good()) {
		std::string cmd;
		file_stream >> cmd;
		if (!file_stream.good()) {
			break;
		}
		std::cout << "Do " << cmd << "\n";

		if ("Window" == cmd) {
			file_stream >> c.window.width;
			file_stream >> c.window.height;
			file_stream >> c.window.framerate;
			file_stream >> c.window.enable_fullscreen;
		}
		else {
			std::cerr << "Unknown command: " << cmd << "\n";
		}
	}
	return c;
}
