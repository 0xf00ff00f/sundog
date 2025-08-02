add_library(stb STATIC
    stb_image.c
    stb_image.h
    stb_image_write.c
    stb_image_write.h
    stb_truetype.c
    stb_truetype.h
)

target_compile_definitions(stb PUBLIC STBI_NO_STDIO)

target_include_directories(stb PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
