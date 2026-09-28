#include "include/md.hpp"
#include <cctype>
#include <stack>

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
            case 1: cbType = ContentBlockType::Heading1; break;
            case 2: cbType = ContentBlockType::Heading2; break;
            case 3: cbType = ContentBlockType::Heading3; break;
            case 4: cbType = ContentBlockType::Heading4; break;
            case 5: cbType = ContentBlockType::Heading5; break;
            case 6: cbType = ContentBlockType::Heading6; break;
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
        cbType = ContentBlockType::Bullet;
    } else {
        rci.setIndex(index);
    }

    parseNormalText();
}

void MarkdownParser::parseBold() {
    uint32_t initialIndex = rci.index();
    uint32_t index = initialIndex;
    std::stack<uint32_t> asteriskStack;

    bool foundAsterisk = false;
    bool foundSpace = false;
    bool prevChar = false;
    bool prevCharAsterisk = false;
    uint32_t asteriskCount = 0;

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
}

void MarkdownParser::parseItalics() {
    while (!isAtEnd()) {
    }
}

void MarkdownParser::parseStrikethrough() {
    while (!isAtEnd()) {
    }
}

void MarkdownParser::parseCode() {
    uint32_t indexBefore = rci.index();

    if (!match('`')) { return; }

    uint32_t index = rci.index();
    uint32_t length = 0;

    while (!isAtEnd()) {
        if (match('`')) {
            fragments.push_back({
                MarkdownFragmentType::Code,
                { index, length },
            });
            return;
        }

        advance();

        ++length;
    }

    fragments.push_back({
        MarkdownFragmentType::Code,
        { index, length },
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

    cbType = ContentBlockType::Image;

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

    cbType = ContentBlockType::Link;

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

    if (rci.index() == 0) { startIndex = 0; }

    while (!isAtEnd()) {
        advance();
        ++length;
    }

    fragments.push_back({ MarkdownFragmentType::Normal, { startIndex, length } });
}

Markdown MarkdownParser::parse() {
    cbType = ContentBlockType::Paragraph;
    bool _start = true;

    uint32_t hashes = 0;
    uint32_t dashes = 0;
    uint32_t indent = 0;
    uint32_t spaces = 0;

    uint32_t count = 0;

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
            cout << "\nscanning: " << peek();
            case '#':
                if (_start) {
                    parseHeading();
                } else {
                    parseNormalText();
                }
                _start = false;
                break;

            case '-':
                if (_start && isspace(peekNext())) {
                    parseBullets();
                } else {
                    parseNormalText();
                }
                _start = false;
                break;

            case '*':
                if (_start && peekNext() != '*') {
                    _start = false;
                    parseBullets();
                } else {
                    parseNormalText();
                }
                break;

            case '~': parseStrikethrough(); break;
            case '`': parseCode(); break;
            case '!':
                if (peekNext() == '[') {
                    parseImage();
                } else {
                    parseNormalText();
                }
                break;
            case '[':
                parseLink();
            case '|': parseTableRow(); break;
            default:
                _start = false;
                parseNormalText();
        }

        count++;

        if (count > 100) {
            break;
        }

        advance();
    }

    return { cbType, startIndex, 0, 0, fragments };
}

