#include <iostream>
#include <filesystem>

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
		copy("./engine/dependencies/freetype.dll", gameOutputPath / "freetype.dll", copy_options::overwrite_existing);
		copy_file(executablePath, gameOutputPath / ("laser game" + executablePath.extension().string()), copy_options::overwrite_existing);

		for (const auto& dirEntry : std::filesystem::recursive_directory_iterator("./assets/levels")) {
			const auto& path = dirEntry.path();
			// TODO: Minify the json.
			//Json
		}
		
	}
	catch (const filesystem_error& e) {
		cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}
}