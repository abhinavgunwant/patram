#ifndef PATRAM_UI_ROPE
#define PATRAM_UI_ROPE

#include <string>
#include <stack>
#include <tuple>

using namespace std;

class Rope {
    private:
        stack<Rope*> traverseStack;

    public:
        string text;
        uint32_t lenLeft;
        Rope* left;
        Rope* right;

        ~Rope();
        Rope();
        Rope(string);
        Rope(Rope* r1, Rope* r2);

        inline bool isLeaf();
        string getString();
        char operator[](uint32_t i);
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
        void clear();
};

struct TraversalNode {
    Rope* node;
    bool visited;

    TraversalNode(Rope* node): node(node), visited(false) {}
};

#endif // PATRAM_UI_ROPE

