#ifndef DATABASE_H
#define DATABASE_H

#include "Record.h"
#include "Index.h"
#include <string>
#include <vector>

using namespace std;

/**
 * @class Database
 * @brief Core storage engine, index manager, query planner, and performance benchmarker.
 *
 * Responsibilities:
 * - Loads and saves records from/to a (`|`) text file (F1, F2).
 * - Enforces unique Record IDs across all insertions and updates (F2).
 * - Manages multiple polymorphic `Index*` pointers (HashIndex and AVLIndex) purely through
 *   the `Index` ADT interface (F3, F6).
 * - Keeps all active indexes synchronized on every insert, update, and delete (F6).
 * - Chooses the optimal execution path (Hash Index, AVL Index, or Full Table Scan),
 *   reports the chosen method to the user, and measures execution time in microseconds (F6).
 * - Implements custom merge sort (since `std::sort` is prohibited) for fallback sorting.
 */
class Database {
private:
    vector<Record> records;
    vector<Index*> indexes; // Polymorphic collection of active indexes (F3, F6)
    string dataFilePath;

    // Helper: finds the vector position of a record by its unique ID (-1 if not found)
    int findRecordPosById(int id) const;

    // Helper: rebuilds all active indexes from scratch (used after loading from file)
    void rebuildAllIndexes();

    // Helper: selects best index for exact match (prefers HASH, falls back to AVL, else nullptr)
    Index* findExactIndex(const string& field) const;

    // Helper: selects an AVL index on `field` for range/order queries (else nullptr)
    Index* findRangeOrderIndex(const string& field) const;

    // Custom Merge Sort (replaces prohibited std::sort for unindexed ORDER BY / TOP-K fallback)
    void mergeSort(vector<int>& indices, const string& field, bool ascending) const;
    void merge(vector<int>& indices, int left, int mid, int right,
               const string& field, bool ascending) const;
    int compareRecordsByField(int idxA, int idxB, const string& field) const;

    // Helper: checks if a single record satisfies `field op value` (used in AND queries & AGGREGATE)
    bool matchesCondition(const Record& rec, const string& field,
                          const string& op, const string& value) const;

    // Pretty-prints a table of records in aligned columns
    void printRecordsTable(const vector<int>& indices) const;

public:
    explicit Database(const string& filePath);
    ~Database();

    // File Persistence (F1, F2)
    bool loadFromFile();
    bool saveToFile() const;

    // Record Management (F2)
    bool addRecord(const Record& rec, string& errorMsg);
    bool updateRecord(int id, const string& field, const string& newValue, string& errorMsg);
    bool deleteRecord(int id, string& errorMsg);

    // Index Management (F6)
    bool createIndex(const string& type, const string& field, string& errorMsg);
    bool dropIndex(const string& type, const string& field, string& errorMsg);
    void listIndexes() const;
    void inspectIndex(const string& type, const string& field) const;

    // Query Execution (F7)
    void queryExact(const string& field, const string& value) const;
    void queryRange(const string& field, double minVal, double maxVal) const;
    void querySort(const string& field, bool ascending) const;
    void queryMinMax(const string& field, bool isMax) const;
    void queryTopK(const string& field, int k, bool highest) const;
    void queryAnd(const string& field1, const string& op1, const string& val1,
                  const string& field2, const string& op2, const string& val2) const;
    void queryAggregate(const string& targetNumericField,
                        const string& condField, const string& condOp,
                        const string& condVal) const;
    void showAll(int limit = 20) const;

    // Performance Benchmark comparing Indexed vs. Full Scan search speed
    void runBenchmark(const string& field, const string& exactVal,
                      double rangeMin, double rangeMax) const;

    int getRecordCount() const { return static_cast<int>(records.size()); }
};

#endif // DATABASE_H