macro(AddTest)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES)

    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${TEST_NAME} ${TEST_SOURCES})
    target_compile_features(${TEST_NAME} PUBLIC cxx_std_23)
    target_link_libraries(${TEST_NAME} PRIVATE base)
endmacro()

AddTest(NAME test-glyph-generator SOURCES test_glyph_generator.cc)
AddTest(NAME test-sprite-sheet SOURCES test_sprite_sheet.cc)
AddTest(NAME test-gui-painter SOURCES test_gui_painter.cc)
AddTest(NAME test-gui-layout SOURCES test_gui_layout.cc)
AddTest(NAME test-gui-events SOURCES test_gui_events.cc)
AddTest(NAME test-gui-scroll-area SOURCES test_gui_scroll_area.cc)
AddTest(NAME test-gui-text-rotation SOURCES test_gui_text_rotation.cc)
