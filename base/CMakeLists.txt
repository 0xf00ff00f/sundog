add_subdirectory(3rdparty)
add_subdirectory(muslots)

add_library(base)
target_sources(
  base
  PRIVATE arg_parser.h
          arg_parser.cc
          asset_path.h
          asset_path.cc
          file.h
          file.cc
          utf8_util.h
          utf8_util.cc
          image.h
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
          window_base.cc
          gui.h
          gui.cc
          system.h
          system.cc)
target_compile_features(base PUBLIC cxx_std_23)
target_compile_definitions(base PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_include_directories(base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(base PUBLIC glfw glm glad muslots stb)
target_compile_definitions(base
                           PUBLIC ASSETSDIR="${PROJECT_SOURCE_DIR}/assets/")
