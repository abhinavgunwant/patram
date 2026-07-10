#include "include/md.hpp"

ContentBlockType getCBTypeAndStartIndexString(string inputText, uint32_t& startIndex) {
    uint8_t hashCount = 0;
    uint8_t dashCount = 0;
    uint8_t tickCount = 0;

    uint32_t i = 0;

    for (char ch: inputText) {
        if (ch == ' ' || isalnum(ch)) { ++i; break; }
        if (ch == '#') { ++hashCount; }
        if (ch == '-') { ++dashCount; }
        if (ch == '`') { ++tickCount; }

        ++i;
    }

    startIndex = i;

    if (hashCount && !dashCount && !tickCount) {
        switch (hashCount) {
            case 1: return ContentBlockType::Heading1;
            case 2: return ContentBlockType::Heading2;
            case 3: return ContentBlockType::Heading3;
            case 4: return ContentBlockType::Heading4;
            case 5: return ContentBlockType::Heading5;
            default: return ContentBlockType::Heading6;
        }
    }

    // if (dashCount == 1 && hashCount == 0 && tickCount == 0) {
    //     return ContentBlockType::Bullet;
    // }
    //
    // if (tickCount == 3 && hashCount == 0 && dashCount == 0) {
    //     return ContentBlockType::Code;
    // }

    return ContentBlockType::Paragraph;
}

ContentBlockType getCBTypeString(string inputText) {
    uint32_t _;
    return getCBTypeAndStartIndexString(inputText, _);
}

inline void parseNormalString(
    string::iterator& itr,
    string& inputStr,
    uint32_t currentIndex,
    vector<MarkdownFragment>& frags
) {
    uint32_t i = currentIndex;

    for (; itr!=inputStr.end(); ++itr, ++i) {
        if (*itr == '*' || *itr == '~' || *itr == '`') {
            frags.push_back({
                MarkdownFragmentType::Normal,
                inputStr.substr(currentIndex, i-currentIndex)
            });
            --itr;
            return;
        }
    }

    frags.push_back({
        MarkdownFragmentType::Normal,
        inputStr.substr(currentIndex, i-currentIndex)
    });
}

inline void parseBoldString(
    string::iterator& itr,
    string& inputStr,
    uint32_t currentIndex,
    vector<MarkdownFragment>& frags
) {
    uint32_t i = currentIndex;

    for (; itr!=inputStr.end(); ++itr, ++i) {
        if (
            *itr == '*'
            && (itr + 1) != inputStr.end()
            && *(itr + 1) == '*'
        ) {
            frags.push_back({
                MarkdownFragmentType::Bold,
                inputStr.substr(currentIndex, i-currentIndex)
            });
            itr += 2;
            return;
        }
    }

    frags.push_back({
        MarkdownFragmentType::Normal,
        inputStr.substr(currentIndex, i-currentIndex)
    });
}

inline void parseItalicString(
    string::iterator& itr,
    string& inputStr,
    uint32_t currentIndex,
    vector<MarkdownFragment>& frags
) {
    uint32_t i = currentIndex;

    for (; itr != inputStr.end(); ++itr, ++i) {
        if (*itr == '*') {
            frags.push_back({
                MarkdownFragmentType::Italic,
                inputStr.substr(currentIndex, i-currentIndex)
            });
            ++itr;
            return;
        }
    }

    frags.push_back({
        MarkdownFragmentType::Normal,
        inputStr.substr(currentIndex, i-currentIndex)
    });
}

inline void parseUnderlineString(
    string::iterator& itr,
    string& inputStr,
    uint32_t currentIndex,
    vector<MarkdownFragment>& frags
) {
    // TODO: implement!
}

inline void parseStrikethroughString(
    string::iterator& itr,
    string& inputStr,
    uint32_t currentIndex,
    vector<MarkdownFragment>& frags
) {
    uint32_t i = currentIndex;

    for (; itr != inputStr.end(); ++itr, ++i) {
        if (*itr == '~') {
            frags.push_back({
                MarkdownFragmentType::Strikethrough,
                inputStr.substr(currentIndex, i-currentIndex)
            });
            ++itr;
            return;
        }
    }

    frags.push_back({
        MarkdownFragmentType::Normal,
        inputStr.substr(currentIndex, i-currentIndex)
    });
}

inline void parseCodeString(
    string::iterator& itr,
    string& inputStr,
    uint32_t currentIndex,
    vector<MarkdownFragment>& frags
) {
    uint32_t i = currentIndex;

    for (uint32_t i=currentIndex; itr!=inputStr.end(); ++itr, ++i) {
        if (*itr == '`') {
            frags.push_back({
                MarkdownFragmentType::Strikethrough,
                inputStr.substr(currentIndex, i-currentIndex)
            });
            ++itr;
            return;
        }
    }

    frags.push_back({
        MarkdownFragmentType::Normal,
        inputStr.substr(currentIndex, i-currentIndex)
    });
}

Markdown parseContentString(string inputText) {
    uint32_t startIndex = 0;
    ContentBlockType cbType = getCBTypeAndStartIndexString(inputText, startIndex);
    Markdown md = { cbType, startIndex, 0, 0, vector<MarkdownFragment>() };

    uint32_t i = startIndex;

    for (
        string::iterator itr = inputText.begin() + startIndex;
        itr != inputText.end();
        ++itr, ++i
    ) {
        switch (*itr) {
            case '*': 
                if ((itr + 1) != inputText.end() && *(itr + 1) == '*') {
                    parseBoldString(itr, inputText, i, md.fragments);
                } else {
                    parseItalicString(itr, inputText, i, md.fragments);
                }
                break;

            case '~': parseStrikethroughString(itr, inputText, i, md.fragments); break;
            case '`': parseCodeString(itr, inputText, i, md.fragments); break;
            default: parseNormalString(itr, inputText, i, md.fragments);
        }

        if (itr >= inputText.end()) { break; }
    }

    return md;
}

