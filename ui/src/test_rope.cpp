#include "include/rope.hpp"

#include <iostream>

uint32_t testPassed = 0;
uint32_t totalTests = 0;

bool allPassed = true;
bool passed = true;

#define TEST_START(funName, _testName) void funName () {\
    string testName = _testName; cout << "\nTesting \""\
    << testName << "\"";

#define TEST_END cout << "\n    \"" << testName << "\" ";\
        if(passed) {\
            ++testPassed;\
            cout << "\033[32mpassed.\033[0m";\
        } else {\
            cout << "\033[31mfailed.\033[0m";\
        }\
    }

#define RUN_TEST(funName) passed = true;\
    ++totalTests;\
    funName()

#define ASSERT(x) if (x == false) {\
    allPassed = false;\
    passed = false;\
    cout << "\n\033[31mAssertion at line " << __LINE__ << " failed.\033[0m";\
    }

#define ASSERT_EQ(x, y) if (x != y) {\
    allPassed = false;\
    passed = false;\
    cout << "\nAssertion at line " << __LINE__ << " failed. Expected: `" << y << "`, Got: `" << x << "`";\
    }

#define ASSERT_NULL(x) if (x != nullptr) {\
    allPassed = false;\
    passed = false;\
    cout << "\nAssertion at line " << __LINE__ << " failed. Expected NULL " << ", Got: `" << x << "`";\
    }

TEST_START(basicTest, "Basic Test")
    Rope a("Say ");
    Rope b("hello ");
    Rope c("to");
    Rope d(" the");
    Rope e(" beautiful");
    Rope f(" world!");

    Rope g(&b, &c);
    Rope h(&a, &g);
    Rope i(&d, &e);
    Rope j(&i, &f);

    Rope k(&h, &j);

    ASSERT_EQ(k.getString(), "Say hello to the beautiful world!");
    ASSERT_EQ(k[8], 'o');
    ASSERT_EQ(k[16], ' ');
    ASSERT_EQ(k[20], 'u');
TEST_END

TEST_START(testInsert, "Insert test")
    Rope hello("world!");

    ASSERT_EQ(hello.getString(), "world!");

    hello.insert(0, "Hello ");

    ASSERT_EQ(hello.getString(), "Hello world!");

    hello.insert(6, "beautiful ");

    ASSERT_EQ(hello.getString(), "Hello beautiful world!");
TEST_END

TEST_START(testPrepend, "Prepend test")
    Rope hello("day!");

    ASSERT_EQ(hello.getString(), "day!");

    hello.prepend("awesome ");

    ASSERT_EQ(hello.getString(), "awesome day!");

    hello.prepend("an ");

    ASSERT_EQ(hello.getString(), "an awesome day!");

    hello.prepend("Today's ");

    ASSERT_EQ(hello.getString(), "Today's an awesome day!");
TEST_END

TEST_START(testRemove, "Remove test")
    Rope a("Say ");
    Rope b("hello ");
    Rope c("to");
    Rope d(" the");
    Rope e(" beautiful");
    Rope f(" world!");

    Rope g(&b, &c);
    Rope h(&a, &g);
    Rope i(&d, &e);
    Rope j(&i, &f);

    Rope k(&h, &j);

    ASSERT_EQ(k.getString(), "Say hello to the beautiful world!");

    k.remove(16, 10);

    ASSERT_EQ(k[17], 'w');
    ASSERT_EQ(k.getString(), "Say hello to the world!");

    k.remove(18, 3);

    ASSERT_EQ(k.getString(), "Say hello to the wd!");

    k.remove(8, 3);

    ASSERT_EQ(k.getString(), "Say hello the wd!");

    k.remove(7, 8);

    ASSERT_EQ(k.getString(), "Say held!");
TEST_END

TEST_START(testClear, "Clear test")
    Rope a("Say ");
    Rope b("hello ");
    Rope c("to");
    Rope d(" the");
    Rope e(" beautiful");
    Rope f(" world!");

    Rope g(&b, &c);
    Rope h(&a, &g);
    Rope i(&d, &e);
    Rope j(&i, &f);

    Rope k(&h, &j);

    ASSERT_EQ(k.getString(), "Say hello to the beautiful world!");

    k.clear();

    ASSERT_EQ(k.getString(), "");
    ASSERT_EQ(k.length(), 0);
    ASSERT_NULL(k.left);
    ASSERT_NULL(k.right);
TEST_END

int main() {
    cout << "Testing " << __FILE__ << "\n";
    RUN_TEST(basicTest);
    RUN_TEST(testInsert);
    RUN_TEST(testPrepend);
    RUN_TEST(testRemove);
    RUN_TEST(testClear);

    if (allPassed) {
        cout << "\n\nAll tests passed!";
        return 0;
    }

    cout << "\n\n" << testPassed << "/" << totalTests << " tests passed!";
    return 1;
}

