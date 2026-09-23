#ifndef PATRAM_UI_ROPE
#define PATRAM_UI_ROPE

#include <string>
#include <tuple>

using namespace std;

struct RopeViewData {
    uint32_t startIndex;
    uint32_t length;
};

class Rope {
    private:
        void propagateLenLeft(Rope* node, int64_t change);

    public:
        string text;
        uint32_t lenLeft;

        Rope* parent;
        Rope* left;
        Rope* right;

        ~Rope();
        Rope();
        Rope(string);
        Rope(char*);
        Rope(Rope* r1, Rope* r2);
        Rope(Rope r1, Rope r2);
        Rope(string s1, string s2);

        static Rope* deepCopy(Rope* rope) {
            return new Rope(rope->getString());
        }

        inline bool isLeaf();
        string getString();
        char operator[](uint32_t i);
        void recalculateLenLeft();

        Rope* rightSiblingOrAncestor(Rope* node);
        Rope* leftSiblingOrAncestor(Rope* node);
        /**
         * Returns the leaf node and the relative index to that node.
         */
        tuple<Rope*, uint32_t> findLeafForIndex(uint32_t i);
        void insert(uint32_t i, string s);
        uint32_t length();
        void append(string s);
        void prepend(string s);
        string substring(uint32_t offset, uint32_t length);
        string substring(RopeViewData rvd);
        Rope* split(uint32_t index);
        void pick(Rope* leaf);
        void remove(uint32_t startIndex, uint32_t length);
        void clear();
        void balance();
        Rope* clone();
        Rope* cloneLeaf();
        void print(uint32_t depth = 0);
};

struct RopeView {
    uint32_t startIndex;
    uint32_t length;
    Rope* rope;
};

class RopeTraverser {
    public:
        Rope* root;
        Rope* current;
        Rope* nextInQueue;

        RopeTraverser(Rope* root);
        RopeTraverser(RopeTraverser&);
        bool hasNext();
        Rope* next();
        void reset();
};

class ReverseRopeTraverser {
    private:
        Rope* root;
        Rope* current;
        Rope* nextInQueue;

    public:
        ReverseRopeTraverser(Rope* root);
        bool hasNext();
        Rope* next();
        void reset();
};

class RopeCharIterator {
    private:
        bool newIterator;

        uint32_t globalIndex;
        uint32_t currentRopeIndex;

        Rope* root;
        Rope* current;
        RopeTraverser rt;

    public:
        RopeCharIterator(Rope* root);

        bool hasNext();
        char next();
        char peekNext();
        void reset();
        Rope* getRootRope();
        uint32_t index();
        void setIndex(uint32_t index);
};

#endif // PATRAM_UI_ROPE

