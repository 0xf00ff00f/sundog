#include "sprite_sheet.h"
#include "glyph_image_generator.h"

#include <stb_image_write.h>

int main(int argc, char *argv[])
{
    GlyphImageGenerator generator{"OpenSans_Bold.ttf", 120};
    if (!generator.isValid())
        return 1;

    SpriteSheet spriteSheet{512, 512};

    for (char32_t c = U'!'; c <= U'~'; ++c)
    {
        auto glyphImage = generator.generate(c);
        spriteSheet.tryInsert(glyphImage.image);
    }

    const auto image = spriteSheet.image();

    const auto pixels = image.pixels();
    stbi_write_png("sheet.png", image.width(), image.height(), 4, pixels.data(), image.width() * sizeof(uint32_t));
}
