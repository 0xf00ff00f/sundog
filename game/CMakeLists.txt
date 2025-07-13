add_subdirectory(3rdparty)

set(GAME_SOURCES main.cc)

add_executable(game)
target_sources(game PRIVATE ${GAME_SOURCES})
target_compile_features(game PUBLIC cxx_std_23)
target_link_libraries(game PRIVATE glfw glad)
set_target_properties(game PROPERTIES CXX_STANDARD_REQUIRED ON)
