/*
================================================================================
CSCE 1101 - Fundamentals of Computing II
Assignment 3: Build Your Own Mini Database (Indexing with Hash Tables and AVL Trees)
================================================================================

1. TEAM MEMBER INFORMATION
   - Name: Hana Okasha | AUC ID: 900251938

2. DATASET DESCRIPTION & SOURCES (F1)
   - Topic: Curated International & Hollywood Cinema Catalog (`data/movies.txt`)
   - Size: 320 real-world movie records, (`id|title|director|genre|year|rating`).
   - Fields (6 total):
       1. `id`       (Numeric, Unique Primary Key: 101..420)
       2. `title`    (Text: Official English release title)
       3. `director` (Text: Primary film director)
       4. `genre`    (Text: Primary genre classification, e.g., Drama, Sci-Fi, Crime, Action)
       5. `year`     (Numeric: Theatrical release year, 1921..2024)
       6. `rating`   (Numeric: Public weighted rating on a 1.0 - 10.0 scale)
   - Methodology & Sources:
     Compiled directly from publicly available filmography records (IMDb Top/Popular lists
     and AFI catalogs) into a plain-text file (`data/movies.txt`). Pipe (`|`)
     delimiters were chosen so commas inside movie titles do not collide with field separators.

3. PROGRAM ARCHITECTURE & INDEX ADT DIAGRAM (F3)
   - Overview:
     `main.cpp` initializes `Database("data/movies.txt")` and `QueryEngine`. `Database`
     stores all `Record` structs in a contiguous `vector<Record>` and maintains a
     polymorphic list of `vector<Index*>`. Whenever a record is added, updated, or
     deleted, `Database` notifies every active `Index*` so all indexes remain strictly
     synchronized. During query execution, `Database` inspects active `Index*` objects
     via the `Index` ADT interface to pick the fastest access path (Hash -> AVL -> Scan)
     and reports both the chosen method and execution time in microseconds.

   - Class Diagram of the Index ADT:

                      +-----------------------------------+
                      |         <<abstract>>              |
                      |              Index                |
                      |-----------------------------------|
                      | # fieldName : string              |
                      |-----------------------------------|
                      | + getType() : string              |
                      | + supportsRangeAndOrder() : bool  |
                      | + insert(Record, int) : void      |
                      | + remove(Record, int) : void      |
                      | + searchExact(string) : vector    |
                      | + searchRange(min, max) : vector  |
                      | + getAllSorted(asc) : vector      |
                      | + getMin() / getMax() : vector    |
                      | + getTopK(k, highest) : vector    |
                      +-----------------------------------+
                                 ^               ^
                                 |               |
                 +---------------+               +---------------+
                 |                                               |
       +-------------------+                           +-------------------+
       |     HashIndex     |                           |     AVLIndex      |
       |-------------------|                           |-------------------|
       | - buckets         |                           | - root : AVLNode* |
       | - capacity        |                           | - nodeCount : int |
       | - numKeys         |                           | - numeric : bool  |
       |-------------------|                           |-------------------|
       | + O(1) exact      |                           | + O(log N) exact  |
       | + Auto-rehash     |                           | + Range & Sorted  |
       |   (load > 0.75)   |                           | + Min/Max/Top-K   |
       | + Duplicate keys  |                           | + LL/RR/LR/RL bal |
       +-------------------+                           +-------------------+

   - Handling Unsupported Operations in the ADT:
     Hash tables are unordered and cannot answer range or sorted queries without scanning
     and sorting all keys. `Index::supportsRangeAndOrder()` returns `false` on `HashIndex`
     and `true` on `AVLIndex`. `Database::findRangeOrderIndex()` checks this flag before
     dispatching range/sort/min/max/top-k queries, cleanly falling back to a full scan +
     custom iterative merge sort when no AVL index exists on the target field.

4. FEATURE MAP (F1 - F8 & Sample Command File)
   - F1 (Dataset):
       * `data/movies.txt` (320 records, 6 fields)
       * `Record.h` (`struct Record`, `isValidField`, `isNumericField`)
       * `Database.cpp` (`Database::loadFromFile`)
   - F2 (Record Management):
       * `Database.cpp` (`addRecord`, `updateRecord`, `deleteRecord`, `saveToFile`)
   - F3 (Index ADT):
       * `Index.h` (`class Index`)
   - F4 (Hash Index):
       * `HashIndex.h`, `HashIndex.cpp` (`hashFunction`, `resize`, `insert`, `remove`, `searchExact`)
   - F5 (AVL Index):
       * `AVLIndex.h`, `AVLIndex.cpp` (`rotateLeft`, `rotateRight`, `rebalance`, `searchRange`,
         `getAllSorted`, `getMin`, `getMax`, `getTopK`, `printStats`)
   - F6 (Index Management & Query Planner):
       * `Database.cpp` (`createIndex`, `dropIndex`, `listIndexes`, `findExactIndex`,
         `findRangeOrderIndex`, index synchronization in `addRecord`/`updateRecord`/`deleteRecord`)
   - F7 (Query Language & Batch Execution):
       * `QueryEngine.h`, `QueryEngine.cpp` (`tokenize`, `executeCommand`, `runScriptFile`, `printHelp`)
       * `Database.cpp` (`queryExact`, `queryRange`, `querySort`, `queryMinMax`, `queryTopK`,
         `queryAnd`, `queryAggregate`, `mergeSort`)
   - F8 (Console User Interface):
       * `QueryEngine.cpp` (`startInteractiveLoop`), `Database.cpp` (`printRecordsTable`)
   - Performance Measurement:
       * `Database.cpp` (`runBenchmark` and microsecond `chrono` timers in every query)
   - Sample Command File:
       * `commands.txt` (24 commands covering every feature and invalid command handling)

5. QUERY LANGUAGE REFERENCE (SYNTAX & EXAMPLES)
   - ADD <id> "<title>" "<director>" "<genre>" <year> <rating>
       Ex: ADD 999 "Inside Out 2" "Kelsey Mann" "Animation" 2024 9.5
   - UPDATE <id> <field> "<new_value>"
       Ex: UPDATE 999 rating 8.6
   - DELETE <id>
       Ex: DELETE 999
   - SAVE
       Ex: SAVE
   - SHOW [limit]
       Ex: SHOW 10
   - CREATE_INDEX <HASH|AVL> <field>
       Ex: CREATE_INDEX HASH genre
   - DROP_INDEX <HASH|AVL> <field>
       Ex: DROP_INDEX HASH genre
   - INDEXES
       Ex: INDEXES
   - INSPECT_INDEX <HASH|AVL> <field>
       Ex: INSPECT_INDEX AVL genre
   - FIND <field> = "<value>"
       Ex: FIND genre = "Drama"
   - RANGE <numeric_field> <min> <max>
       Ex: RANGE rating 8.8 9.5
   - SORT <field> <ASC|DESC>
       Ex: SORT year DESC
   - MIN <field>  |  MAX <field>
       Ex: MAX rating
   - TOP <k> <field>  |  BOTTOM <k> <field>
       Ex: TOP 5 rating
   - AND <field1> <op1> "<val1>" <field2> <op2> "<val2>"
       Ex: AND genre = "Sci-Fi" year >= 2010
   - STATS <numeric_field> WHERE <field> <op> "<value>"
       Ex: STATS rating WHERE director = "Christopher Nolan"
   - BENCHMARK <field> "<exact_val>" [range_min range_max]
       Ex: BENCHMARK year 1994 1990 2000
   - RUN <script_file_path>
       Ex: RUN commands.txt
   - HELP  |  EXIT

6. COMPILATION & EXECUTION INSTRUCTIONS
   - Compile (from the project root folder):
       g++ -std=c++17 -O2 -Wall -Wextra main.cpp HashIndex.cpp AVLIndex.cpp Database.cpp QueryEngine.cpp -o minidb
   - Run interactive console:
       ./minidb
   - Run directly with the sample command script:
       ./minidb commands.txt
   - Known Limitations:
     * Pipe characters (`|`) are forbidden inside text fields because `|` is the storage delimiter.
     * `RANGE` queries apply only to numeric fields (`id`, `year`, `rating`).
     * Tree structure ASCII rendering in `INSPECT_INDEX AVL <field>` displays full branches for
       trees up to 25 distinct nodes so large trees do not overflow standard console widths.

7. AI USAGE LOG
- Tool: Gemini
   - AI was used to help with path resolution and help generate the 320-record
     real-world movie dataset.
================================================================================
*/

#include "Database.h"
#include "QueryEngine.h"
#include <iostream>
#include <fstream>

using namespace std;

// Checks common relative locations so the program works from both project root and IDE build folders
string resolveDataPath() {
    const string candidates[] = {
        "data/movies.txt",
        "../data/movies.txt",
        "../../data/movies.txt"
    };
    for (const string& path : candidates) {
        ifstream test(path);
        if (test.is_open()) {
            return path;
        }
    }
    return "data/movies.txt";
}

int main(int argc, char* argv[]) {
    string dataPath = resolveDataPath();
    Database db(dataPath);

    cout << "====================================================\n";
    cout << "   Mini Database Engine (Hash Table & AVL Indexing) \n";
    cout << "====================================================\n";

    if (db.loadFromFile()) {
        cout << "Loaded " << db.getRecordCount() << " records from '" << dataPath << "'.\n";
    } else {
        cout << "Warning: Could not open '" << dataPath << "'. Starting with an empty database.\n";
    }

    QueryEngine engine(db);

    // Allow passing a batch command file as a CLI argument (e.g., ./minidb commands.txt)
    if (argc >= 2) {
        engine.runScriptFile(argv[1]);
    }

    engine.startInteractiveLoop();
    return 0;
}