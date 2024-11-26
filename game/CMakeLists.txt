add_executable(game "main.cpp" "MainLoop.cpp" "Editor.cpp" "EditorEntities.cpp" "GameRenderer.cpp" "Game.cpp"  "Level.cpp" "EditorActions.cpp" "Stereographic.cpp" "FileSelectWidget.cpp"  "EditorGridTool.cpp" "GameUpdate.cpp"  "Circle.cpp" "GameSerialization.cpp" "GrabTools.cpp" "LevelSelect.cpp" "Ui.cpp" "MainMenu.cpp"  "SettingsManager.cpp" "Animation.cpp" "GameUi.cpp" "Levels.cpp" "GameSave.cpp" "Serialization.cpp"  "GameAudio.cpp" "Constants.cpp")

target_link_libraries(game PUBLIC engine)

target_compile_features(game PUBLIC cxx_std_23)
set_target_properties(game PROPERTIES CXX_EXTENSIONS OFF)

target_include_directories(game PUBLIC "../" "../engine/dependencies/")

include("../engine/codeGenTool/targetAddGenerated.cmake")

target_link_libraries(game PUBLIC gfx2d)

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/../engine/dependencies/freetype.dll" 
	"${CMAKE_CURRENT_SOURCE_DIR}/../freetype.dll" 
	COPYONLY)

add_custom_command(TARGET game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
	"${CMAKE_BINARY_DIR}/engine/dependencies/openal-soft/OpenAL32.dll"
	"${CMAKE_CURRENT_SOURCE_DIR}/../OpenAL32.dll") 

if (WIN32)
	target_link_libraries(game PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../engine/dependencies/freetype.lib")
else()
	target_link_libraries(game PUBLIC freetype)
endif()

# enumerator 'identifier' in switch of enum 'enumeration' is not handled
target_compile_options(game PRIVATE /we4062)

targetAddGenerated(game ${CMAKE_CURRENT_SOURCE_DIR})

# If this is on, then the console logs won't show even when launched from console.
if (FINAL_RELEASE)
	if (WIN32)
		set_target_properties(game PROPERTIES WIN32_EXECUTABLE TRUE)
	endif()
endif()

if (FINAL_RELEASE)
	set(EMBED_TOOL "$<TARGET_FILE:embedTool>")
	add_custom_command(
		TARGET game PRE_BUILD
		COMMAND ${EMBED_TOOL}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		DEPENDS ${EMBED_TOOL}
	)
	set(BUILD_TOOL "$<TARGET_FILE:buildTool>")
	set(targetName "game")
	add_custom_command(
		TARGET ${targetName} POST_BUILD
		COMMAND ${BUILD_TOOL} $<TARGET_FILE:game>
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		DEPENDS ${BUILD_TOOL}
	)
endif()