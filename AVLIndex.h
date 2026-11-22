#ifndef AVLINDEX_H
#define AVLINDEX_H

#include "Index.h"
#include <string>
#include <vector>

using namespace std;

/**
 * @class AVLIndex
 * @brief Self-balancing Binary Search Tree (AVL Tree) implementing the Index ADT.
 *
 * Responsibilities:
 * - Maintains strict O(log N) height balance via LL, RR, LR, and RL rotations after every
 *   insertion and deletion.
 * - Supports both numeric fields (compared numerically) and text fields (compared
 *   case-insensitively lexicographically).
 * - Handles duplicate field values cleanly by storing a `vector<int>` of record
 *   positions inside each tree node, preventing degenerate chains on repeated keys.
 * - Provides fast range searches, ascending/descending sorted traversals, min/max lookups,
 *   top-k / bottom-k queries, and tree structure visualization.
 */
class AVLIndex : public Index {
private:
    struct AVLNode {
        double numKey;              // Used when field is numeric
        string strKey;              // Used when field is text (lowercased)
        string displayKey;          // Original text for readable tree display
        vector<int> recordIndices;  // All record indices sharing this key value
        AVLNode* left;
        AVLNode* right;
        int height;

        AVLNode(double nKey, const string& sKey, const string& disp, int recIdx)
            : numKey(nKey), strKey(sKey), displayKey(disp),
              left(nullptr), right(nullptr), height(1) {
            recordIndices.push_back(recIdx);
        }
    };

    AVLNode* root;
    int nodeCount;
    bool numeric;

    // Helper functions for height, balance factor, and rotations
    int getHeight(AVLNode* node) const;
    int getBalance(AVLNode* node) const;
    void updateHeight(AVLNode* node);

    AVLNode* rotateRight(AVLNode* y);
    AVLNode* rotateLeft(AVLNode* x);
    AVLNode* rebalance(AVLNode* node);

    // Key comparison helper: returns -1 if a < b, 0 if a == b, +1 if a > b
    int compareKeys(double numA, const string& strA, AVLNode* node) const;
    string toLowerStr(const string& s) const;

    // Recursive tree operations
    AVLNode* insertNode(AVLNode* node, double nKey, const string& sKey,
                        const string& disp, int recIdx);
    AVLNode* findMinNode(AVLNode* node) const;
    AVLNode* removeNode(AVLNode* node, double nKey, const string& sKey, int recIdx);
    void destroyTree(AVLNode* node);

    // Query traversal helpers
    void rangeHelper(AVLNode* node, double minVal, double maxVal, vector<int>& result) const;
    void inOrderHelper(AVLNode* node, vector<int>& result) const;
    void reverseInOrderHelper(AVLNode* node, vector<int>& result) const;
    void topKHelper(AVLNode* node, int k, bool highest, vector<int>& result) const;
    void printTreeHelper(AVLNode* node, const string& prefix, bool isLeft) const;

public:
    explicit AVLIndex(const string& field);
    ~AVLIndex() override;

    string getType() const override { return "AVL"; }
    bool supportsRangeAndOrder() const override { return true; }

    void insert(const Record& record, int recordIndex) override;
    void remove(const Record& record, int recordIndex) override;
    void clear() override;

    vector<int> searchExact(const string& value) const override;
    vector<int> searchRange(double minVal, double maxVal) const override;
    vector<int> getAllSorted(bool ascending) const override;
    vector<int> getMin() const override;
    vector<int> getMax() const override;
    vector<int> getTopK(int k, bool highest) const override;

    int getTreeHeight() const { return getHeight(root); }
    int getNodeCount() const { return nodeCount; }
    void printStats() const override;
};

#endif // AVLINDEX_H