#include "HashIndex.h"
#include <iostream>
#include <cstdlib>
#include <cctype>
#include <cstdio>

using namespace std;

HashIndex::HashIndex(const string& field, int initialCapacity)
    : Index(field), capacity(initialCapacity), numKeys(0) {
    buckets.resize(capacity, nullptr);
}

HashIndex::~HashIndex() {
    clear();
}

// string hash: distributes both short numeric strings and long titles uniformly
size_t HashIndex::hashFunction(const string& key) const {
    size_t hash = 5381;
    for (char c : key) {
        // Case-insensitive hashing for text fields so "comedy" matches "Comedy"
        unsigned char uc = static_cast<unsigned char>(tolower(static_cast<unsigned char>(c)));
        hash = ((hash << 5) + hash) + uc; // hash * 33 + c
    }
    return hash % static_cast<size_t>(capacity);
}

// Ensures consistent key formatting for numeric fields (e.g., "1994" vs "1994.000000")
string HashIndex::normalizeKey(const string& rawKey) const {
    if (Record::isNumericField(fieldName)) {
        try {
            double val = stod(rawKey);
            char buf[64];
            snprintf(buf, sizeof(buf), "%.4f", val);
            return string(buf);
        } catch (...) {
            return rawKey;
        }
    }
    // Lowercase text keys for case-insensitive exact match
    string lower = rawKey;
    for (size_t i = 0; i < lower.size(); ++i) {
        lower[i] = static_cast<char>(tolower(static_cast<unsigned char>(lower[i])));
    }
    return lower;
}

void HashIndex::resize() {
    int oldCapacity = capacity;
    vector<HashNode*> oldBuckets = buckets;

    capacity *= 2;
    buckets.assign(capacity, nullptr);

    // Re-link existing nodes directly into new buckets without reallocating record vectors
    for (int i = 0; i < oldCapacity; ++i) {
        HashNode* curr = oldBuckets[i];
        while (curr != nullptr) {
            HashNode* nextNode = curr->next;
            size_t newIdx = hashFunction(curr->key);
            curr->next = buckets[newIdx];
            buckets[newIdx] = curr;
            curr = nextNode;
        }
    }
}

void HashIndex::insert(const Record& record, int recordIndex) {
    // Maintain load factor <= 0.75 to guarantee O(1) expected lookup time
    if (static_cast<double>(numKeys + 1) / capacity > 0.75) {
        resize();
    }

    string key = normalizeKey(record.getStringField(fieldName));
    size_t idx = hashFunction(key);

    HashNode* curr = buckets[idx];
    while (curr != nullptr) {
        if (curr->key == key) {
            // Key already exists: append this record's index to handle duplicate field values
            curr->recordIndices.push_back(recordIndex);
            return;
        }
        curr = curr->next;
    }

    // Key does not exist yet: prepend a new HashNode to the bucket chain
    HashNode* newNode = new HashNode(key, recordIndex);
    newNode->next = buckets[idx];
    buckets[idx] = newNode;
    numKeys++;
}

void HashIndex::remove(const Record& record, int recordIndex) {
    string key = normalizeKey(record.getStringField(fieldName));
    size_t idx = hashFunction(key);

    HashNode* curr = buckets[idx];
    HashNode* prev = nullptr;

    while (curr != nullptr) {
        if (curr->key == key) {
            // Remove matching recordIndex from the vector
            for (size_t i = 0; i < curr->recordIndices.size(); ++i) {
                if (curr->recordIndices[i] == recordIndex) {
                    curr->recordIndices.erase(curr->recordIndices.begin() + i);
                    break;
                }
            }
            // If no records remain with this key, unlink and free the HashNode
            if (curr->recordIndices.empty()) {
                if (prev == nullptr) {
                    buckets[idx] = curr->next;
                } else {
                    prev->next = curr->next;
                }
                delete curr;
                numKeys--;
            }
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

void HashIndex::clear() {
    for (int i = 0; i < capacity; ++i) {
        HashNode* curr = buckets[i];
        while (curr != nullptr) {
            HashNode* temp = curr;
            curr = curr->next;
            delete temp;
        }
        buckets[i] = nullptr;
    }
    numKeys = 0;
}

vector<int> HashIndex::searchExact(const string& value) const {
    string key = normalizeKey(value);
    size_t idx = hashFunction(key);

    HashNode* curr = buckets[idx];
    while (curr != nullptr) {
        if (curr->key == key) {
            return curr->recordIndices;
        }
        curr = curr->next;
    }
    return {};
}

void HashIndex::printStats() const {
    int nonEmpty = 0;
    int maxChain = 0;
    for (int i = 0; i < capacity; ++i) {
        int len = 0;
        HashNode* curr = buckets[i];
        while (curr != nullptr) {
            len++;
            curr = curr->next;
        }
        if (len > 0) nonEmpty++;
        if (len > maxChain) maxChain = len;
    }
    cout << "  [HashIndex on '" << fieldName << "'] "
         << "Distinct Keys: " << numKeys
         << " | Buckets: " << capacity
         << " | Load Factor: " << (static_cast<double>(numKeys) / capacity)
         << " | Max Chain Length: " << maxChain << "\n";
}