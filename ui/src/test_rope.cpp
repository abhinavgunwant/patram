#include "include/rope.hpp"
#include "test.hpp"

#include <iostream>

SETUP_TEST;

TEST_START(basicTest, "Basic Test")
    Rope* a = new Rope("Say ");
    Rope* b = new Rope("hello ");
    Rope* c = new Rope("to");
    Rope* d = new Rope(" the");
    Rope* e = new Rope(" beautiful");
    Rope* f = new Rope(" world!");

    Rope* g = new Rope(b, c);
    Rope* h = new Rope(a, g);
    Rope* i = new Rope(d, e);
    Rope* j = new Rope(i, f);

    Rope* k = new Rope(h, j);

    ASSERT_EQ(k->getString(), "Say hello to the beautiful world!");
    ASSERT_EQ((*k)[8], 'o');
    ASSERT_EQ((*k)[16], ' ');
    ASSERT_EQ((*k)[20], 'u');
    ASSERT_EQ(k->length(), 33);

    Rope* testLen = new Rope("Let's say this is a string with length 42.");

    ASSERT_EQ(testLen->length(), 42);
TEST_END

TEST_START(testInsert, "Insert test")
    Rope* hello = new Rope("world!");

    ASSERT_EQ(hello->getString(), "world!");

    hello->insert(0, "Hello ");

    ASSERT_EQ(hello->getString(), "Hello world!");

    hello->insert(6, "beautiful ");

    ASSERT_EQ(hello->getString(), "Hello beautiful world!");
TEST_END

TEST_START(testPrepend, "Prepend test")
    Rope* hello = new Rope("day!");

    ASSERT_EQ(hello->getString(), "day!");

    hello->prepend("awesome ");

    ASSERT_EQ(hello->getString(), "awesome day!");

    hello->prepend("an ");

    ASSERT_EQ(hello->getString(), "an awesome day!");

    hello->prepend("Today's ");

    ASSERT_EQ(hello->getString(), "Today's an awesome day!");
TEST_END

TEST_START(testRemove, "Remove test")
    Rope* a = new Rope("Say ");
    Rope* b = new Rope("hello ");
    Rope* c = new Rope("to");
    Rope* d = new Rope(" the");
    Rope* e = new Rope(" beautiful");
    Rope* f = new Rope(" world!");

    Rope* g = new Rope(b, c);
    Rope* h = new Rope(a, g);
    Rope* i = new Rope(d, e);
    Rope* j = new Rope(i, f);

    Rope* k = new Rope(h, j);

    ASSERT_EQ(k->getString(), "Say hello to the beautiful world!");

    k->remove(16, 10);

    ASSERT_EQ((*k)[17], 'w');
    ASSERT_EQ(k->getString(), "Say hello to the world!");

    k->remove(18, 3);

    ASSERT_EQ(k->getString(), "Say hello to the wd!");

    k->remove(8, 3);

    ASSERT_EQ(k->getString(), "Say hello the wd!");

    k->remove(7, 8);

    ASSERT_EQ(k->getString(), "Say held!");

    delete k;
TEST_END

TEST_START(testClear, "Clear test")
    Rope* a = new Rope("Say ");
    Rope* b = new Rope("hello ");
    Rope* c = new Rope("to");
    Rope* d = new Rope(" the");
    Rope* e = new Rope(" beautiful");
    Rope* f = new Rope(" world!");

    Rope* g = new Rope(b, c);
    Rope* h = new Rope(a, g);
    Rope* i = new Rope(d, e);
    Rope* j = new Rope(i, f);

    Rope* k = new Rope(h, j);

    ASSERT_EQ(k->getString(), "Say hello to the beautiful world!");

    k->clear();

    ASSERT_EQ(k->getString(), "");
    ASSERT_EQ(k->length(), 0);
    ASSERT_NULL(k->left);
    ASSERT_NULL(k->right);
TEST_END

TEST_START(testBalance, "Balance test")
    Rope* a = new Rope("Lorem ");
    Rope* b = new Rope("ipsum ");
    Rope* c = new Rope("dolor ");
    Rope* d = new Rope("sit ");
    Rope* e = new Rope("amet, ");
    Rope* f = new Rope("consectetur ");
    Rope* g = new Rope("adipiscing ");
    Rope* h = new Rope("elit, ");
    Rope* i = new Rope("sed ");
    Rope* j = new Rope("do ");
    Rope* k = new Rope("eiusmod ");
    Rope* l = new Rope("tempor ");
    Rope* m = new Rope("incididunt ");
    Rope* n = new Rope("ut ");
    Rope* o = new Rope("labore ");
    Rope* p = new Rope("et ");
    Rope* q = new Rope("dolore ");
    Rope* r = new Rope("magna ");
    Rope* s = new Rope("aliqua.");

    Rope* t = new Rope(a, b);
    Rope* u = new Rope(t, c);
    Rope* v = new Rope(u, d);
    Rope* w = new Rope(v, e);
    Rope* x = new Rope(w, f);
    Rope* y = new Rope(x, g);
    Rope* z = new Rope(y, h);
    Rope* aa = new Rope(z, i);
    Rope* ab = new Rope(aa, j);
    Rope* ac = new Rope(ab, k);
    Rope* ad = new Rope(ac, l);
    Rope* ae = new Rope(ad, m);
    Rope* af = new Rope(ae, n);
    Rope* ag = new Rope(af, o);
    Rope* ah = new Rope(ag, p);
    Rope* ai = new Rope(ah, q);
    Rope* aj = new Rope(ai, r);
    Rope* ak = new Rope(aj, s);

    ASSERT_EQ(ak->getString(), "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");

    ak->balance();

    ASSERT_EQ(ak->getString(), "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");
TEST_END

TEST_START(testSubstring, "Substring test")
    Rope* r = new Rope("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.");

    Rope* r1 = new Rope("Lorem ");
    Rope* r2 = new Rope("ipsum ");
    Rope* r3 = new Rope("dolor ");
    Rope* r4 = new Rope("sit ");
    Rope* r5 = new Rope("amet.");

    Rope* r6 = new Rope(r1, r2);
    Rope* r7 = new Rope(r3, r4);
    Rope* r8 = new Rope(r7, r5);
    Rope* r9 = new Rope(r6, r8);

    ASSERT_EQ(r->substring(12, 5), "dolor");
    ASSERT_EQ(r->substring(0, 5), "Lorem");
    ASSERT_EQ(r->substring(116, 7), "aliqua.");

    ASSERT_EQ(r9->substring(0, 5), "Lorem");
    ASSERT_EQ(r9->substring(0, 6), "Lorem ");
    ASSERT_EQ(r9->substring(0, 7), "Lorem i");
    ASSERT_EQ(r9->substring(1, 7), "orem ip");
    ASSERT_EQ(r9->substring(1, 13), "orem ipsum do");
    ASSERT_EQ(r9->substring(1, 17), "orem ipsum dolor ");
TEST_END

int main() {
    cout << "Testing " << __FILE__ << "\n";
    RUN_TEST(basicTest);
    RUN_TEST(testInsert);
    RUN_TEST(testPrepend);
    RUN_TEST(testRemove);
    RUN_TEST(testClear);
    RUN_TEST(testBalance);
    RUN_TEST(testSubstring);

    WRAP_TESTS();

    return 0;
}

