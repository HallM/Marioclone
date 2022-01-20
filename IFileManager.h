#pragma once

#include <string>

// An interface to allow different file managers for platforms.
class IFileManager {
public:
	virtual ~IFileManager() = default;

	virtual std::string load_file(std::string path) = 0;
};
