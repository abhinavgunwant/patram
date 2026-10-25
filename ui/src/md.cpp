#include "include/md.hpp"
#include <cctype>
#include <stack>
#include "include/ui.hpp"

bool containsCodeBlockBound(Rope* rope) {
    uint32_t consecutiveTickCount = 0;
    uint32_t maxConsecutiveTickCount = 0;

    RopeCharIterator rci(rope);

    char lastChar = '`';

    while (rci.hasNext()) {
        char ch = rci.next();

        if (ch == '`') {
            if (lastChar != '`') {
                consecutiveTickCount = 1;
            } else {
                ++consecutiveTickCount;
            }
        }

        if (consecutiveTickCount > maxConsecutiveTickCount) {
            maxConsecutiveTickCount = consecutiveTickCount;
        }

        lastChar = ch;
    }

    return maxConsecutiveTickCount == 3;
}

MarkdownParser::MarkdownParser(Rope* rope): rope(rope), rci(rope), error(false), start(true) {}

char MarkdownParser::peek() {
    if (start) {
        start = false;

        if (rci.hasNext()) {
            current = rci.next();
        } else {
            error = true;
        }
    }

    return current;
}

char MarkdownParser::peekNext() { return rci.peekNext(); }

bool MarkdownParser::match(char c) {
    if (current == c) {
        advance();
        return true;
    }

    return false;
}

bool MarkdownParser::isAtEnd() {
    return !rci.hasNext();
}

void MarkdownParser::advance() {
    if (rci.hasNext()) {
        current = rci.next();
        return;
    }

    error = true;
}

inline void MarkdownParser::pushNormalFragment(uint32_t index, uint32_t length) {
    fragments.push_back({ MarkdownFragmentType::Normal, { index, length } });
}

void MarkdownParser::parseHeading() {
    uint32_t hashes = 0;

    while (!isAtEnd()) {
        if (match('#')) {
            ++hashes;
        } else {
            break;
        }
    }

    if (match(' ')) {
        switch (hashes) {
            case 1: cbType = static_cast<uint8_t>(ContentBlockType::Heading1); break;
            case 2: cbType = static_cast<uint8_t>(ContentBlockType::Heading2); break;
            case 3: cbType = static_cast<uint8_t>(ContentBlockType::Heading3); break;
            case 4: cbType = static_cast<uint8_t>(ContentBlockType::Heading4); break;
            case 5: cbType = static_cast<uint8_t>(ContentBlockType::Heading5); break;
            case 6: cbType = static_cast<uint8_t>(ContentBlockType::Heading6); break;
        }

        if (hashes > 0 && hashes < 7) {
            startIndex = rci.index();
        }
    }

    parseNormalText();
}

void MarkdownParser::parseTableRow() {
    // TODO: Implement!
}

void MarkdownParser::parseBullets() {
    uint32_t index = rci.index();

    if ((match('-') || match('*')) && match(' ')) {
        startIndex = rci.index();
        cbType = static_cast<uint8_t>(ContentBlockType::Bullet);
    } else {
        rci.setIndex(index);
    }
}

void MarkdownParser::parseBold() {
    uint32_t initialIndex = rci.index();
    uint32_t index = initialIndex;
    std::stack<uint32_t> asteriskStack;

    if (initialIndex == 0) {
        startIndex = 0;
    }

    /*
    bool foundAsterisk = false;
    bool foundSpace = false;
    bool prevChar = false;
    bool prevCharAsterisk = false;
    uint32_t asteriskCount = 0;
    */

    if (!(match('*') && match('*'))) {
        rci.setIndex(initialIndex);
        return;
    }

    index += 2;

    if (startIndex == 0) {
        startIndex = 2;
    }

    uint32_t length = 0;

    while (!isAtEnd()) {
        if (peek() == '*') {
            if (peekNext() == '*') {
                advance();
                break;
            }

            fragments.push_back({
                MarkdownFragmentType::Bold, { index, length }
            });

            parseItalics();

            advance();

            index = rci.index();
            length = 0;
            continue;
        }

        advance();
        ++length;
    }

    /*
    while (!isAtEnd()) {
        if (match('*')) {
            if (!foundAsterisk) {
                index = rci.index();
                foundAsterisk = true;
            }

            if (prevChar) {
                prevCharAsterisk = true;
                prevChar = false;
            }

            ++asteriskCount;
        } else {
            if (!prevCharAsterisk && peek() == ' ') {
                foundSpace = true;
            } else if (foundAsterisk) {
                if (prevCharAsterisk) {
                    if (!asteriskStack.empty()) {
                        uint32_t startIndex = asteriskStack.top() + 1;
                        uint32_t lastIndex = asteriskStack.top();
                        uint32_t popped = 1;

                        asteriskStack.pop();

                        for (uint32_t i=0; i<asteriskCount && !asteriskStack.empty(); ++i) {
                            if (asteriskStack.top() - lastIndex == 1) {
                                lastIndex = asteriskStack.top();
                                asteriskStack.pop();
                                ++popped;
                            } else {
                                // TODO: create new fragment here...
                            }

                            if (popped == 2) {
                                // TODO: create a new bold fragment here...
                                popped = 0;
                            }
                        }

                        uint32_t length = index - asteriskCount - startIndex;
                    }

                    prevCharAsterisk = false;
                } else {
                    for (uint32_t i=0; i<asteriskCount; ++i) {
                        asteriskStack.push(i + index);
                    }
                }

                prevChar = false;
            } else {
                foundSpace = false;
                prevChar = true;
            }

            asteriskCount = 0;
            foundAsterisk = false;
            advance();
        }
    }

    // TODO: At the end of line... maybe use the lines below...
    for (uint32_t i=0; i<asteriskCount; ++i) {
        asteriskStack.push(i + index);
    }
    */

    if (length) {
        fragments.push_back({ MarkdownFragmentType::Bold, { index, length } });
    }
}

