#include <base/glyph_generator.h>
#include <base/arg_parser.h>

#include <stb_image_write.h>

#include <cstring>
#include <iostream>

void printUsage(const char *argv0)
{
    std::cout << "Usage: " << argv0 << " [OPTIONS] OUTFILE\n\n"
              << "Options:\n"
              << "  -f, --font-path=PATH       Font path\n"
              << "  -c, --codepoint=CODEPOINT  Codepoint\n"
              << "  -t, --text=TEXT            Text\n"
              << "  -s, --font-size=SIZE       Font pixel height\n"
              << "  -o, --outline-size=PIXELS  Outline size in pixels\n";
}

int main(int argc, const char *argv[])
{
    if (argc == 1)
    {
        printUsage(argv[0]);
        return 0;
    }

    char32_t codepoint{U'a'};
    std::u32string text;
    std::string fontPath{"OpenSans_Bold.ttf"};
    int fontSize{50};
    int outlineSize{0};
    std::string outFile;

    ArgParser parser;
    parser.addOption(fontPath, 'f', "font-path");
    parser.addOption(fontSize, 's', "font-size");
    parser.addOption(outlineSize, 'o', "outline-size");
    parser.addOption(codepoint, 'c', "codepoint");
    parser.addOption(text, 't', "text");

    const auto unused = parser.parse(std::span{argv + 1, argv + argc});
    if (unused.size() != 1)
    {
        printUsage(argv[0]);
        return 1;
    }
    outFile = unused.front();

    GlyphGenerator generator(Font(fontPath, fontSize, outlineSize));
    if (!generator.valid())
        return 1;

    Image<uint32_t> image;

    if (text.empty())
    {
        auto glyphImage = generator.generate(codepoint);
        image = std::move(glyphImage.image);
    }
    else
    {
        image = generator.generate(text);
    }

    const auto pixels = image.pixels();
    stbi_write_png(outFile.c_str(), image.width(), image.height(), 4, pixels.data(), image.width() * sizeof(uint32_t));
}
