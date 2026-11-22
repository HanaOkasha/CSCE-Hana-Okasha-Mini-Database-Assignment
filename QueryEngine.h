#ifndef QUERYENGINE_H
#define QUERYENGINE_H

#include "Database.h"
#include <string>
#include <vector>

using namespace std;

/**
 * @class QueryEngine
 * @brief Command-line parser, batch script runner, and interactive console interface.
 *
 * Responsibilities:
 * - Tokenizes user commands while respecting double-quoted multi-word strings
 *   (e.g., `"The Godfather"` or `"Christopher Nolan"`).
 * - Dispatches parsed commands to the `Database` engine.
 * - Executes batch script files line by line (`RUN <filepath>`) for automated testing.
 * - Handles invalid syntax, wrong argument counts, and non-numeric inputs gracefully
 *   with clear error messages and zero crashes or infinite loops (F7).
 */
class QueryEngine {
private:
    Database& db;

    // Splits a command line into tokens, keeping quoted strings together as single tokens
    vector<string> tokenize(const string& line) const;

    // Converts a command keyword to uppercase for case-insensitive command matching
    string toUpper(const string& s) const;

    // Prints complete syntax and example reference for all commands
    void printHelp() const;

public:
    explicit QueryEngine(Database& database);

    // Executes a single command string; returns false if the user entered EXIT/QUIT
    bool executeCommand(const string& rawLine);

    // Reads and runs all commands in a text file sequentially
    void runScriptFile(const string& filePath);

    // Starts the interactive Read-Eval-Print Loop (REPL)
    void startInteractiveLoop();
};

#endif // QUERYENGINE_H