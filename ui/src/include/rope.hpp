#ifndef PATRAM_UI_ROPE
#define PATRAM_UI_ROPE

#include <string>
#include <stack>
#include <tuple>

using namespace std;

class Rope {
    private:
        stack<Rope*> traverseStack;
        Rope* rightSiblingOrAncestor(Rope* node);
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
        Rope(Rope* r1, Rope* r2);

        inline bool isLeaf();
        string getString();
        char operator[](uint32_t i);
        void recalculateLenLeft();

        void startTraversing();
        bool hasNext();
        Rope* next();
        /**
         * Returns the leaf node and the relative index to that node.
         */
        tuple<Rope*, uint32_t> findLeafForIndex(uint32_t i);
        void insert(uint32_t i, string s);
        uint32_t length();
        void append(string s);
        void prepend(string s);
        void remove(uint32_t startIndex, uint32_t length);
        void clear();
};

struct TraversalNode {
    Rope* node;
    bool visited;

    TraversalNode(Rope* node): node(node), visited(false) {}
};

#endif // PATRAM_UI_ROPE

