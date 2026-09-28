#include <iostream>
#include <thread>

#include "test.hpp"
#include "include/md.hpp"
#include "include/rope.hpp"

using namespace std;

SETUP_TEST;

TEST_START(cbType, "Content Block Type Test")
    Rope* testHeading1 = new Rope("# Test heading 1");
    Rope* testHeading2 = new Rope("## Test heading 2");
    Rope* testHeading3 = new Rope("### Test heading 3");
    Rope* testHeading4 = new Rope("#### Test heading 4");
    Rope* testHeading5 = new Rope("##### Test heading 5");
    Rope* testHeading6 = new Rope("###### Test heading 6");
    // string testCodeBlock = "```python";
    // string testBullet = "- This is a bullet list";
    // string testNumbering = "1. This is a numbered list";

    MarkdownParser parser1(testHeading1);
    MarkdownParser parser2(testHeading2);
    MarkdownParser parser3(testHeading3);
    MarkdownParser parser4(testHeading4);
    MarkdownParser parser5(testHeading5);
    MarkdownParser parser6(testHeading6);

    Markdown md1 = parser1.parse();
    Markdown md2 = parser2.parse();
    Markdown md3 = parser3.parse();
    Markdown md4 = parser4.parse();
    Markdown md5 = parser5.parse();
    Markdown md6 = parser6.parse();

    ContentBlockType cbType1 = md1.cbType;
    ContentBlockType cbType2 = md2.cbType;
    ContentBlockType cbType3 = md3.cbType;
    ContentBlockType cbType4 = md4.cbType;
    ContentBlockType cbType5 = md5.cbType;
    ContentBlockType cbType6 = md6.cbType;
    // ContentBlockType cbTypeCode = getCBType(testCodeBlock);
    // ContentBlockType cbTypeBullet = getCBType(testBullet);
    // ContentBlockType cbTypeNumbering = getCBType(testNumbering);

    ASSERT_EQ((int)cbType1, (int)ContentBlockType::Heading1);
    ASSERT_EQ((int)cbType2, (int)ContentBlockType::Heading2);
    ASSERT_EQ((int)cbType3, (int)ContentBlockType::Heading3);
    ASSERT_EQ((int)cbType4, (int)ContentBlockType::Heading4);
    ASSERT_EQ((int)cbType5, (int)ContentBlockType::Heading5);
    ASSERT_EQ((int)cbType6, (int)ContentBlockType::Heading6);
    // ASSERT_EQ((int)cbTypeCode, (int)ContentBlockType::Code);
    // ASSERT_EQ((int)cbTypeBullet, (int)ContentBlockType::Bullet);
    // ASSERT_EQ((int)cbTypeNumbering, (int)ContentBlockType::Numbering);
TEST_END

TEST_START(mdParsingBasic, "Markdown Parsing Basic Test")
    Rope* mdText = new Rope("# some heading 1 text");

    // Markdown md = parseContent(mdText);
    MarkdownParser parser(mdText);
    Markdown md = parser.parse();

    ASSERT_EQ((int)md.cbType, (int)ContentBlockType::Heading1);
    ASSERT_EQ(md.fragments.size(), 1);
    ASSERT_EQ((int)md.fragments[0].fragType, (int)MarkdownFragmentType::Normal);

    // "some heading 1 text"
    ASSERT_EQ(md.fragments[0].text.startIndex, 2); 
    ASSERT_EQ(md.fragments[0].text.length, 19); 
TEST_END

TEST_START(bulletParsing, "Markdown bullet parsing")
    Rope* bulletTest = new Rope("- Some bullet");

    MarkdownParser parser(bulletTest);
    Markdown md = parser.parse();

    ASSERT_EQ((int)md.cbType, (int)ContentBlockType::Bullet);
    ASSERT_EQ(md.fragments.size(), 1);
    ASSERT_EQ((int)md.fragments[0].fragType, (int)MarkdownFragmentType::Normal);

    // "some heading 1 text"
    ASSERT_EQ(md.fragments[0].text.startIndex, 2); 
    ASSERT_EQ(md.fragments[0].text.length, 11); 
TEST_END

TEST_START(imageParsing, "Markdown Image Parsing Test")
    Rope* testRope = new Rope("![someImage.jpg](Some Image!)");
    MarkdownParser parser(testRope);
    Markdown md = parser.parse();

    ASSERT_EQ((int)md.cbType, (int)ContentBlockType::Image);
    ASSERT_EQ(md.fragments.size(), 1);
    ASSERT_EQ((int)md.fragments[0].fragType, (int)MarkdownFragmentType::Image);

    ASSERT_EQ(md.fragments[0].imgOrLink.url.startIndex, 2);
    ASSERT_EQ(md.fragments[0].imgOrLink.url.length, 13);
    ASSERT_EQ(md.fragments[0].imgOrLink.text.startIndex, 17);
    ASSERT_EQ(md.fragments[0].imgOrLink.text.length, 11);
TEST_END

int main() {
    cout << "\nRunning md tests (" << __FILE__ << ")";

    RUN_TEST(cbType);
    RUN_TEST(mdParsingBasic);
    RUN_TEST(bulletParsing);
    RUN_TEST(imageParsing);

    WRAP_TESTS();

    return 0;
}

