#include "AVLIndex.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cctype>

using namespace std;

AVLIndex::AVLIndex(const string& field)
    : Index(field), root(nullptr), nodeCount(0), numeric(Record::isNumericField(field)) {}

AVLIndex::~AVLIndex() {
    clear();
}

string AVLIndex::toLowerStr(const string& s) const {
    string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(tolower(static_cast<unsigned char>(out[i])));
    }
    return out;
}

int AVLIndex::getHeight(AVLNode* node) const {
    return node ? node->height : 0;
}

int AVLIndex::getBalance(AVLNode* node) const {
    return node ? (getHeight(node->left) - getHeight(node->right)) : 0;
}

void AVLIndex::updateHeight(AVLNode* node) {
    if (node) {
        int lh = getHeight(node->left);
        int rh = getHeight(node->right);
        node->height = 1 + (lh > rh ? lh : rh);
    }
}

AVLIndex::AVLNode* AVLIndex::rotateRight(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    updateHeight(y);
    updateHeight(x);
    return x;
}

AVLIndex::AVLNode* AVLIndex::rotateLeft(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    updateHeight(x);
    updateHeight(y);
    return y;
}

AVLIndex::AVLNode* AVLIndex::rebalance(AVLNode* node) {
    updateHeight(node);
    int balance = getBalance(node);

    // Left Heavy
    if (balance > 1) {
        if (getBalance(node->left) < 0) {
            node->left = rotateLeft(node->left); // LR case
        }
        return rotateRight(node); // LL case
    }
    // Right Heavy
    if (balance < -1) {
        if (getBalance(node->right) > 0) {
            node->right = rotateRight(node->right); // RL case
        }
        return rotateLeft(node); // RR case
    }
    return node;
}

int AVLIndex::compareKeys(double numA, const string& strA, AVLNode* node) const {
    if (numeric) {
        if (fabs(numA - node->numKey) < 1e-6) return 0;
        return (numA < node->numKey) ? -1 : 1;
    } else {
        if (strA == node->strKey) return 0;
        return (strA < node->strKey) ? -1 : 1;
    }
}

AVLIndex::AVLNode* AVLIndex::insertNode(AVLNode* node, double nKey, const string& sKey,
                                        const string& disp, int recIdx) {
    if (!node) {
        nodeCount++;
        return new AVLNode(nKey, sKey, disp, recIdx);
    }

    int cmp = compareKeys(nKey, sKey, node);
    if (cmp < 0) {
        node->left = insertNode(node->left, nKey, sKey, disp, recIdx);
    } else if (cmp > 0) {
        node->right = insertNode(node->right, nKey, sKey, disp, recIdx);
    } else {
        // Duplicate field value: store alongside existing record indices in this node
        node->recordIndices.push_back(recIdx);
        return node;
    }

    return rebalance(node);
}

AVLIndex::AVLNode* AVLIndex::findMinNode(AVLNode* node) const {
    AVLNode* curr = node;
    while (curr && curr->left != nullptr) {
        curr = curr->left;
    }
    return curr;
}

AVLIndex::AVLNode* AVLIndex::removeNode(AVLNode* node, double nKey, const string& sKey, int recIdx) {
    if (!node) return nullptr;

    int cmp = compareKeys(nKey, sKey, node);
    if (cmp < 0) {
        node->left = removeNode(node->left, nKey, sKey, recIdx);
    } else if (cmp > 0) {
        node->right = removeNode(node->right, nKey, sKey, recIdx);
    } else {
        // Found matching key: remove only the specific recordIndex
        for (size_t i = 0; i < node->recordIndices.size(); ++i) {
            if (node->recordIndices[i] == recIdx) {
                node->recordIndices.erase(node->recordIndices.begin() + i);
                break;
            }
        }
        // If other records still share this key, keep the AVL node intact
        if (!node->recordIndices.empty()) {
            return node;
        }

        // Otherwise, delete this node from the AVL tree
        if (!node->left || !node->right) {
            AVLNode* temp = node->left ? node->left : node->right;
            delete node;
            nodeCount--;
            return temp;
        } else {
            // Two children: replace with in-order successor's key and payload
            AVLNode* successor = findMinNode(node->right);
            node->numKey = successor->numKey;
            node->strKey = successor->strKey;
            node->displayKey = successor->displayKey;
            node->recordIndices = successor->recordIndices;

            // Remove the successor node completely (clear its vector first so it deletes)
            successor->recordIndices.clear();
            node->right = removeNode(node->right, successor->numKey, successor->strKey, -1);
        }
    }

    return rebalance(node);
}

void AVLIndex::destroyTree(AVLNode* node) {
    if (!node) return;
    destroyTree(node->left);
    destroyTree(node->right);
    delete node;
}

void AVLIndex::insert(const Record& record, int recordIndex) {
    double nKey = numeric ? record.getNumericField(fieldName) : 0.0;
    string disp = record.getStringField(fieldName);
    string sKey = toLowerStr(disp);
    root = insertNode(root, nKey, sKey, disp, recordIndex);
}

