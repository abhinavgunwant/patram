#include <iostream>
#include <sstream>
#include <cassert>

#include "include/rope.hpp"

using namespace std;

Rope::Rope() {
    text = "";
    lenLeft = 0;
    left = nullptr;
    right = nullptr;
}

Rope::Rope(string text) {
    this->text = text;
    lenLeft = text.length();
    left = nullptr;
    right = nullptr;
}

Rope::Rope(Rope* r1, Rope* r2) {
    this->text = "";
    lenLeft = r1->getString().length();
    left = r1;
    right = r2;
}

Rope::~Rope() {
    if (left != nullptr) {
        left->~Rope();
    }

    if (right != nullptr) {
        right->~Rope();
    }
}

inline bool Rope::isLeaf() {
    return left == nullptr && right == nullptr;
}

string Rope::getString() {
    stringstream ss;
    Rope* node = left;

    stack<Rope*> s;

    s.push(this);

    while (!s.empty()) {
        node = s.top();
        s.pop();

        if (node->isLeaf()) {
            ss << node->text;
        }

        if (node->right != nullptr) {
            s.push(node->right);
        }

        if (node->left != nullptr) {
            s.push(node->left);
        }
    }

    return ss.str();
}

void Rope::startTraversing() {
    while (!traverseStack.empty()) {
        traverseStack.pop();
    }

    traverseStack.push(this);
}

inline bool Rope::hasNext() { return !traverseStack.empty(); }

Rope* Rope::next() {
    while (!traverseStack.empty()) {
        Rope* node = traverseStack.top();
        traverseStack.pop();

        if (node->isLeaf()) {
            return node;
        }

        if (node->right != nullptr) {
            traverseStack.push(node->right);
        }

        if (node->left != nullptr) {
            traverseStack.push(node->left);
        }
    }

    return nullptr;
}

char Rope::operator[](uint32_t i) {
    tuple<Rope*, uint32_t> result = findLeafForIndex(i);
    Rope* node = get<0>(result);
    uint32_t index = get<1>(result);
    return node->text[index];
}

tuple<Rope*, uint32_t> Rope::findLeafForIndex(uint32_t i) {
    if (isLeaf()) {
        return {this, i};
    }

    if (lenLeft < i) {
        return right->findLeafForIndex(i - lenLeft);
    }

    return left->findLeafForIndex(i);
}

void Rope::append(string s) {
    if (isLeaf()) {
        if (!text.empty()) {
            left = new Rope(text);
            right = new Rope(s);

            text = "";
            lenLeft = text.length();
            return;
        }

        text = s;
        lenLeft = text.length();
        return;
    }

    if (right == nullptr) {
        right = new Rope(s);
        return;
    }

    Rope* node = right;

    while (node->right != nullptr) { node = node->right; }

    Rope* newLeft = new Rope(text);
    Rope* newRight = new Rope(s);
    text = "";
    lenLeft = text.length();
    node->left = newLeft;
    node->right = newRight;
}

void Rope::prepend(string s) {
    if (isLeaf()) {
        if (!text.empty()) {
            left = new Rope(s);
            right = new Rope(text);

            text = "";
            lenLeft = s.length();
            return;
        }

        text = s;
        lenLeft = s.length();
        return;
    }

    if (left == nullptr) {
        left = new Rope(s);
        return;
    }

    Rope* node = left;

    while (node->left != nullptr) { node = node->left; }

    Rope* newLeft = new Rope(s);
    Rope* newRight = new Rope(text);
    text = "";
    lenLeft = s.length();
    node->left = newLeft;
    node->right = newRight;
}

void Rope::insert(uint32_t i, string s) {
    if (i == 0) {
        prepend(s);
        return;
    }

    if (i == length()) {
        append(s);
        return;
    }

    tuple<Rope*, uint32_t> oldLeafTuple = findLeafForIndex(i);

    Rope* oldLeaf = get<0>(oldLeafTuple);
    uint32_t len = oldLeaf->text.length();
    uint32_t indx = get<1>(oldLeafTuple);

    string str1 = oldLeaf->text.substr(0, indx);
    string str2 = oldLeaf->text.substr(indx, len - indx);

    if (str2.empty()) {
        Rope* newLeft = new Rope(str1);
        Rope* newRight = new Rope(s);

        oldLeaf->left = newLeft;
        oldLeaf->right = newRight;

        oldLeaf->text = "";
        oldLeaf->lenLeft = newLeft->text.length();
        return;
    }

    Rope* newLeft = new Rope(str1);
    Rope* newRightLeft = new Rope(s);
    Rope* newRightRight = new Rope(str2);

    oldLeaf->left = newLeft;
    oldLeaf->right = new Rope(newRightLeft, newRightRight);

    oldLeaf->text = "";
    oldLeaf->lenLeft = newLeft->text.length();
}

uint32_t Rope::length() {
    uint32_t len = 0;
    startTraversing();

    while (hasNext()) {
        Rope* node = next();
        len += node->text.length();
    }

    return len;
}

int main() {
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

    Rope final(&h, &j);

    assert(final.getString() == "Say hello to the beautiful world!");
    assert(final[8] == 'o');
    assert(final[20] == 'u');

    Rope hello("world!");

    assert(hello.getString() == "world!");

    hello.insert(0, "Hello ");

    assert(hello.getString() == "Hello world!");

    hello.insert(6, "beautiful ");

    assert(hello.getString() == "Hello beautiful world!");


    //
    // ---------------
    //
    cout << "PASSED!";

    return 0;
}