ContentBlockType getCBTypeAndStartIndex(Rope* rope, uint32_t& startIndex) {
    uint8_t hashCount = 0;
    uint8_t dashCount = 0;
    uint8_t tickCount = 0;

    uint32_t i = 0;

    RopeCharIterator rci(rope);

    while (rci.hasNext()) {
        char ch = rci.next();

        if (ch == ' ' || isalnum(ch)) { break; }
        if (ch == '#') { ++hashCount; }
        if (ch == '-') { ++dashCount; }
        if (ch == '`') { ++tickCount; }

        ++i;
    }

    if (hashCount && !dashCount && !tickCount) {
        startIndex = i;

        switch (hashCount) {
            case 1: return ContentBlockType::Heading1;
            case 2: return ContentBlockType::Heading2;
            case 3: return ContentBlockType::Heading3;
            case 4: return ContentBlockType::Heading4;
            case 5: return ContentBlockType::Heading5;
            default: return ContentBlockType::Heading6;
        }
    }

    // if (dashCount == 1 && hashCount == 0 && tickCount == 0) {
    //     return ContentBlockType::Bullet;
    // }
    //
    // if (tickCount == 3 && hashCount == 0 && dashCount == 0) {
    //     return ContentBlockType::Code;
    // }

    return ContentBlockType::Paragraph;
}

ContentBlockType getCBType(Rope* rope) {
    uint32_t _;
    return getCBTypeAndStartIndex(rope, _);
}

inline void parseBoldText(
    RopeCharIterator& rci,
    vector<MarkdownFragment>& frags
) {
    uint32_t currentIndex = rci.index();
    uint32_t i = currentIndex;

    while (rci.hasNext()) {
        char ch = rci.next();
        char nextChar = rci.peekNext();

        if (ch == '*' && nextChar == '*') {
            frags.push_back({
                MarkdownFragmentType::Bold,
                rci.getRootRope()->substring(currentIndex, i-currentIndex)
            });
            rci.next();
            rci.next();
            return;
        }

        ++i;
    }

    frags.push_back({
        MarkdownFragmentType::Normal,
        rci.getRootRope()->substring(currentIndex, i-currentIndex)
    });
}

inline void parseItalicText(
    RopeCharIterator& rci,
    vector<MarkdownFragment>& frags
) {
    uint32_t currentIndex = rci.index();
    uint32_t i = currentIndex;

    while (rci.hasNext()) {
        char ch = rci.next();

        if (ch == '*') {
            frags.push_back({
                MarkdownFragmentType::Italic,
                rci.getRootRope()->substring(currentIndex, i-currentIndex)
            });
            rci.next();
            return;
        }
    }

    frags.push_back({
        MarkdownFragmentType::Normal,
        rci.getRootRope()->substring(currentIndex, i-currentIndex)
    });
}

inline void parseStrikethroughText(
    RopeCharIterator& rci,
    vector<MarkdownFragment>& frags
) {
    uint32_t currentIndex = rci.index();
    uint32_t i = currentIndex;

    while (rci.hasNext()) {
        char ch = rci.next();

        if (ch == '~') {
            frags.push_back({
                MarkdownFragmentType::Strikethrough,
                rci.getRootRope()->substring(currentIndex, i-currentIndex)
            });
            rci.next();
            return;
        }
    }

    frags.push_back({
        MarkdownFragmentType::Normal,
        rci.getRootRope()->substring(currentIndex, i-currentIndex)
    });
}

inline void parseCodeText(
    RopeCharIterator& rci,
    vector<MarkdownFragment>& frags
) {
    uint32_t currentIndex = rci.index();
    uint32_t i = currentIndex;

    while (rci.hasNext()) {
        char ch = rci.next();

        if (ch == '`') {
            frags.push_back({
                MarkdownFragmentType::Strikethrough,
                rci.getRootRope()->substring(currentIndex, i-currentIndex)
            });
            rci.next();
            return;
        }
    }

    frags.push_back({
        MarkdownFragmentType::Normal,
        rci.getRootRope()->substring(currentIndex, i-currentIndex)
    });
}

inline void parseNormalText(
    RopeCharIterator& rci,
    vector<MarkdownFragment>& frags
) {
    uint32_t currentIndex = rci.index();
    uint32_t i = currentIndex;

    while (rci.hasNext()) {
        char ch = rci.peekNext();

        if (ch == '*' || ch == '~' || ch == '`') {
            frags.push_back({
                MarkdownFragmentType::Normal,
                rci.getRootRope()->substring(currentIndex, i-currentIndex)
            });
            return;
        }

        rci.next();
        ++i;
    }

    if (i > currentIndex) {
        frags.push_back({
            MarkdownFragmentType::Normal,
            rci.getRootRope()->substring(currentIndex, i-currentIndex)
        });
    }
}

Markdown parseContent(Rope* rope) {
    uint32_t startIndex = 0;
    ContentBlockType cbType = getCBTypeAndStartIndex(rope, startIndex);
    Markdown md = { cbType, startIndex, 0, 0, vector<MarkdownFragment>() };

    RopeCharIterator rci(rope);
    rci.setIndex(startIndex);

    while (rci.hasNext()) {
        char ch = rci.next();

        switch (ch) {
            case '*': 
                if (rci.peekNext() == '*') {
                    parseBoldText(rci, md.fragments);
                } else {
                    parseItalicText(rci, md.fragments);
                }
                break;

            case '~': parseStrikethroughText(rci, md.fragments); break;
            case '`': parseCodeText(rci, md.fragments); break;
            default: parseNormalText(rci, md.fragments);
        }
    }

    return md;
}

