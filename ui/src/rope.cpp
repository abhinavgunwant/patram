#include <iostream>
#include <sstream>

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

Rope::Rope(Rope* r1, Rope* r2) {
    this->text = "";
    left = r1;
    right = r2;
    left->parent = this;
    right->parent = this;
    recalculateLenLeft();
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

    if (node == nullptr || node == this || node == this->right) {
        return nullptr;
    }

    return node->parent->right;
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

void Rope::recalculateLenLeft() {
    if (left != nullptr) {
        lenLeft = 0;
        Rope* node = left;

        while (node != this) {
            if (node->isLeaf()) {
                lenLeft += node->text.length();

                node = rightSiblingOrAncestor(node);

                if (node == right || node == nullptr) {
                    break;
                }

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
            lenLeft = text.length();
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

    Rope* newLeft = new Rope(text);
    Rope* newRight = new Rope(s);
    newLeft->parent = node;
    newRight->parent = node;
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
        if (startIndex + length < text.length()) {
            text.erase(startIndex, length);
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
                if (node->parent->right == node) {
                    node->parent->right = nullptr;
                } else {
                    node->parent->left = nullptr;
                }

                change = static_cast<int64_t>(node->text.length());

                propagateLenLeft(node, -change);

                node->~Rope();
            } else {
                change = static_cast<int64_t>(length);

                node->text.erase(index, length);
                propagateLenLeft(node, -change);
            }
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
            propagateLenLeft(endNode, -len);
        }
    }
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
    if (node == nullptr && node->parent == nullptr) {
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

