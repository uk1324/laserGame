#pragma once

#include <filesystem>

// For some reason functions like GetOpenFilename change the current / working directory.
extern std::filesystem::path executableWorkingDirectory;