#include "Database.h"
#include "HashIndex.h"
#include "AVLIndex.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cctype>

using namespace std;

Database::Database(const string& filePath) : dataFilePath(filePath) {}

Database::~Database() {
    for (Index* idx : indexes) {
        delete idx;
    }
    indexes.clear();
}

int Database::findRecordPosById(int id) const {
    // Use an index on "id" if one exists for O(1) or O(log N) lookup
    Index* idIdx = findExactIndex("id");
    if (idIdx != nullptr) {
        vector<int> matches = idIdx->searchExact(to_string(id));
        if (!matches.empty()) return matches[0];
        return -1;
    }
    for (size_t i = 0; i < records.size(); ++i) {
        if (records[i].id == id) return static_cast<int>(i);
    }
    return -1;
}

void Database::rebuildAllIndexes() {
    for (Index* idx : indexes) {
        idx->clear();
        for (int i = 0; i < static_cast<int>(records.size()); ++i) {
            idx->insert(records[i], i);
        }
    }
}

Index* Database::findExactIndex(const string& field) const {
    Index* fallbackAvl = nullptr;
    for (Index* idx : indexes) {
        if (idx->getFieldName() == field) {
            if (idx->getType() == "HASH") return idx; // Hash is preferred for exact match
            if (idx->getType() == "AVL") fallbackAvl = idx;
        }
    }
    return fallbackAvl;
}

Index* Database::findRangeOrderIndex(const string& field) const {
    for (Index* idx : indexes) {
        if (idx->getFieldName() == field && idx->supportsRangeAndOrder()) {
            return idx;
        }
    }
    return nullptr;
}

bool Database::loadFromFile() {
    ifstream inFile(dataFilePath);
    if (!inFile.is_open()) {
        return false;
    }

    records.clear();
    string line;
    int lineNum = 0;

    while (getline(inFile, line)) {
        lineNum++;
        if (line.empty() || line[0] == '#') continue; // Skip blank lines and comment headers

        // format: id|title|director|genre|year|rating
        stringstream ss(line);
        string idStr, title, director, genre, yearStr, ratingStr;

        if (!getline(ss, idStr, '|') ||
            !getline(ss, title, '|') ||
            !getline(ss, director, '|') ||
            !getline(ss, genre, '|') ||
            !getline(ss, yearStr, '|') ||
            !getline(ss, ratingStr, '|')) {
            continue;
        }

        try {
            int id = stoi(idStr);
            int year = stoi(yearStr);
            double rating = stod(ratingStr);
            if (findRecordPosById(id) == -1) {
                records.push_back(Record(id, title, director, genre, year, rating));
            }
        } catch (...) {
            cerr << "Warning: Skipping malformed record at line " << lineNum << "\n";
        }
    }

    inFile.close();
    rebuildAllIndexes();
    return true;
}

bool Database::saveToFile() const {
    ofstream outFile(dataFilePath);
    if (!outFile.is_open()) {
        return false;
    }

    outFile << "# id|title|director|genre|year|rating\n";
    for (const Record& r : records) {
        outFile << r.id << "|"
                << r.title << "|"
                << r.director << "|"
                << r.genre << "|"
                << r.year << "|"
                << fixed << setprecision(1) << r.rating << "\n";
    }
    outFile.close();
    return true;
}

bool Database::addRecord(const Record& rec, string& errorMsg) {
    if (findRecordPosById(rec.id) != -1) {
        errorMsg = "Duplicate ID: A record with ID " + to_string(rec.id) + " already exists.";
        return false;
    }
    if (rec.title.find('|') != string::npos ||
        rec.director.find('|') != string::npos ||
        rec.genre.find('|') != string::npos) {
        errorMsg = "Text fields cannot contain the '|' delimiter character.";
        return false;
    }

    int newPos = static_cast<int>(records.size());
    records.push_back(rec);

    // Keep all active indexes synchronized (F6)
    for (Index* idx : indexes) {
        idx->insert(rec, newPos);
    }
    return true;
}

