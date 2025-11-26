add_executable(game)
target_sources(
  game
  PUBLIC camera_controller.cc
         camera_controller.h
         date_gizmo.cc
         date_gizmo.h
         game.cc
         game.h
         game_window.cc
         game_window.h
         julian_clock.h
         lambert.cc
         lambert.h
         main.cc
         orbital_elements.cc
         orbital_elements.h
         universe.cc
         universe.h
         universe_map.cc
         game_window.h
         game_window.cc
         game.h
         game.cc
         camera_controller.h
         camera_controller.cc
         date_gizmo.h
         date_gizmo.cc
         table_gizmo.h
         table_gizmo.cc
         market_snapshot_gizmo.h
         market_snapshot_gizmo.cc)
target_compile_features(game PUBLIC cxx_std_23)
target_compile_definitions(game PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(game PRIVATE nlohmann_json::nlohmann_json base)
set_target_properties(game PROPERTIES CXX_STANDARD_REQUIRED ON)
