project(buildTool)


add_executable(buildTool "main.cpp" "../engine/engine/Utils/Json.cpp" "../engine/engine/Utils/FileIo.cpp" "../engine/engine/Utils/JsonFileIo.cpp" "../engine/dependencies/Json/JsonValue.cpp" "../engine/engine/Utils/Assertions.cpp" "../engine/dependencies/Json/JsonPrinter.cpp" "../engine/dependencies/Json/JsonParser.cpp" "../engine/engine/Utils/Put.cpp")

target_include_directories(buildTool PUBLIC "../engine" "../engine/engine/Utils" "../engine/dependencies/")

target_compile_features(buildTool PUBLIC cxx_std_23)