bool Database::updateRecord(int id, const string& field, const string& newValue, string& errorMsg) {
    int pos = findRecordPosById(id);
    if (pos == -1) {
        errorMsg = "Record with ID " + to_string(id) + " not found.";
        return false;
    }
    if (!Record::isValidField(field)) {
        errorMsg = "Invalid field name '" + field + "'. Valid fields: id, title, director, genre, year, rating.";
        return false;
    }
    if (field == "id") {
        errorMsg = "Updating primary key 'id' is prohibited to preserve referential integrity.";
        return false;
    }
    if (newValue.find('|') != string::npos) {
        errorMsg = "Field values cannot contain the '|' delimiter character.";
        return false;
    }

    Record oldCopy = records[pos];

    // Validate and apply new field value
    try {
        if (field == "title") records[pos].title = newValue;
        else if (field == "director") records[pos].director = newValue;
        else if (field == "genre") records[pos].genre = newValue;
        else if (field == "year") records[pos].year = stoi(newValue);
        else if (field == "rating") records[pos].rating = stod(newValue);
    } catch (...) {
        errorMsg = "Invalid numeric value '" + newValue + "' for field '" + field + "'.";
        return false;
    }

    // Update any index built on the modified field (F6)
    for (Index* idx : indexes) {
        if (idx->getFieldName() == field) {
            idx->remove(oldCopy, pos);
            idx->insert(records[pos], pos);
        }
    }
    return true;
}

bool Database::deleteRecord(int id, string& errorMsg) {
    int pos = findRecordPosById(id);
    if (pos == -1) {
        errorMsg = "Record with ID " + to_string(id) + " not found.";
        return false;
    }

    // Swap-with-last removal keeps vector removal O(1) while only updating the moved record's index entry
    int lastPos = static_cast<int>(records.size()) - 1;
    Record deletedRec = records[pos];

    for (Index* idx : indexes) {
        idx->remove(deletedRec, pos);
    }

    if (pos != lastPos) {
        Record movedRec = records[lastPos];
        for (Index* idx : indexes) {
            idx->remove(movedRec, lastPos);
            idx->insert(movedRec, pos);
        }
        records[pos] = movedRec;
    }

    records.pop_back();
    return true;
}

bool Database::createIndex(const string& type, const string& field, string& errorMsg) {
    if (!Record::isValidField(field)) {
        errorMsg = "Invalid field '" + field + "'. Valid fields: id, title, director, genre, year, rating.";
        return false;
    }
    if (type != "HASH" && type != "AVL") {
        errorMsg = "Invalid index type '" + type + "'. Must be HASH or AVL.";
        return false;
    }

    // Prevent duplicate index of the same type on the same field
    for (Index* idx : indexes) {
        if (idx->getType() == type && idx->getFieldName() == field) {
            errorMsg = "A " + type + " index already exists on field '" + field + "'.";
            return false;
        }
    }

    Index* newIndex = nullptr;
    if (type == "HASH") {
        newIndex = new HashIndex(field);
    } else {
        newIndex = new AVLIndex(field);
    }

    for (int i = 0; i < static_cast<int>(records.size()); ++i) {
        newIndex->insert(records[i], i);
    }

    indexes.push_back(newIndex);
    return true;
}

bool Database::dropIndex(const string& type, const string& field, string& errorMsg) {
    for (size_t i = 0; i < indexes.size(); ++i) {
        if (indexes[i]->getType() == type && indexes[i]->getFieldName() == field) {
            delete indexes[i];
            indexes.erase(indexes.begin() + i);
            return true;
        }
    }
    errorMsg = "No " + type + " index found on field '" + field + "'.";
    return false;
}

void Database::listIndexes() const {
    if (indexes.empty()) {
        cout << "No active indexes.\n";
        return;
    }
    cout << "Active Indexes (" << indexes.size() << "):\n";
    for (const Index* idx : indexes) {
        idx->printStats();
    }
}

void Database::inspectIndex(const string& type, const string& field) const {
    for (const Index* idx : indexes) {
        if (idx->getType() == type && idx->getFieldName() == field) {
            idx->printStats();
            return;
        }
    }
    cout << "Error: No " << type + " index exists on field '" << field << "'.\n";
}

// Merge Sort 

int Database::compareRecordsByField(int idxA, int idxB, const string& field) const {
    if (Record::isNumericField(field)) {
        double a = records[idxA].getNumericField(field);
        double b = records[idxB].getNumericField(field);
        if (fabs(a - b) < 1e-6) return 0;
        return (a < b) ? -1 : 1;
    } else {
        string a = records[idxA].getStringField(field);
        string b = records[idxB].getStringField(field);
        for (size_t i = 0; i < a.size(); ++i) a[i] = static_cast<char>(tolower(static_cast<unsigned char>(a[i])));
        for (size_t i = 0; i < b.size(); ++i) b[i] = static_cast<char>(tolower(static_cast<unsigned char>(b[i])));
        if (a == b) return 0;
        return (a < b) ? -1 : 1;
    }
}

