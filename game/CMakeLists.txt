add_subdirectory(3rdparty)
add_subdirectory(muslots)

add_library(infrastructure)
target_sources(
  infrastructure
  PRIVATE arg_parser.h
          arg_parser.cc
          asset_path.h
          asset_path.cc
          file.h
          file.cc
          utf8_util.h
          utf8_util.cc
          font.h
          font.cc
          font_info.h
          font_info.cc
          glyph_generator.h
          glyph_generator.cc
          rect.h
          sprite_sheet.h
          sprite_sheet.cc
          sprite_book.h
          sprite_book.cc
          sprite_texture_book.h
          sprite_texture_book.cc
          glhelpers.h
          glhelpers.cc
          mesh.h
          mesh.cc
          glyph_cache.h
          glyph_cache.cc
          tile_batcher.h
          tile_batcher.cc
          shader_manager.h
          shader_manager.cc
          painter.h
          painter.cc
          window_base.h
          window_base.cc)
target_compile_features(infrastructure PUBLIC cxx_std_23)
target_compile_definitions(infrastructure PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(infrastructure PUBLIC glfw glm glad muslots stb)
target_compile_definitions(
  infrastructure PUBLIC ASSETSDIR="${CMAKE_CURRENT_SOURCE_DIR}/../assets/")

add_executable(game)
target_sources(
  game
  PUBLIC main.cc
         orbital_elements.h
         orbital_elements.cc
         lambert.h
         lambert.cc
         universe.h
         universe.cc
         universe_map.h
         universe_map.cc)
target_compile_features(game PUBLIC cxx_std_23)
target_compile_definitions(game PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(game PRIVATE nlohmann_json::nlohmann_json infrastructure)
set_target_properties(game PROPERTIES CXX_STANDARD_REQUIRED ON)

add_executable(test-glyph-generator test_glyph_generator.cc)
target_compile_features(test-glyph-generator PUBLIC cxx_std_23)
target_link_libraries(test-glyph-generator PRIVATE infrastructure)

add_executable(test-sprite-sheet test_sprite_sheet.cc)
target_compile_features(test-sprite-sheet PUBLIC cxx_std_23)
target_link_libraries(test-sprite-sheet PRIVATE infrastructure)
