#include <iostream>
#include <sstream>
#include <list>
#include <vector>

#include "include/rope.hpp"

using namespace std;

Rope::Rope() {
    text = "";
    lenLeft = 0;
    parent = nullptr;
    left = nullptr;
    right = nullptr;
}

Rope::Rope(string text) {
    this->text = text;
    lenLeft = text.length();
    parent = nullptr;
    left = nullptr;
    right = nullptr;
}

Rope::Rope(char* text) {
    this->text = text;
    lenLeft = strlen(text);
    parent = nullptr;
    left = nullptr;
    right = nullptr;
}

Rope::Rope(Rope* r1, Rope* r2) {
    this->text = "";
    parent = nullptr;
    left = r1;
    right = r2;
    left->parent = this;
    right->parent = this;

    if (left->isLeaf()) {
        lenLeft = left->text.length();
    } else {
        recalculateLenLeft();
    }
}

Rope::Rope(Rope r1, Rope r2) {
    this->text = "";
    parent = nullptr;
    left = r1.clone();
    right = r2.clone();
    left->parent = this;
    right->parent = this;

    if (left->isLeaf()) {
        lenLeft = left->text.length();
    } else {
        recalculateLenLeft();
    }
}

Rope::Rope(string s1, string s2) {
    this->text = "";
    left = new Rope(s1);
    right = new Rope(s2);
    lenLeft = s1.length();
    left->parent = this;
    right->parent = this;
    parent = nullptr;
}

Rope::~Rope() {
    if (left != nullptr) {
        delete left;
        left = nullptr;
    }

    if (right != nullptr) {
        delete right;
        right = nullptr;
    }
}

inline bool Rope::isLeaf() {
    return left == nullptr && right == nullptr;
}

Rope* Rope::rightSiblingOrAncestor(Rope* node) {
    if (node == nullptr || node->parent == nullptr || node == this) {
        return nullptr;
    }

    if (node == node->parent->left && node->parent->right != nullptr) {
        return node->parent->right;
    }

    while (
        node != nullptr && node->parent != nullptr && node != this
        && node != this->right && (node == node->parent->right || node->parent->right == nullptr)
    ) {
        node = node->parent;
    }

    if (node == nullptr || node == this || node == this->right || node->parent == nullptr) {
        return nullptr;
    }

    return node->parent->right;
}

Rope* Rope::leftSiblingOrAncestor(Rope* node) {
    if (node == nullptr || node->parent == nullptr || node == this) {
        return nullptr;
    }

    if (node == node->parent->right && node->parent->left != nullptr) {
        return node->parent->left;
    }

    while (
        node != nullptr && node->parent != nullptr && node != this
        && node != this->right && (node == node->parent->right || node->parent->right == nullptr)
    ) {
        node = node->parent;
    }

    if (node == nullptr || node == this || node == this->right || node->parent == nullptr) {
        return nullptr;
    }

    return node->parent->left;
}

string Rope::getString() {
    stringstream ss;
    Rope* node = this;

    while (node != nullptr) {
        if (node->isLeaf()) {
            ss << node->text;

            node = rightSiblingOrAncestor(node);
            continue;
        }

        if (node->left != nullptr) {
            node = node->left;
            continue;
        }

        if (node->right != nullptr) {
            node = node->right;
        }
    }

    return ss.str();
}

void Rope::recalculateLenLeft() {
    RopeTraverser rt(left);
    lenLeft = 0;

    while (rt.hasNext()) {
        lenLeft += rt.next()->text.length();
    }
}

char Rope::operator[](uint32_t i) {
    tuple<Rope*, uint32_t> result = findLeafForIndex(i);
    Rope* node = get<0>(result);

    if (node != nullptr) {
        uint32_t index = get<1>(result);
        return node->text[index];
    }

    return '\0';
}

tuple<Rope*, uint32_t> Rope::findLeafForIndex(uint32_t i) {
    uint32_t index = i;
    Rope * node = this;

    while (node != nullptr) {
        if (node->isLeaf()) {
            return { node, index };
        }

        if (node->lenLeft <= index) {
            index -= node->lenLeft;
            node = node->right;
            continue;
        }

        node = node->left;
    }

    return { nullptr, 0 };
}