void Database::merge(vector<int>& indices, int left, int mid, int right,
                     const string& field, bool ascending) const {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    vector<int> L(n1), R(n2);
    for (int i = 0; i < n1; ++i) L[i] = indices[left + i];
    for (int j = 0; j < n2; ++j) R[j] = indices[mid + 1 + j];

    int i = 0, j = 0, k = left;
    while (i < n1 && j < n2) {
        int cmp = compareRecordsByField(L[i], R[j], field);
        bool takeLeft = ascending ? (cmp <= 0) : (cmp >= 0);
        if (takeLeft) {
            indices[k++] = L[i++];
        } else {
            indices[k++] = R[j++];
        }
    }
    while (i < n1) indices[k++] = L[i++];
    while (j < n2) indices[k++] = R[j++];
}

void Database::mergeSort(vector<int>& indices, const string& field, bool ascending) const {
    int n = static_cast<int>(indices.size());
    if (n <= 1) return;

    // Iterative bottom-up merge sort avoids deep recursion stacks
    for (int currSize = 1; currSize < n; currSize *= 2) {
        for (int leftStart = 0; leftStart < n - 1; leftStart += 2 * currSize) {
            int mid = min(leftStart + currSize - 1, n - 1);
            int rightEnd = min(leftStart + 2 * currSize - 1, n - 1);
            merge(indices, leftStart, mid, rightEnd, field, ascending);
        }
    }
}

bool Database::matchesCondition(const Record& rec, const string& field,
                                const string& op, const string& value) const {
    if (Record::isNumericField(field)) {
        double recVal = rec.getNumericField(field);
        double targetVal = 0.0;
        try {
            targetVal = stod(value);
        } catch (...) {
            return false;
        }
        if (op == "=") return fabs(recVal - targetVal) < 1e-6;
        if (op == "!=") return fabs(recVal - targetVal) >= 1e-6;
        if (op == "<") return recVal < targetVal;
        if (op == "<=") return recVal <= targetVal;
        if (op == ">") return recVal > targetVal;
        if (op == ">=") return recVal >= targetVal;
        return false;
    } else {
        string recStr = rec.getStringField(field);
        string valStr = value;
        for (size_t i = 0; i < recStr.size(); ++i) recStr[i] = static_cast<char>(tolower(static_cast<unsigned char>(recStr[i])));
        for (size_t i = 0; i < valStr.size(); ++i) valStr[i] = static_cast<char>(tolower(static_cast<unsigned char>(valStr[i])));

        if (op == "=") return recStr == valStr;
        if (op == "!=") return recStr != valStr;
        if (op == "<") return recStr < valStr;
        if (op == "<=") return recStr <= valStr;
        if (op == ">") return recStr > valStr;
        if (op == ">=") return recStr >= valStr;
        return false;
    }
}

void Database::printRecordsTable(const vector<int>& indices) const {
    if (indices.empty()) {
        cout << "  (0 matching records found)\n";
        return;
    }

    cout << "  " << left
         << setw(6) << "ID"
         << setw(36) << "Title"
         << setw(24) << "Director"
         << setw(14) << "Genre"
         << setw(8) << "Year"
         << setw(8) << "Rating" << "\n";
    cout << "  " << string(96, '-') << "\n";

    for (int idx : indices) {
        const Record& r = records[idx];
        string dispTitle = (r.title.size() > 34) ? (r.title.substr(0, 31) + "...") : r.title;
        string dispDir = (r.director.size() > 22) ? (r.director.substr(0, 19) + "...") : r.director;

        cout << "  " << left
             << setw(6) << r.id
             << setw(36) << dispTitle
             << setw(24) << dispDir
             << setw(14) << r.genre
             << setw(8) << r.year
             << fixed << setprecision(1) << setw(8) << r.rating << "\n";
    }
    cout << "  Total returned: " << indices.size() << " record(s).\n";
}

// --- Query Execution Methods ---

void Database::queryExact(const string& field, const string& value) const {
    if (!Record::isValidField(field)) {
        cout << "Error: Invalid field '" << field << "'.\n";
        return;
    }

    auto start = chrono::high_resolution_clock::now();
    vector<int> result;
    string methodUsed;

    Index* idx = findExactIndex(field);
    if (idx != nullptr) {
        result = idx->searchExact(value);
        methodUsed = idx->getType() + " Index on '" + field + "'";
    } else {
        for (int i = 0; i < static_cast<int>(records.size()); ++i) {
            if (matchesCondition(records[i], field, "=", value)) {
                result.push_back(i);
            }
        }
        methodUsed = "Full Table Scan (no index on '" + field + "')";
    }

    auto end = chrono::high_resolution_clock::now();
    long long us = chrono::duration_cast<chrono::microseconds>(end - start).count();

    cout << "[Method: " << methodUsed << " | Time: " << us << " us]\n";
    printRecordsTable(result);
}

