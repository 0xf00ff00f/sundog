add_subdirectory(3rdparty)
add_subdirectory(muslots)

add_library(infrastructure)
target_sources(
  infrastructure PRIVATE file.h file.cc utf8_util.h utf8_util.cc
                         glyph_image_generator.h glyph_image_generator.cc)
target_compile_features(infrastructure PUBLIC cxx_std_23)
target_link_libraries(infrastructure PUBLIC muslots stb)

add_executable(game)
target_sources(game PUBLIC main.cc glhelpers.h glhelpers.cc mesh.h mesh.cc)
target_compile_features(game PUBLIC cxx_std_23)
target_compile_definitions(game PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_compile_definitions(
  game PUBLIC ASSETSDIR="${CMAKE_CURRENT_SOURCE_DIR}/../assets/")
target_link_libraries(game PRIVATE glfw glad glm nlohmann_json::nlohmann_json)
set_target_properties(game PROPERTIES CXX_STANDARD_REQUIRED ON)

add_executable(test-glyph-generator test_glyph_generator.cc)
target_compile_features(test-glyph-generator PUBLIC cxx_std_23)
target_link_libraries(test-glyph-generator PRIVATE infrastructure)