void MarkdownParser::parseItalics() {
    uint32_t initialIndex = rci.index();
    uint32_t index = initialIndex;
    std::stack<uint32_t> asteriskStack;

    if (initialIndex == 0) {
        startIndex = 0;
    }

    if (!match('*')) {
        rci.setIndex(initialIndex);
        return;
    }

    ++index;

    uint32_t length = 0;

    while (!isAtEnd()) {
        if (peek() == '*') {
            if (peekNext() != '*') {
                break;
            }

            fragments.push_back({
                MarkdownFragmentType::Italic, { index, length }
            });

            parseBold();

            advance();

            index = rci.index();
            length = 0;
            continue;
        }

        advance();
        ++length;
    }

    if (length) {
        fragments.push_back({
            MarkdownFragmentType::Italic, { index, length }
        });
    }
}

void MarkdownParser::parseStrikethrough() {
    while (!isAtEnd()) {
    }
}

void MarkdownParser::parseCode() {
    uint32_t beginIndex = rci.index();

    uint32_t index = beginIndex;

    while (match(' ') || match('\t')) {
        ++index;
    }

    if (!(match('`') && match('`') && match('`'))) {
        rci.setIndex(beginIndex);
        parseNormalText();
        return;
    }

    uint32_t length = 0;
    uint32_t tickCount = 0;
    bool foundTick = false;

    cbType = static_cast<uint8_t>(ContentBlockType::Code);
    startIndex = index + 3;

    while (!isAtEnd()) {
        if (match('`')) {
            ++tickCount;

            if (foundTick && tickCount == 3) {
                break;
            }

            foundTick = true;
        } else if (!foundTick) {
            ++length;
        } else if (foundTick) {
            foundTick = false;
            length += tickCount;
            tickCount = 0;
        }

        advance();
    }

    fragments.push_back({
        MarkdownFragmentType::Code,
        { index + 3, length },
    });
}

void MarkdownParser::parseImage() {
    uint32_t beginIndex = rci.index();

    if (!(match('!') && match('['))) {
        rci.setIndex(beginIndex);
        parseNormalText();
        return;
    }

    uint32_t altTextStartIndex = rci.index();
    uint32_t altTextLength = 0;

    bool foundText = false;

    while (!isAtEnd()) {
        if (match(']')) {
            foundText = true;
            break;
        }

        ++altTextLength;
        advance();
    }

    if (!foundText || !match('(')) {
        rci.setIndex(beginIndex);
        parseNormalText();
        return;
    }

    uint32_t urlStartIndex = rci.index();
    uint32_t urlLength = 0;
    bool foundUrl = false;

    while (!isAtEnd()) {
        if (match(')')) {
            foundUrl = true;
            break;
        }

        ++urlLength;
        advance();
    }

    if (!foundUrl) {
        rci.setIndex(beginIndex);
        parseNormalText();
        return;
    }

    cbType = static_cast<uint8_t>(ContentBlockType::Image);

    fragments.push_back({
        .fragType = MarkdownFragmentType::Image,
        .imgOrLink = {
            .url = { .startIndex = urlStartIndex, .length = urlLength },
            .text = { .startIndex = altTextStartIndex, .length = altTextLength }
        },
    });

    while (!isAtEnd() && match(' ')) { advance(); }

    if (!isAtEnd()) {
        parseNormalText();
    }
}