void Database::queryRange(const string& field, double minVal, double maxVal) const {
    if (!Record::isValidField(field) || !Record::isNumericField(field)) {
        cout << "Error: Range queries require a valid numeric field (id, year, rating).\n";
        return;
    }
    if (minVal > maxVal) {
        cout << "Error: Minimum value (" << minVal << ") cannot exceed maximum value (" << maxVal << ").\n";
        return;
    }

    auto start = chrono::high_resolution_clock::now();
    vector<int> result;
    string methodUsed;

    Index* idx = findRangeOrderIndex(field);
    if (idx != nullptr) {
        result = idx->searchRange(minVal, maxVal);
        methodUsed = "AVL Index on '" + field + "'";
    } else {
        for (int i = 0; i < static_cast<int>(records.size()); ++i) {
            double val = records[i].getNumericField(field);
            if (val >= minVal && val <= maxVal) {
                result.push_back(i);
            }
        }
        methodUsed = "Full Table Scan (no AVL index on '" + field + "')";
    }

    auto end = chrono::high_resolution_clock::now();
    long long us = chrono::duration_cast<chrono::microseconds>(end - start).count();

    cout << "[Method: " << methodUsed << " | Time: " << us << " us]\n";
    printRecordsTable(result);
}

void Database::querySort(const string& field, bool ascending) const {
    if (!Record::isValidField(field)) {
        cout << "Error: Invalid field '" << field << "'.\n";
        return;
    }

    auto start = chrono::high_resolution_clock::now();
    vector<int> result;
    string methodUsed;

    Index* idx = findRangeOrderIndex(field);
    if (idx != nullptr) {
        result = idx->getAllSorted(ascending);
        methodUsed = "AVL Index in-order traversal on '" + field + "'";
    } else {
        result.resize(records.size());
        for (int i = 0; i < static_cast<int>(records.size()); ++i) result[i] = i;
        mergeSort(result, field, ascending);
        methodUsed = "Full Table Scan + Custom Merge Sort (no AVL index on '" + field + "')";
    }

    auto end = chrono::high_resolution_clock::now();
    long long us = chrono::duration_cast<chrono::microseconds>(end - start).count();

    cout << "[Method: " << methodUsed << " | Time: " << us << " us]\n";
    printRecordsTable(result);
}

void Database::queryMinMax(const string& field, bool isMax) const {
    if (!Record::isValidField(field)) {
        cout << "Error: Invalid field '" << field << "'.\n";
        return;
    }

    auto start = chrono::high_resolution_clock::now();
    vector<int> result;
    string methodUsed;

    Index* idx = findRangeOrderIndex(field);
    if (idx != nullptr) {
        result = isMax ? idx->getMax() : idx->getMin();
        methodUsed = "AVL Index extreme-node lookup on '" + field + "'";
    } else {
        if (!records.empty()) {
            int bestIdx = 0;
            for (int i = 1; i < static_cast<int>(records.size()); ++i) {
                int cmp = compareRecordsByField(i, bestIdx, field);
                if ((isMax && cmp > 0) || (!isMax && cmp < 0)) {
                    bestIdx = i;
                }
            }
            for (int i = 0; i < static_cast<int>(records.size()); ++i) {
                if (compareRecordsByField(i, bestIdx, field) == 0) {
                    result.push_back(i);
                }
            }
        }
        methodUsed = "Full Table Scan (no AVL index on '" + field + "')";
    }

    auto end = chrono::high_resolution_clock::now();
    long long us = chrono::duration_cast<chrono::microseconds>(end - start).count();

    cout << "[Method: " << methodUsed << " | Time: " << us << " us]\n";
    printRecordsTable(result);
}

