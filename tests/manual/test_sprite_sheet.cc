#include <base/sprite_book.h>
#include <base/glyph_generator.h>
#include <base/arg_parser.h>

#include <stb_image_write.h>

#include <iostream>

void printUsage(const char *argv0)
{
    std::cout << "Usage: " << argv0 << " [OPTIONS] OUTFILE\n\n"
              << "Options:\n"
              << "  -f, --font-name=PATH       Font name\n"
              << "  -s, --font-size=SIZE       Font pixel height\n"
              << "  -o, --outline-size=PIXELS  Outline size in pixels\n"
              << "  -w, --sheet-width=WIDTH    Sheet width\n"
              << "  -h, --sheet-height=HEIGHT  Sheet height\n";
}

int main(int argc, const char *argv[])
{
    std::string fontName{"OpenSans_Bold.ttf"};
    int fontSize{120};
    int outlineSize{0};
    std::string outFile;
    size_t sheetWidth{512};
    size_t sheetHeight{512};

    ArgParser parser;
    parser.addOption(fontName, 'f', "font-name");
    parser.addOption(fontSize, 's', "font-size");
    parser.addOption(outlineSize, 'o', "outline-size");
    parser.addOption(sheetWidth, 'w', "sheet-width");
    parser.addOption(sheetHeight, 'h', "sheet-height");

    const auto unused = parser.parse(std::span{argv + 1, argv + argc});
    if (unused.size() != 1)
    {
        printUsage(argv[0]);
        return 1;
    }
    outFile = unused.front();

    GlyphGenerator generator(Font(fontName, fontSize, outlineSize));
    if (!generator.valid())
        return 1;

    SpriteBook spriteBook{sheetWidth, sheetHeight};

    for (char32_t c = U'!'; c <= U'~'; ++c)
    {
        auto glyphImage = generator.generate(c);
        spriteBook.tryInsert(glyphImage.image);
    }

    const auto pages = spriteBook.pages();

    for (size_t index = 0; const auto *page : pages)
    {
        const auto pixels = page->pixels();
        stbi_write_png(std::format("{}-{}.png", outFile, index).c_str(), page->width(), page->height(), 4,
                       pixels.data(), page->width() * sizeof(uint32_t));
        ++index;
    }
}
