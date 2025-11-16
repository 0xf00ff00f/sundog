add_executable(test-glyph-generator test_glyph_generator.cc)
target_compile_features(test-glyph-generator PUBLIC cxx_std_23)
target_link_libraries(test-glyph-generator PRIVATE base)

add_executable(test-sprite-sheet test_sprite_sheet.cc)
target_compile_features(test-sprite-sheet PUBLIC cxx_std_23)
target_link_libraries(test-sprite-sheet PRIVATE base)

add_executable(test-gui-layout test_gui_layout.cc)
target_compile_features(test-gui-layout PUBLIC cxx_std_23)
target_link_libraries(test-gui-layout PRIVATE base)

add_executable(test-gui-events test_gui_events.cc)
target_compile_features(test-gui-events PUBLIC cxx_std_23)
target_link_libraries(test-gui-events PRIVATE base)
