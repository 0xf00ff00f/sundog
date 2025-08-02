#include "glyph_image_generator.h"
#include "utf8_util.h"

#include <stb_image_write.h>

#include <cstring>
#include <iostream>

void printHelp()
{
    std::cout << "test-glyph-generator [OPTIONS] OUTFILE\n\n"
              << "Options:\n"
              << "  -h, --help                 This help message\n"
              << "  -f, --font-path=PATH       Font path\n"
              << "  -c, --codepoint=CODEPOINT  Codepoint\n"
              << "  -t, --text=TEXT            Text\n"
              << "  -s, --font-size=SIZE       Font pixel height\n"
              << "  -o, --outline-size=PIXELS  Outline size in pixels\n";
}

void printUsage(const char *argv0)
{
    std::cerr << "Usage: " << argv0 << " [OPTIONS] [OUTFILE]\n";
}

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        printUsage(argv[0]);
        return 0;
    }

    if (argc > 1 && (!std::strcmp(argv[1], "-h") || !std::strcmp(argv[1], "--help")))
    {
        printHelp();
        return 0;
    }

    char32_t codepoint{U'a'};
    std::u32string text;
    std::string fontPath{"OpenSans_Bold.ttf"};
    float fontSize{50.0f};
    int outlineSize{0};
    std::string outFile;

    for (int i = 1; i < argc; ++i)
    {
        if (!std::strcmp(argv[i], "-f") || !std::strcmp(argv[i], "--font-path"))
        {
            ++i;
            if (i < argc)
            {
                fontPath = argv[i];
            }
            else
            {
                printUsage(argv[0]);
                return 1;
            }
        }
        else if (!std::strncmp(argv[i], "--font-path=", 12))
        {
            fontPath = argv[i] + 12;
        }
        else if (!std::strcmp(argv[i], "-c") || !std::strcmp(argv[i], "--codepoint"))
        {
            ++i;
            if (i < argc)
            {
                codepoint = std::atoi(argv[i]);
            }
            else
            {
                printUsage(argv[0]);
                return 1;
            }
        }
        else if (!std::strncmp(argv[i], "--codepoint=", 12))
        {
            codepoint = std::atoi(argv[i] + 12);
        }
        else if (!std::strcmp(argv[i], "-t") || !std::strcmp(argv[i], "--text"))
        {
            ++i;
            if (i < argc)
            {
                text = decodeUtf8(argv[i]);
            }
            else
            {
                printUsage(argv[0]);
                return 1;
            }
        }
        else if (!std::strncmp(argv[i], "--text=", 7))
        {
            text = decodeUtf8(argv[i] + 7);
        }
        else if (!std::strcmp(argv[i], "-s") || !std::strcmp(argv[i], "--font-size"))
        {
            ++i;
            if (i < argc)
            {
                fontSize = std::atof(argv[i]);
            }
            else
            {
                printUsage(argv[0]);
                return 1;
            }
        }
        else if (!std::strncmp(argv[i], "--font-size=", 12))
        {
            fontSize = std::atof(argv[i] + 12);
        }
        else if (!std::strcmp(argv[i], "-o") || !std::strcmp(argv[i], "--outline-size"))
        {
            ++i;
            if (i < argc)
            {
                outlineSize = std::atoi(argv[i]);
            }
            else
            {
                printUsage(argv[0]);
                return 1;
            }
        }
        else if (!std::strncmp(argv[i], "--outline-size=", 15))
        {
            outlineSize = std::atoi(argv[i] + 15);
        }
        else if (outFile.empty())
        {
            outFile = argv[i];
        }
        else
        {
            printUsage(argv[0]);
            return 1;
        }
    }

    GlyphImageGenerator generator{fontPath, fontSize, outlineSize};
    if (!generator.isValid())
        return 1;

    Image<uint32_t> image;

    if (text.empty())
    {
        const auto glyphImage = generator.generate(codepoint);
        image = std::move(glyphImage.image);
    }
    else
    {
        image = generator.generate(text);
    }

    const auto pixels = image.pixels();
    stbi_write_png(outFile.c_str(), image.width(), image.height(), 4, pixels.data(), image.width() * sizeof(uint32_t));
}
