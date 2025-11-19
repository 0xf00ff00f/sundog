add_subdirectory(Catch2)

macro(AddTest)
    set(options)
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES)

    cmake_parse_arguments(TEST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${TEST_NAME} ${TEST_SOURCES})
    target_link_libraries(${TEST_NAME} PRIVATE base Catch2::Catch2WithMain)
endmacro()

AddTest(NAME test-gui SOURCES test_gui.cc)
AddTest(NAME test-rect SOURCES test_rect.cc)