void Database::queryTopK(const string& field, int k, bool highest) const {
    if (!Record::isValidField(field)) {
        cout << "Error: Invalid field '" << field << "'.\n";
        return;
    }
    if (k <= 0) {
        cout << "Error: k must be a positive integer.\n";
        return;
    }

    auto start = chrono::high_resolution_clock::now();
    vector<int> result;
    string methodUsed;

    Index* idx = findRangeOrderIndex(field);
    if (idx != nullptr) {
        result = idx->getTopK(k, highest);
        methodUsed = "AVL Index bounded traversal on '" + field + "'";
    } else {
        vector<int> all(records.size());
        for (int i = 0; i < static_cast<int>(records.size()); ++i) all[i] = i;
        mergeSort(all, field, !highest); // descending if highest, ascending if lowest
        int limit = min(k, static_cast<int>(all.size()));
        for (int i = 0; i < limit; ++i) result.push_back(all[i]);
        methodUsed = "Full Table Scan + Custom Merge Sort (no AVL index on '" + field + "')";
    }

    auto end = chrono::high_resolution_clock::now();
    long long us = chrono::duration_cast<chrono::microseconds>(end - start).count();

    cout << "[Method: " << methodUsed << " | Time: " << us << " us]\n";
    printRecordsTable(result);
}

void Database::queryAnd(const string& field1, const string& op1, const string& val1,
                        const string& field2, const string& op2, const string& val2) const {
    if (!Record::isValidField(field1) || !Record::isValidField(field2)) {
        cout << "Error: Invalid field name in compound query.\n";
        return;
    }

    auto start = chrono::high_resolution_clock::now();
    vector<int> candidates;
    string methodUsed;

    // Query Optimizer: use an index on condition 1 or condition 2 to narrow candidate set first
    Index* idx1 = (op1 == "=") ? findExactIndex(field1) : nullptr;
    Index* idx2 = (op2 == "=") ? findExactIndex(field2) : nullptr;

    if (idx1 != nullptr) {
        candidates = idx1->searchExact(val1);
        methodUsed = idx1->getType() + " Index on '" + field1 + "' + filter on '" + field2 + "'";
    } else if (idx2 != nullptr) {
        candidates = idx2->searchExact(val2);
        methodUsed = idx2->getType() + " Index on '" + field2 + "' + filter on '" + field1 + "'";
    } else {
        candidates.resize(records.size());
        for (int i = 0; i < static_cast<int>(records.size()); ++i) candidates[i] = i;
        methodUsed = "Full Table Scan (no suitable exact index on '" + field1 + "' or '" + field2 + "')";
    }

    vector<int> result;
    for (int idx : candidates) {
        if (matchesCondition(records[idx], field1, op1, val1) &&
            matchesCondition(records[idx], field2, op2, val2)) {
            result.push_back(idx);
        }
    }

    auto end = chrono::high_resolution_clock::now();
    long long us = chrono::duration_cast<chrono::microseconds>(end - start).count();

    cout << "[Method: " << methodUsed << " | Time: " << us << " us]\n";
    printRecordsTable(result);
}

void Database::queryAggregate(const string& targetNumericField,
                              const string& condField, const string& condOp,
                              const string& condVal) const {
    if (!Record::isValidField(targetNumericField) || !Record::isNumericField(targetNumericField)) {
        cout << "Error: Aggregation target '" << targetNumericField
             << "' must be a numeric field (id, year, rating).\n";
        return;
    }
    if (!Record::isValidField(condField)) {
        cout << "Error: Invalid condition field '" << condField << "'.\n";
        return;
    }

    auto start = chrono::high_resolution_clock::now();
    vector<int> candidates;
    string methodUsed;

    Index* idx = (condOp == "=") ? findExactIndex(condField) : nullptr;
    if (idx != nullptr) {
        candidates = idx->searchExact(condVal);
        methodUsed = idx->getType() + " Index on '" + condField + "'";
    } else {
        candidates.resize(records.size());
        for (int i = 0; i < static_cast<int>(records.size()); ++i) candidates[i] = i;
        methodUsed = "Full Table Scan (no exact index on '" + condField + "')";
    }

    int count = 0;
    double sum = 0.0;
    for (int pos : candidates) {
        if (matchesCondition(records[pos], condField, condOp, condVal)) {
            count++;
            sum += records[pos].getNumericField(targetNumericField);
        }
    }

    auto end = chrono::high_resolution_clock::now();
    long long us = chrono::duration_cast<chrono::microseconds>(end - start).count();

    cout << "[Method: " << methodUsed << " | Time: " << us << " us]\n";
    if (count == 0) {
        cout << "  Count: 0 | Average " << targetNumericField << ": N/A (no matching records)\n";
    } else {
        cout << "  Count: " << count
             << " | Average " << targetNumericField << ": "
             << fixed << setprecision(2) << (sum / count) << "\n";
    }
}

