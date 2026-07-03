#ifndef PATRAM_SYS_TEST
#define PATRAM_SYS_TEST

#include <chrono>

#define SETUP_TEST uint32_t testPassed = 0, totalTests = 0; bool allPassed = true, passed = true; auto t0 = std::chrono::high_resolution_clock::now()

#define WRAP_TESTS() do {\
    auto t1 = std::chrono::high_resolution_clock::now();\
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0);\
    if (allPassed) {\
        cout << "\n\nAll tests passed!";\
        cout << "\nTime elapsed: " << duration;\
        return 0;\
    }\
    cout << "\n\n" << testPassed << "/" << totalTests << " tests passed!";\
    cout << "\nTime elapsed: " << duration;\
} while (false)

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

#endif // PATRAM_SYS_TEST

