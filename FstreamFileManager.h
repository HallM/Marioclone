#pragma once

#include "IFileManager.h"

class FstreamFileManager : public IFileManager {
public:
	FstreamFileManager();
	~FstreamFileManager();

	std::string load_file(std::string path);
};
