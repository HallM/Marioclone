#include "FstreamFileManager.h"

#include <fstream>

FstreamFileManager::FstreamFileManager() {}
FstreamFileManager::~FstreamFileManager() {}

std::string
FstreamFileManager::load_file(std::string path) {
    std::ifstream stream(path);
    std::string content(
        (std::istreambuf_iterator<char>(stream)),
        (std::istreambuf_iterator<char>())
    );
    return content;
}
