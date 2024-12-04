#include <iostream>
#include <filesystem>
#include <JsonFileIo.hpp>
#include <Json/JsonPrinter.hpp>
#include <fstream>

using namespace std;
using namespace filesystem;

const path outputPath = "./generated/build/";

int main(int argc, char** argv) {
	cout << "working directory = " << current_path().string() << '\n';

	if (argc != 2) {
		cerr << "wrong number of arguments " << argc << '\n';
		return EXIT_FAILURE;
	}
	const string_view executablePathString(argv[1]);
	cout << "executable path = " << executablePathString << '\n';

	const path executablePath(executablePathString);

	const auto gameOutputPath = outputPath / "laser game";

	auto copyDirRecursive = [&](const char* dir) {
		copy("./platformer/assets", gameOutputPath / "assets", copy_options::recursive | copy_options::overwrite_existing);
	};

	try {
		create_directories(gameOutputPath);
		create_directories(gameOutputPath / "assets/fonts");
		copy("./engine/assets/fonts/RobotoMono-Regular.ttf", gameOutputPath / "assets/fonts/RobotoMono-Regular.ttf", copy_options::overwrite_existing);
		copy("./assets/fonts/Tektur-Regular.ttf", gameOutputPath / "assets/fonts/Tektur-Regular.ttf", copy_options::overwrite_existing);
		copy("./engine/dependencies/freetype.dll", gameOutputPath / "freetype.dll", copy_options::overwrite_existing);
		copy_file("./OpenAL32.dll", gameOutputPath / "OpenAL32.dll", copy_options::overwrite_existing);
		copy_file(executablePath, gameOutputPath / ("laser game" + executablePath.extension().string()), copy_options::overwrite_existing);

		for (const auto& dirEntry : std::filesystem::directory_iterator("./assets/levels")) {
			if (dirEntry.is_directory()) {
				continue;
			}
			const auto& inPath = dirEntry.path();
			const auto json = tryLoadJsonFromFile(inPath.string());
			if (!json.has_value()) {
				cerr << "invalid json file: " << inPath << '\n';
				return EXIT_FAILURE;
			}
			const auto outDir = gameOutputPath / "assets/levels";
			create_directories(outDir);
			const auto outFile = outDir / inPath.filename();
			cout << "copying level " << inPath.filename() << " to " << outFile << '\n';
			std::ofstream file(outFile);
			// minifying json
			Json::print(file, *json);
		}
		copy("./assets/sound", gameOutputPath / "assets/sound", copy_options::recursive | copy_options::overwrite_existing);
		
	}
	catch (const filesystem_error& e) {
		cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}