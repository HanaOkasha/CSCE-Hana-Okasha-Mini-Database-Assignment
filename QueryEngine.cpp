#include "QueryEngine.h"
#include <iostream>
#include <fstream>
#include <cctype>

using namespace std;

QueryEngine::QueryEngine(Database& database) : db(database) {}

string QueryEngine::toUpper(const string& s) const {
    string out = s;
    for (size_t i = 0; i < out.size(); ++i) {
        out[i] = static_cast<char>(toupper(static_cast<unsigned char>(out[i])));
    }
    return out;
}

vector<string> QueryEngine::tokenize(const string& line) const {
    vector<string> tokens;
    string current;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (isspace(static_cast<unsigned char>(c)) && !inQuotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

void QueryEngine::printHelp() const {
    cout << "\n==================== MINI DATABASE QUERY LANGUAGE ====================\n"
         << "Note: Wrap multi-word values in double quotes, e.g., \"Sci-Fi\" or \"Inception\".\n"
         << "Available fields: id, title, director, genre, year, rating\n\n"
         << "1. RECORD MANAGEMENT:\n"
         << "   ADD <id> \"<title>\" \"<director>\" \"<genre>\" <year> <rating>\n"
         << "     Example: ADD 999 \"Dune: Part Two\" \"Denis Villeneuve\" \"Sci-Fi\" 2024 8.5\n"
         << "   UPDATE <id> <field> \"<new_value>\"\n"
         << "     Example: UPDATE 999 rating 8.6\n"
         << "   DELETE <id>\n"
         << "     Example: DELETE 999\n"
         << "   SAVE\n"
         << "     Example: SAVE\n"
         << "   SHOW [limit]\n"
         << "     Example: SHOW 10\n\n"
         << "2. INDEX MANAGEMENT:\n"
         << "   CREATE_INDEX <HASH|AVL> <field>\n"
         << "     Example: CREATE_INDEX HASH genre\n"
         << "   DROP_INDEX <HASH|AVL> <field>\n"
         << "     Example: DROP_INDEX HASH genre\n"
         << "   INDEXES\n"
         << "     Example: INDEXES\n"
         << "   INSPECT_INDEX <HASH|AVL> <field>\n"
         << "     Example: INSPECT_INDEX AVL rating\n\n"
         << "3. QUERIES:\n"
         << "   FIND <field> = \"<value>\"\n"
         << "     Example: FIND genre = \"Drama\"\n"
         << "   RANGE <numeric_field> <min> <max>\n"
         << "     Example: RANGE rating 8.5 9.3\n"
         << "   SORT <field> <ASC|DESC>\n"
         << "     Example: SORT rating DESC\n"
         << "   MIN <field>   |   MAX <field>\n"
         << "     Example: MAX rating\n"
         << "   TOP <k> <field>   |   BOTTOM <k> <field>\n"
         << "     Example: TOP 5 rating\n"
         << "   AND <field1> <op1> \"<val1>\" <field2> <op2> \"<val2>\"\n"
         << "     Example: AND genre = \"Sci-Fi\" year >= 2010\n"
         << "   STATS <numeric_field> WHERE <field> <op> \"<value>\"\n"
         << "     Example: STATS rating WHERE director = \"Christopher Nolan\"\n\n"
         << "4. BENCHMARKING, BATCH SCRIPTS & SYSTEM:\n"
         << "   BENCHMARK <field> \"<exact_val>\" [range_min range_max]\n"
         << "     Example: BENCHMARK year 1994 1990 2000\n"
         << "   RUN <script_file_path>\n"
         << "     Example: RUN commands.txt\n"
         << "   HELP\n"
         << "   EXIT\n"
         << "======================================================================\n";
}

bool QueryEngine::executeCommand(const string& rawLine) {
    // Strip leading/trailing whitespace and ignore comments
    size_t first = rawLine.find_first_not_of(" \t\r\n");
    if (first == string::npos || rawLine[first] == '#') {
        return true;
    }

    vector<string> tokens = tokenize(rawLine);
    if (tokens.empty()) return true;

    string cmd = toUpper(tokens[0]);

    if (cmd == "EXIT" || cmd == "QUIT") {
        cout << "Exiting Mini Database. Goodbye!\n";
        return false;
    }
    else if (cmd == "HELP") {
        printHelp();
    }
    else if (cmd == "SHOW") {
        int limit = 20;
        if (tokens.size() >= 2) {
            try {
                limit = stoi(tokens[1]);
            } catch (...) {
                cout << "Error: SHOW limit must be an integer.\n";
                return true;
            }
        }
        db.showAll(limit);
    }
    else if (cmd == "SAVE") {
        if (db.saveToFile()) {
            cout << "Dataset saved successfully (" << db.getRecordCount() << " records).\n";
        } else {
            cout << "Error: Could not write to dataset file.\n";
        }
    }
    else if (cmd == "ADD") {
        if (tokens.size() != 7) {
            cout << "Syntax Error: ADD <id> \"<title>\" \"<director>\" \"<genre>\" <year> <rating>\n";
            return true;
        }
        try {
            int id = stoi(tokens[1]);
            int year = stoi(tokens[5]);
            double rating = stod(tokens[6]);
            Record r(id, tokens[2], tokens[3], tokens[4], year, rating);
            string err;
            if (db.addRecord(r, err)) {
                cout << "Record ID " << id << " added successfully.\n";
            } else {
                cout << "Error: " << err << "\n";
            }
        } catch (...) {
            cout << "Error: ID and year must be integers, and rating must be numeric.\n";
        }
    }
    else if (cmd == "UPDATE") {
        if (tokens.size() != 4) {
            cout << "Syntax Error: UPDATE <id> <field> \"<new_value>\"\n";
            return true;
        }
        try {
            int id = stoi(tokens[1]);
            string err;
            if (db.updateRecord(id, tokens[2], tokens[3], err)) {
                cout << "Record ID " << id << " updated successfully.\n";
            } else {
                cout << "Error: " << err << "\n";
            }
        } catch (...) {
            cout << "Error: Record ID must be an integer.\n";
        }
    }
    else if (cmd == "DELETE") {
        if (tokens.size() != 2) {
            cout << "Syntax Error: DELETE <id>\n";
            return true;
        }
        try {
            int id = stoi(tokens[1]);
            string err;
            if (db.deleteRecord(id, err)) {
                cout << "Record ID " << id << " deleted successfully.\n";
            } else {
                cout << "Error: " << err << "\n";
            }
        } catch (...) {
            cout << "Error: Record ID must be an integer.\n";
        }
    }
    else if (cmd == "CREATE_INDEX") {
        if (tokens.size() != 3) {
            cout << "Syntax Error: CREATE_INDEX <HASH|AVL> <field>\n";
            return true;
        }
        string err;
        string idxType = toUpper(tokens[1]);
        if (db.createIndex(idxType, tokens[2], err)) {
            cout << "Created " << idxType << " index on field '" << tokens[2] << "'.\n";
        } else {
            cout << "Error: " << err << "\n";
        }
    }
    else if (cmd == "DROP_INDEX") {
        if (tokens.size() != 3) {
            cout << "Syntax Error: DROP_INDEX <HASH|AVL> <field>\n";
            return true;
        }
        string err;
        string idxType = toUpper(tokens[1]);
        if (db.dropIndex(idxType, tokens[2], err)) {
            cout << "Dropped " << idxType << " index on field '" << tokens[2] << "'.\n";
        } else {
            cout << "Error: " << err << "\n";
        }
    }
    else if (cmd == "INDEXES") {
        db.listIndexes();
    }
    else if (cmd == "INSPECT_INDEX") {
        if (tokens.size() != 3) {
            cout << "Syntax Error: INSPECT_INDEX <HASH|AVL> <field>\n";
            return true;
        }
        db.inspectIndex(toUpper(tokens[1]), tokens[2]);
    }
    else if (cmd == "FIND") {
        if (tokens.size() != 4 || tokens[2] != "=") {
            cout << "Syntax Error: FIND <field> = \"<value>\"\n";
            return true;
        }
        db.queryExact(tokens[1], tokens[3]);
    }
    else if (cmd == "RANGE") {
        if (tokens.size() != 4) {
            cout << "Syntax Error: RANGE <numeric_field> <min> <max>\n";
            return true;
        }
        try {
            double minV = stod(tokens[2]);
            double maxV = stod(tokens[3]);
            db.queryRange(tokens[1], minV, maxV);
        } catch (...) {
            cout << "Error: Range bounds must be numeric.\n";
        }
    }
    else if (cmd == "SORT") {
        if (tokens.size() != 3) {
            cout << "Syntax Error: SORT <field> <ASC|DESC>\n";
            return true;
        }
        string dir = toUpper(tokens[2]);
        if (dir != "ASC" && dir != "DESC") {
            cout << "Error: Sort direction must be ASC or DESC.\n";
            return true;
        }
        db.querySort(tokens[1], dir == "ASC");
    }
    else if (cmd == "MIN" || cmd == "MAX") {
        if (tokens.size() != 2) {
            cout << "Syntax Error: " << cmd << " <field>\n";
            return true;
        }
        db.queryMinMax(tokens[1], cmd == "MAX");
    }
    else if (cmd == "TOP" || cmd == "BOTTOM") {
        if (tokens.size() != 3) {
            cout << "Syntax Error: " << cmd << " <k> <field>\n";
            return true;
        }
        try {
            int k = stoi(tokens[1]);
            db.queryTopK(tokens[2], k, cmd == "TOP");
        } catch (...) {
            cout << "Error: k must be an integer.\n";
        }
    }
    else if (cmd == "AND") {
        if (tokens.size() != 7) {
            cout << "Syntax Error: AND <field1> <op1> \"<val1>\" <field2> <op2> \"<val2>\"\n";
            return true;
        }
        db.queryAnd(tokens[1], tokens[2], tokens[3], tokens[4], tokens[5], tokens[6]);
    }
    else if (cmd == "STATS") {
        if (tokens.size() != 6 || toUpper(tokens[2]) != "WHERE") {
            cout << "Syntax Error: STATS <numeric_field> WHERE <field> <op> \"<value>\"\n";
            return true;
        }
        db.queryAggregate(tokens[1], tokens[3], tokens[4], tokens[5]);
    }
    else if (cmd == "BENCHMARK") {
        if (tokens.size() != 3 && tokens.size() != 5) {
            cout << "Syntax Error: BENCHMARK <field> \"<exact_val>\" [range_min range_max]\n";
            return true;
        }
        try {
            double rMin = (tokens.size() == 5) ? stod(tokens[3]) : 0.0;
            double rMax = (tokens.size() == 5) ? stod(tokens[4]) : 0.0;
            db.runBenchmark(tokens[1], tokens[2], rMin, rMax);
        } catch (...) {
            cout << "Error: Range bounds in BENCHMARK must be numeric.\n";
        }
    }
    else if (cmd == "RUN") {
        if (tokens.size() != 2) {
            cout << "Syntax Error: RUN <script_file_path>\n";
            return true;
        }
        runScriptFile(tokens[1]);
    }
    else {
        cout << "Unknown command '" << tokens[0] << "'. Type HELP for a list of valid commands.\n";
    }

    return true;
}

void QueryEngine::runScriptFile(const string& filePath) {
    ifstream script(filePath);
    if (!script.is_open()) {
        cout << "Error: Could not open command script file '" << filePath << "'.\n";
        return;
    }

    cout << "\n>>> Running batch script: " << filePath << " <<<\n";
    string line;
    int cmdNum = 0;

    while (getline(script, line)) {
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == string::npos || line[first] == '#') continue;

        cmdNum++;
        cout << "\n[" << cmdNum << "] > " << line << "\n";
        if (!executeCommand(line)) {
            break;
        }
    }
    cout << "\n>>> Finished executing batch script (" << cmdNum << " commands). <<<\n";
}

void QueryEngine::startInteractiveLoop() {
    cout << "Type HELP for available commands, or EXIT to quit.\n";
    string line;
    while (true) {
        cout << "\ndb> ";
        if (!getline(cin, line)) {
            break; // Clean exit on EOF (Ctrl+D / Ctrl+Z)
        }
        if (!executeCommand(line)) {
            break;
        }
    }
}