add_executable(game)
target_sources(
  game
  PUBLIC main.cc
         julian_clock.h
         orbital_elements.h
         orbital_elements.cc
         lambert.h
         lambert.cc
         universe.h
         universe.cc
         universe_map.h
         universe_map.cc
         game_window.h
         game_window.cc
         game.h
         game.cc
         camera_controller.h
         camera_controller.cc
         date_gizmo.h
         date_gizmo.cc)
target_compile_features(game PUBLIC cxx_std_23)
target_compile_definitions(game PUBLIC GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(game PRIVATE nlohmann_json::nlohmann_json base)
set_target_properties(game PROPERTIES CXX_STANDARD_REQUIRED ON)
