add_subdirectory(3rdparty)

add_executable(game)
target_sources(game PUBLIC main.cc)
target_sources(game PUBLIC FILE_SET CXX_MODULES FILES glhelpers.cc mesh.cc)
target_compile_features(game PUBLIC cxx_std_23)
target_compile_definitions(game PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(
  game PUBLIC ASSETSDIR="${CMAKE_CURRENT_SOURCE_DIR}/../assets/")
target_link_libraries(game PRIVATE glfw glad glm nlohmann_json::nlohmann_json)
set_target_properties(game PROPERTIES CXX_STANDARD_REQUIRED ON)