void Database::showAll(int limit) const {
    if (limit <= 0) limit = static_cast<int>(records.size());
    vector<int> indices;
    int count = min(limit, static_cast<int>(records.size()));
    for (int i = 0; i < count; ++i) indices.push_back(i);
    printRecordsTable(indices);
    if (count < static_cast<int>(records.size())) {
        cout << "  (Showing first " << count << " of " << records.size() << " total records)\n";
    }
}

void Database::runBenchmark(const string& field, const string& exactVal,
                            double rangeMin, double rangeMax) const {
    if (!Record::isValidField(field)) {
        cout << "Error: Invalid field '" << field << "'.\n";
        return;
    }

    const int ITERATIONS = 1000;
    cout << "\n=== Performance Benchmark (" << ITERATIONS << " iterations on "
         << records.size() << " records, field='" << field << "') ===\n";

    // 1. Exact Match: Full Scan vs. Hash Index vs. AVL Index
    auto t1 = chrono::high_resolution_clock::now();
    int scanHits = 0;
    for (int it = 0; it < ITERATIONS; ++it) {
        for (int i = 0; i < static_cast<int>(records.size()); ++i) {
            if (matchesCondition(records[i], field, "=", exactVal)) scanHits++;
        }
    }
    auto t2 = chrono::high_resolution_clock::now();
    long long scanExactUs = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();

    HashIndex tempHash(field);
    AVLIndex tempAvl(field);
    for (int i = 0; i < static_cast<int>(records.size()); ++i) {
        tempHash.insert(records[i], i);
        tempAvl.insert(records[i], i);
    }

    auto t3 = chrono::high_resolution_clock::now();
    int hashHits = 0;
    for (int it = 0; it < ITERATIONS; ++it) {
        hashHits += static_cast<int>(tempHash.searchExact(exactVal).size());
    }
    auto t4 = chrono::high_resolution_clock::now();
    long long hashExactUs = chrono::duration_cast<chrono::microseconds>(t4 - t3).count();

    auto t5 = chrono::high_resolution_clock::now();
    int avlHits = 0;
    for (int it = 0; it < ITERATIONS; ++it) {
        avlHits += static_cast<int>(tempAvl.searchExact(exactVal).size());
    }
    auto t6 = chrono::high_resolution_clock::now();
    long long avlExactUs = chrono::duration_cast<chrono::microseconds>(t6 - t5).count();

    cout << "  [Exact Match '" << exactVal << "'] (matches per run: " << (scanHits / ITERATIONS) << ")\n"
         << "    Full Table Scan : " << scanExactUs << " us\n"
         << "    Hash Index      : " << hashExactUs << " us  (Speedup: "
         << fixed << setprecision(1)
         << (hashExactUs > 0 ? static_cast<double>(scanExactUs) / hashExactUs : 0.0) << "x)\n"
         << "    AVL Index       : " << avlExactUs << " us  (Speedup: "
         << (avlExactUs > 0 ? static_cast<double>(scanExactUs) / avlExactUs : 0.0) << "x)\n";

    // 2. Range Query (if numeric)
    if (Record::isNumericField(field)) {
        auto r1 = chrono::high_resolution_clock::now();
        int rangeScanHits = 0;
        for (int it = 0; it < ITERATIONS; ++it) {
            for (int i = 0; i < static_cast<int>(records.size()); ++i) {
                double v = records[i].getNumericField(field);
                if (v >= rangeMin && v <= rangeMax) rangeScanHits++;
            }
        }
        auto r2 = chrono::high_resolution_clock::now();
        long long scanRangeUs = chrono::duration_cast<chrono::microseconds>(r2 - r1).count();

        auto r3 = chrono::high_resolution_clock::now();
        int rangeAvlHits = 0;
        for (int it = 0; it < ITERATIONS; ++it) {
            rangeAvlHits += static_cast<int>(tempAvl.searchRange(rangeMin, rangeMax).size());
        }
        auto r4 = chrono::high_resolution_clock::now();
        long long avlRangeUs = chrono::duration_cast<chrono::microseconds>(r4 - r3).count();

        cout << "  [Range Query " << rangeMin << ".." << rangeMax << "] (matches per run: "
             << (rangeScanHits / ITERATIONS) << ")\n"
             << "    Full Table Scan : " << scanRangeUs << " us\n"
             << "    AVL Index       : " << avlRangeUs << " us  (Speedup: "
             << (avlRangeUs > 0 ? static_cast<double>(scanRangeUs) / avlRangeUs : 0.0) << "x)\n";
    }
    cout << "====================================================================\n";
}