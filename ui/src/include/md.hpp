#ifndef PATRAM_UI_MD
#define PATRAM_UI_MD

#include <vector>
#include "ui.hpp"

enum class MarkdownFragmentType: uint8_t {
    Normal,
    Bold,
    Italic,
    Underline,
    Strikethrough,
    Code,
    Image,
    Link,
};

struct MarkdownImageOrLink {
    RopeViewData url;
    RopeViewData text;
};

struct MarkdownLink {
    RopeViewData url;
    RopeViewData text;
};

struct MarkdownFragment {
    MarkdownFragmentType fragType;

    union {
        RopeViewData text;
        MarkdownImageOrLink imgOrLink;
    };
};

struct Markdown {
    ContentBlockType cbType;
    uint32_t startIndex;
    uint16_t columns;
    uint32_t rows;
    std::vector<MarkdownFragment> fragments;
};

class MarkdownParser {
    Rope* rope;
    RopeCharIterator rci;
    std::vector<MarkdownFragment> fragments;

    char current;
    bool start;
    bool error;
    ContentBlockType cbType;
    uint32_t startIndex;

    char peek();
    char peekNext();
    bool match(char c);
    bool isAtEnd();
    void advance();

    void parseHeading();
    void parseTableRow();
    void parseBullets();
    void parseBold();
    void parseItalics();
    void parseStrikethrough();
    void parseCode();
    void parseImage();
    void parseLink();
    void parseNormalText();
public:
    MarkdownParser(Rope* rope);

    Markdown parse();
};

bool containsCodeBlockBound(Rope* rope);

#endif // PATRAM_UI_MD 

