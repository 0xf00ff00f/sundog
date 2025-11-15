add_subdirectory(Catch2)

add_executable(gui gui.cc)
target_link_libraries(gui PRIVATE base Catch2::Catch2WithMain)