// TODO: parse [[Such Links]]
void MarkdownParser::parseLink() {
    uint32_t beginIndex = rci.index();

    advance();

    uint32_t textStartIndex = rci.index();
    uint32_t textLength = 0;

    bool foundText = false;

    while (!isAtEnd()) {
        if (match(']')) {
            foundText = true;
            break;
        }

        ++textLength;
        advance();
    }

    if (!foundText || !match('(')) {
        rci.setIndex(beginIndex);
        parseNormalText();
        return;
    }

    uint32_t urlStartIndex = rci.index();
    uint32_t urlLength = 0;
    bool foundUrl = false;

    while (!isAtEnd()) {
        if (match(')')) {
            foundUrl = true;
            break;
        }

        ++urlLength;
        advance();
    }

    if (!foundUrl) {
        rci.setIndex(beginIndex);
        parseNormalText();
        return;
    }

    cbType = static_cast<uint8_t>(ContentBlockType::Link);

    fragments.push_back({
        .fragType = MarkdownFragmentType::Link,
        .imgOrLink = {
            .url = { .startIndex = urlStartIndex, .length = urlLength },
            .text = { .startIndex = textStartIndex, .length = textLength }
        },
    });

    while (!isAtEnd() && match(' ')) { advance(); }

    if (!isAtEnd()) {
        parseNormalText();
    }
}

void MarkdownParser::parseNormalText() {
    uint32_t length = 0;
    uint32_t index = 0;

    if (rci.index() == 0) { startIndex = 0; } else {
        index = rci.index();
    }

    while (!isAtEnd()) {
        advance();
        ++length;
    }

    fragments.push_back({ MarkdownFragmentType::Normal, { index, length } });
}

Markdown MarkdownParser::parse() {
    cbType = static_cast<uint8_t>(ContentBlockType::Paragraph);
    bool _start = true;
    bool parsingNormalText = false;

    uint32_t hashes = 0;
    uint32_t dashes = 0;
    uint32_t indent = 0;
    uint32_t spaces = 0;
    uint32_t count = 0;

    uint32_t normalTextIndex = 0;
    uint32_t normalTextLength = 0;

    while (!isAtEnd()) {
        if (_start && isspace(peek())) {
            ++spaces;

            if (spaces % 4 == 0) {
                ++indent;
            }

            advance();
            continue;
        }

        switch (peek()) {
            case '#':
                if (parsingNormalText) {
                    pushNormalFragment(normalTextIndex, normalTextLength);
                    parsingNormalText = false;
                }

                if (_start) {
                    parseHeading();
                } else {
                    parseNormalText();
                }
                _start = false;
                break;

            case '-':
                if (parsingNormalText) {
                    pushNormalFragment(normalTextIndex, normalTextLength);
                    parsingNormalText = false;
                }

                if (_start && isspace(peekNext())) {
                    parseBullets();
                } else {
                    parseNormalText();
                }
                _start = false;
                break;

            case '*':
                if (parsingNormalText) {
                    pushNormalFragment(normalTextIndex, normalTextLength);
                    parsingNormalText = false;
                }

                if (peekNext() == '*') {
                    parseBold();
                } else if (_start && (peekNext() == ' ' || peekNext() == '\t')) {
                    parseBullets();
                } else {
                    parseItalics();
                }
                _start = false;
                break;

            case '~':
                if (parsingNormalText) {
                    pushNormalFragment(normalTextIndex, normalTextLength);
                    parsingNormalText = false;
                }

                parseStrikethrough();
                break;

            case '`':
                if (parsingNormalText) {
                    pushNormalFragment(normalTextIndex, normalTextLength);
                    parsingNormalText = false;
                }

                parseCode();
                break;

            case '!':
                if (parsingNormalText) {
                    pushNormalFragment(normalTextIndex, normalTextLength);
                    parsingNormalText = false;
                }

                if (peekNext() == '[') {
                    parseImage();
                } else {
                    parseNormalText();
                }
                break;

            case '[':
                if (parsingNormalText) {
                    pushNormalFragment(normalTextIndex, normalTextLength);
                    parsingNormalText = false;
                }

                parseLink();
            case '|':
                if (parsingNormalText) {
                    pushNormalFragment(normalTextIndex, normalTextLength);
                    parsingNormalText = false;
                }

                parseTableRow();
                break;
            default:
                if (!parsingNormalText) {
                    parsingNormalText = true;
                    normalTextIndex = rci.index();
                    normalTextLength = 1;
                } else {
                    ++normalTextLength;
                }

                _start = false;
        }

        count++;

        if (count > 100) {
            break;
        }

        advance();
    }

    if (parsingNormalText) {
        pushNormalFragment(normalTextIndex, normalTextLength);
    }

    return { cbType, startIndex, 0, 0, fragments };
}

