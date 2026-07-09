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

    ContentBlockType cbType1 = getCBType(testHeading1);
    ContentBlockType cbType2 = getCBType(testHeading2);
    ContentBlockType cbType3 = getCBType(testHeading3);
    ContentBlockType cbType4 = getCBType(testHeading4);
    ContentBlockType cbType5 = getCBType(testHeading5);
    ContentBlockType cbType6 = getCBType(testHeading6);
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

    Markdown md = parseContent(mdText);

    ASSERT_EQ((int)md.cbType, (int)ContentBlockType::Heading1);
    ASSERT_EQ(md.fragments.size(), 1);
    ASSERT_EQ((int)md.fragments[0].fragType, (int)MarkdownFragmentType::Normal);
    ASSERT_EQ(md.fragments[0].text, "some heading 1 text");
TEST_END

int main() {
    cout << "\nRunning md tests (" << __FILE__ << ")";

    RUN_TEST(cbType);
    RUN_TEST(mdParsingBasic);

    WRAP_TESTS();

    return 0;
}

