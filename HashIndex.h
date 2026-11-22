#ifndef HASHINDEX_H
#define HASHINDEX_H

#include "Index.h"
#include <string>
#include <vector>

using namespace std;

/**
 * @class HashIndex
 * @brief Custom Separate-Chaining Hash Table implementing the Index ADT for O(1) exact matches.
 *
 * Responsibilities:
 * - Maps field values (both text and numeric, normalized to canonical keys) to lists of
 *   record positions (`recordIndex`) in the master table.
 * - Handles duplicate keys (e.g., dozens of movies sharing genre "Drama") by storing a
 *   `vector<int>` of record positions inside each HashNode.
 * - Automatically doubles capacity and rehashes all entries when the load factor exceeds 0.75.
 */
class HashIndex : public Index {
private:
    struct HashNode {
        string key;
        vector<int> recordIndices; // Stores all record positions sharing this key
        HashNode* next;

        HashNode(const string& k, int idx) : key(k), next(nullptr) {
            recordIndices.push_back(idx);
        }
    };

    vector<HashNode*> buckets;
    int capacity;
    int numKeys; // Number of distinct keys currently stored

    // Polynomial rolling hash (djb2 variant) for strings
    size_t hashFunction(const string& key) const;

    // Normalizes numeric keys (e.g., "9" and "9.0") so exact-match lookups always match
    string normalizeKey(const string& rawKey) const;

    // Doubles bucket array and re-inserts existing nodes when load factor > 0.75
    void resize();

public:
    explicit HashIndex(const string& field, int initialCapacity = 32);
    ~HashIndex() override;

    string getType() const override { return "HASH"; }
    bool supportsRangeAndOrder() const override { return false; }

    void insert(const Record& record, int recordIndex) override;
    void remove(const Record& record, int recordIndex) override;
    void clear() override;

    vector<int> searchExact(const string& value) const override;
    void printStats() const override;
};

#endif // HASHINDEX_H