void AVLIndex::remove(const Record& record, int recordIndex) {
    double nKey = numeric ? record.getNumericField(fieldName) : 0.0;
    string disp = record.getStringField(fieldName);
    string sKey = toLowerStr(disp);
    root = removeNode(root, nKey, sKey, recordIndex);
}

void AVLIndex::clear() {
    destroyTree(root);
    root = nullptr;
    nodeCount = 0;
}

vector<int> AVLIndex::searchExact(const string& value) const {
    double nKey = 0.0;
    if (numeric) {
        try {
            nKey = stod(value);
        } catch (...) {
            return {};
        }
    }
    string sKey = toLowerStr(value);

    AVLNode* curr = root;
    while (curr != nullptr) {
        int cmp = compareKeys(nKey, sKey, curr);
        if (cmp == 0) return curr->recordIndices;
        curr = (cmp < 0) ? curr->left : curr->right;
    }
    return {};
}

void AVLIndex::rangeHelper(AVLNode* node, double minVal, double maxVal, vector<int>& result) const {
    if (!node) return;
    // Prune left subtree if current key is already <= minVal
    if (node->numKey > minVal) {
        rangeHelper(node->left, minVal, maxVal, result);
    }
    if (node->numKey >= minVal && node->numKey <= maxVal) {
        for (int idx : node->recordIndices) {
            result.push_back(idx);
        }
    }
    // Prune right subtree if current key is already >= maxVal
    if (node->numKey < maxVal) {
        rangeHelper(node->right, minVal, maxVal, result);
    }
}

vector<int> AVLIndex::searchRange(double minVal, double maxVal) const {
    vector<int> result;
    if (!numeric) return result;
    rangeHelper(root, minVal, maxVal, result);
    return result;
}

void AVLIndex::inOrderHelper(AVLNode* node, vector<int>& result) const {
    if (!node) return;
    inOrderHelper(node->left, result);
    for (int idx : node->recordIndices) {
        result.push_back(idx);
    }
    inOrderHelper(node->right, result);
}

void AVLIndex::reverseInOrderHelper(AVLNode* node, vector<int>& result) const {
    if (!node) return;
    reverseInOrderHelper(node->right, result);
    for (int idx : node->recordIndices) {
        result.push_back(idx);
    }
    reverseInOrderHelper(node->left, result);
}

vector<int> AVLIndex::getAllSorted(bool ascending) const {
    vector<int> result;
    if (ascending) {
        inOrderHelper(root, result);
    } else {
        reverseInOrderHelper(root, result);
    }
    return result;
}

vector<int> AVLIndex::getMin() const {
    if (!root) return {};
    AVLNode* curr = root;
    while (curr->left) curr = curr->left;
    return curr->recordIndices;
}

vector<int> AVLIndex::getMax() const {
    if (!root) return {};
    AVLNode* curr = root;
    while (curr->right) curr = curr->right;
    return curr->recordIndices;
}

void AVLIndex::topKHelper(AVLNode* node, int k, bool highest, vector<int>& result) const {
    if (!node || static_cast<int>(result.size()) >= k) return;

    if (highest) {
        topKHelper(node->right, k, highest, result);
        for (int idx : node->recordIndices) {
            if (static_cast<int>(result.size()) >= k) return;
            result.push_back(idx);
        }
        topKHelper(node->left, k, highest, result);
    } else {
        topKHelper(node->left, k, highest, result);
        for (int idx : node->recordIndices) {
            if (static_cast<int>(result.size()) >= k) return;
            result.push_back(idx);
        }
        topKHelper(node->right, k, highest, result);
    }
}

vector<int> AVLIndex::getTopK(int k, bool highest) const {
    vector<int> result;
    if (k <= 0) return result;
    topKHelper(root, k, highest, result);
    return result;
}

void AVLIndex::printTreeHelper(AVLNode* node, const string& prefix, bool isLeft) const {
    if (!node) return;
    cout << prefix << (isLeft ? "|-- " : "\\-- ");
    if (numeric) {
        cout << node->numKey;
    } else {
        cout << node->displayKey;
    }
    cout << " (count=" << node->recordIndices.size() << ", h=" << node->height << ")\n";

    string childPrefix = prefix + (isLeft ? "|   " : "    ");
    if (node->left || node->right) {
        if (node->left) {
            printTreeHelper(node->left, childPrefix, true);
        } else {
            cout << childPrefix << "|-- [null]\n";
        }
        if (node->right) {
            printTreeHelper(node->right, childPrefix, false);
        } else {
            cout << childPrefix << "\\-- [null]\n";
        }
    }
}

void AVLIndex::printStats() const {
    cout << "  [AVLIndex on '" << fieldName << "'] "
         << "Distinct Nodes: " << nodeCount
         << " | Tree Height: " << getHeight(root) << "\n";
    if (nodeCount == 0) {
        cout << "    (Empty tree)\n";
    } else if (nodeCount <= 25) {
        cout << "    Tree Structure:\n";
        printTreeHelper(root, "    ", false);
    } else {
        cout << "    (Tree has " << nodeCount
             << " nodes; structure display suppressed for trees > 25 nodes to fit screen)\n";
    }
}