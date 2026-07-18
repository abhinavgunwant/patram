#ifndef PATRAM_UI_MD
#define PATRAM_UI_MD

#include <vector>
#include "ui.hpp"

using namespace std;

enum class MarkdownFragmentType: uint8_t {
    Normal,
    Bold,
    Italic,
    Underline,
    Strikethrough,
    Code,
};

struct MarkdownFragment {
    MarkdownFragmentType fragType;
    string text;
};

struct Markdown {
    ContentBlockType cbType;
    uint32_t startIndex;
    uint16_t columns;
    uint32_t rows;
    vector<MarkdownFragment> fragments;
};

ContentBlockType getCBType(Rope* rope);
Markdown parseContent(Rope* rope);
bool containsCodeBlockBound(Rope* rope);

#endif // PATRAM_UI_MD 