void Rope::append(string s) {
    if (isLeaf()) {
        if (!text.empty()) {
            left = new Rope(text);
            right = new Rope(s);

            left->parent = this;
            right->parent = this;

            text = "";
            lenLeft = left->text.length();
            return;
        }

        text = s;
        lenLeft = text.length();
        return;
    }

    if (right == nullptr) {
        right = new Rope(s);
        right->parent = this;
        return;
    }

    Rope* node = right;

    while (node->right != nullptr) { node = node->right; }

    node->text.append(s);
}

void Rope::prepend(string s) {
    if (isLeaf()) {
        if (!text.empty()) {
            left = new Rope(s);
            right = new Rope(text);

            left->parent = this;
            right->parent = this;

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
        left->parent = this;
        return;
    }

    Rope* node = left;

    while (node->left != nullptr) { node = node->left; }

    Rope* newLeft = new Rope(s);
    Rope* newRight = new Rope(node->text);
    newLeft->parent = node;
    newRight->parent = node;
    text = "";
    lenLeft = s.length();
    node->left = newLeft;
    node->right = newRight;
    propagateLenLeft(newLeft, s.length());
}

string Rope::substring(uint32_t offset, uint32_t length) {
    stringstream ss;
    RopeTraverser rt(this);

    uint32_t i = offset;
    uint32_t len = length;

    while (rt.hasNext()) {
        Rope* node = rt.next();

        if (i >= node->length()) {
            i -= node->length();
            continue;
        }

        if (len <= node->text.length()) {
            ss << node->text.substr(i, len);
            break;
        }

        if (node->text.length() <= len) {
            ss << node->text.substr(i, node->text.length() - i);
        } else {
            ss << node->text.substr(i, len);
        }

        len -= node->text.length() - i;
        i = 0;
    }

    return ss.str();
}

string Rope::substring(RopeViewData rvd) { return substring(rvd.startIndex, rvd.length); }

Rope* Rope::split(uint32_t index) {
    if (isLeaf()) {
        string secondText = text.substr(index, text.length() - index);
        text.erase(index, text.length() - index);
        left = new Rope(text);
        right = new Rope(secondText);
        left->parent = this;
        right->parent = this;
        text.clear();
        text.resize(0);

        recalculateLenLeft();

        return this;
    }

    RopeTraverser rt(this);

    uint32_t i = index;

    while (rt.hasNext()) {
        Rope* node = rt.next();

        if (node->lenLeft <= i) {
            i -= node->lenLeft;
            continue;
        }

        string secondText = node->text.substr(i, node->text.length() - i);

        node->text.erase(i, node->text.length() - i);
        node->left = new Rope(node->text);
        node->left->parent = node;
        node->right = new Rope(secondText);
        node->right->parent = node;
        node->text.clear();
        node->text.resize(0);

        recalculateLenLeft();

        return node;
    }

    return nullptr;
}

void Rope::pick(Rope* leaf) {
    Rope* p = leaf->parent;

    if (leaf == p->left) {
        p->text = p->right->text;
        delete p->right;
        p->right = nullptr;
        p->left = nullptr;

        recalculateLenLeft();
        return;
    }

    p->text = p->left->text;
    delete p->left;
    p->right = nullptr;
    p->left = nullptr;

    recalculateLenLeft();
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

    if (oldLeaf == nullptr) {
        return;
    }

    uint32_t len = oldLeaf->text.length();
    uint32_t indx = get<1>(oldLeafTuple);

    if ((indx == 0 || indx == len) && oldLeaf->parent->left != nullptr) {
        Rope* node = oldLeaf->parent->left;
        node->text.append(s);
        int32_t diff = node->text.length() - node->lenLeft;
        node->lenLeft = node->text.length();
        propagateLenLeft(node, diff);
        return;
    }

    string str1 = oldLeaf->text.substr(0, indx);
    string str2 = oldLeaf->text.substr(indx, len - indx);

    if (str2.empty()) {
        Rope* newLeft = new Rope(str1);
        Rope* newRight = new Rope(s);
        newLeft->parent = oldLeaf;
        newRight->parent = oldLeaf;

        oldLeaf->left = newLeft;
        oldLeaf->right = newRight;

        oldLeaf->text = "";
        oldLeaf->lenLeft = newLeft->text.length();
        return;
    }

    Rope* newLeft = new Rope(str1);
    Rope* newRightLeft = new Rope(s);
    Rope* newRightRight = new Rope(str2);

    newLeft->parent = oldLeaf;

    oldLeaf->left = newLeft;
    oldLeaf->right = new Rope(newRightLeft, newRightRight);

    oldLeaf->right->parent = oldLeaf;

    oldLeaf->text = "";
    oldLeaf->lenLeft = newLeft->text.length();
}

void Rope::remove(uint32_t startIndex, uint32_t length) {
    if (isLeaf()) {
        if (startIndex + length <= text.length()) {
            text.erase(startIndex, length);
            lenLeft = text.length();
        }

        return;
    }

    tuple<Rope*, uint32_t> result = findLeafForIndex(startIndex);

    Rope* node = get<0>(result);
    uint32_t index = get<1>(result);

    if (node != nullptr) {
        if (index + length == node->text.length()) {
            int64_t change;

            if (index == 0) {
                change = static_cast<int64_t>(node->text.length());

                Rope* nodeParent = node->parent;

                if (nodeParent->right == node) {
                    nodeParent->text = nodeParent->left->text;
                    nodeParent->lenLeft = nodeParent->left->lenLeft;
                } else {
                    nodeParent->text = nodeParent->right->text;
                    nodeParent->lenLeft = nodeParent->right->lenLeft;
                }

                delete nodeParent->left;
                delete nodeParent->right;

                nodeParent->left = nullptr;
                nodeParent->right = nullptr;

                propagateLenLeft(nodeParent, -change);
            } else {
                change = static_cast<int64_t>(length);

                node->text.erase(index, length);
                propagateLenLeft(node, -change);
                node->lenLeft = node->text.length();
            }
        } else if (index == 0 && length < node->text.length()) {
            uint32_t lenBefore = node->text.length();
            uint32_t lenAfter = node->text.length() - length;
            node->text = node->text.substr(length, lenAfter);
            node->lenLeft = node->text.length();
            propagateLenLeft(node, lenAfter - lenBefore);
        } else if (index + length < node->text.length()) {
            Rope* newLeft = new Rope(node->text.substr(0, index));
            newLeft->parent = node;

            uint32_t i = index + length;
            Rope* newRight = new Rope(node->text.substr(i, node->text.length() - i));
            newRight->parent = node;

            node->text = "";
            node->left = newLeft;
            node->right = newRight;

            int64_t lenLeftDiff = static_cast<int64_t>(node->parent->lenLeft) - static_cast<int64_t>(newLeft->text.length());

            node->lenLeft = newLeft->text.length();

            propagateLenLeft(node->parent, -lenLeftDiff);
        } else {
            tuple<Rope*, uint32_t> result = findLeafForIndex(startIndex + length - 1);

            Rope* endNode = get<0>(result);
            uint32_t endIndex = get<1>(result);

            if (endNode == nullptr) {
                return;
            }

            if (index == 0 && node != endNode) {
                Rope* temp = rightSiblingOrAncestor(node->parent);

                node->~Rope();

                if (temp == nullptr) {
                    return;
                }

                node = temp;
            } else {
                int64_t change = static_cast<int64_t>(node->text.length() - index);

                node->text.erase(index, change);
                node->lenLeft = node->text.length();
                propagateLenLeft(node, -change);

                if (node != endNode) {
                    Rope* temp = rightSiblingOrAncestor(node);

                    if (temp != nullptr) {
                        node = temp;
                    }
                }
            }

            while (node != endNode) {
                if (node->isLeaf()) {
                    Rope* temp = rightSiblingOrAncestor(node);

                    propagateLenLeft(node, -static_cast<int64_t>(node->text.length()));

                    node->~Rope();

                    if (temp == nullptr) {
                        break;
                    }

                    node = temp;

                    continue;
                }

                if (node->left != nullptr) {
                    node = node->left;
                    continue;
                }

                if (node->right != nullptr) {
                    node = node->right;
                }
            }

            uint64_t len = endIndex + 1;
            endNode->text.erase(0, len);
            endNode->lenLeft = endNode->text.length();
            propagateLenLeft(endNode, -len);
        }
    }
}

uint32_t Rope::length() {
    uint32_t len = 0;

    RopeTraverser rt(this);

    while (rt.hasNext()) {
        Rope* node = rt.next();
        len += node->text.length();
    }

    return len;
}

void Rope::clear() {
    text = "";
    lenLeft = 0;

    if (left != nullptr) {
        left->~Rope();
        left = nullptr;
    }

    if (right != nullptr) {
        right->~Rope();
        right = nullptr;
    }
}

void Rope::propagateLenLeft(Rope* node, int64_t change) {
    if (node == nullptr || node->parent == nullptr) {
        return;
    }

    if (node == node->parent->left) {
        node->parent->lenLeft += change;
    }

    node = node->parent;

    while (node != this && node->parent != this && node != nullptr && node->parent != nullptr) {
        if (node == node->parent->left) {
            node->parent->lenLeft += change;
        }

        node = node->parent;
    }

    if (node->parent == this && node == left) {
        lenLeft += change;
    }
}

void Rope::balance() {
    list<Rope*> ropeList;
    vector<Rope*> toDelete;

    toDelete.reserve(128);

    Rope* node = this;
    Rope* leftNode = nullptr;

    while (node != nullptr) {
        if (node->isLeaf()) {
            Rope* nextNode = rightSiblingOrAncestor(node);
            ropeList.push_back(node);
            leftNode = nullptr;

            if (nextNode == nullptr) {
                break;
            }

            node = nextNode;
        } else {
            if (node != this && node != nullptr) {
                toDelete.push_back(node);
            }
        }

        if (node->left != nullptr) {
            node = node->left;
            continue;
        }

        if (node->right != nullptr) {
            node = node->right;
        }
    }

    list<Rope*>::iterator leftItr;

    uint32_t count = 0;

    while (ropeList.size() > 1) {
        Rope* leftNode = nullptr;

        for (list<Rope*>::iterator itr = ropeList.begin(); itr != ropeList.end(); ++itr) {
            if (leftNode == nullptr) {
                leftNode = *itr;
                leftItr = itr;

                continue;
            }

            Rope* rightNode = *itr;

            Rope* newNode = new Rope(leftNode, rightNode);
            *itr = newNode;

            ropeList.erase(leftItr, itr);

            leftNode = nullptr;
        }
    }

    node = ropeList.front();

    left = node->left;
    right = node->right;
    lenLeft = node->lenLeft;
    left->parent = this;
    right->parent = this;

    node->left = nullptr;
    node->right = nullptr;
    node->~Rope();

    for (Rope* node: toDelete) {
        if (node != nullptr) {
            node->right = nullptr;
            node->left = nullptr;
            node->parent = nullptr;
            node->~Rope();
        }
    }
}

inline Rope* Rope::clone() { return new Rope(left, right); }
inline Rope* Rope::cloneLeaf() { return new Rope(text); }

void Rope::print(uint32_t depth) {
    #define INDENT() for (int i=0; i<depth; ++i) { cout << "    "; }

    Rope* node = this;

    if (depth == 0) {
        cout << "\n==[ROPE]==";
    }

    cout << "\n";
    INDENT();
    cout << "text: \"" << text << "\"";

    if (left != nullptr) {
        cout << "\n";
        INDENT();
        cout << "left: ";
        left->print(depth + 1);
    }

    if (right != nullptr) {
        cout << "\n";
        INDENT();
        cout << "right: ";
        right->print(depth + 1);
    }

    #undef INDENT
}

RopeTraverser::RopeTraverser(Rope* root) {
    this->root = root;
    reset();
}

RopeTraverser::RopeTraverser(RopeTraverser& rt) {
    root = rt.root;
    current = rt.current;
    nextInQueue = rt.nextInQueue;
}

inline bool RopeTraverser::hasNext() { return nextInQueue != nullptr; }

Rope* RopeTraverser::next() {
    current = nextInQueue;
    nextInQueue = nullptr;

    Rope* node = root->rightSiblingOrAncestor(current);

    while (node != nullptr && node != root) {
        if (node->isLeaf()) {
            nextInQueue = node;
            break;
        }

        if (node->left != nullptr) {
            node = node->left;
            continue;
        }

        if (node->right != nullptr) {
            node = node->right;
        }
    }

    return current;
}

void RopeTraverser::reset() {
    current = root;

    if (current->isLeaf()) {
        nextInQueue = current;
        return;
    }

    Rope* node = current;

    while (node != nullptr) {
        if (node->isLeaf()) {
            nextInQueue = node;
            return;
        }

        if (node->left != nullptr) {
            node = node->left;
            continue;
        }

        if (node->right != nullptr) {
            node = node->right;
        }
    }
}

ReverseRopeTraverser::ReverseRopeTraverser(Rope* root) {
    this->root = root;
    reset();
}

bool ReverseRopeTraverser::hasNext() { return nextInQueue != nullptr; }

Rope* ReverseRopeTraverser::next() {
    current = nextInQueue;
    nextInQueue = nullptr;

    Rope* node = root->leftSiblingOrAncestor(current);

    while (node != nullptr && node != root) {
        if (node->isLeaf()) {
            nextInQueue = node;
            break;
        }

        if (node->left != nullptr) {
            node = node->left;
            continue;
        }

        if (node->right != nullptr) {
            node = node->right;
        }
    }

    return current;
}

void ReverseRopeTraverser::reset() {
    current = root;

    if (current->isLeaf()) {
        nextInQueue = current;
        return;
    }

    Rope* node = current;

    while (node != nullptr) {
        if (node->isLeaf()) {
            nextInQueue = node;
            return;
        }

        if (node->left != nullptr) {
            node = node->left;
            continue;
        }

        if (node->right != nullptr) {
            node = node->right;
        }
    }
}

// ---
// RopeCharIterator
// ---

RopeCharIterator::RopeCharIterator(Rope* root): rt(root) {
    this->root = root;
    if (rt.hasNext()) {
        current = rt.next();
    } else {
        current = nullptr;
    }
    currentRopeIndex = 0;
    globalIndex = 0;
    newIterator = true;
}

bool RopeCharIterator::hasNext() {
    if (current != nullptr) {
        if (currentRopeIndex < current->text.length()) {
            return true;
        } else {
            return rt.hasNext();
        }
    }

    return false;
}

char RopeCharIterator::next() {
    if (newIterator) {
        newIterator = false;

        if (current->text.length() > 0) {
            return current->text[0];
        }
    }

    if (current != nullptr) {
        if (currentRopeIndex + 1 < current->text.length()) {
            ++currentRopeIndex;
            ++globalIndex;

            return current->text[currentRopeIndex];
        }

        if (rt.hasNext()) {
            currentRopeIndex = 0;
            ++globalIndex;
            current = rt.next();

            return current->text[currentRopeIndex];
        }
    }

    current = nullptr;

    return 0;
}

char RopeCharIterator::peekNext() {
    uint32_t newIndex = currentRopeIndex + 1;

    if (newIndex < current->text.length()) {
        return current->text[newIndex];
    } else if (rt.hasNext()) {
        RopeTraverser rt2(rt);
        Rope* newCurrent = rt2.next();

        return newCurrent->text[0];
    }

    return 0;
}

void RopeCharIterator::reset() {
    rt.reset();
    currentRopeIndex = 0;
    globalIndex = 0;
    current = root;
    newIterator = true;
}

Rope* RopeCharIterator::getRootRope() { return root; }
uint32_t RopeCharIterator::index() { return globalIndex; }

void RopeCharIterator::setIndex(uint32_t index) {
    if (index > root->length()) { return; }

    if (index > globalIndex) { reset(); }

    if (index) { newIterator = false; }

    uint32_t i = index;
    RopeTraverser rt2(root);

    while (rt2.hasNext()) {
        Rope* r = rt2.next();

        if (i < r->length()) {
            currentRopeIndex = i;
            current = r;
            globalIndex = index;
            return;
        }

        if (i == r->length()) {
            i = 0;
            continue;
        }

        i -= r->length();
    }
}

