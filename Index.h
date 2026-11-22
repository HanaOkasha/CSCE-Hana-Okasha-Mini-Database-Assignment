#ifndef RECORD_H
#define RECORD_H

#include <string>
#include <vector>

using namespace std;

/**
 * @class Record
 * @brief Represents a single row/tuple in the Mini Database.
 *
 * Responsibilities:
 * - Stores the 6 required fields for our Movies dataset (1 unique ID, 3 text fields,
 *   2 numeric fields).
 * - Provides uniform field extraction by field name (as strings or doubles) so that
 *   indexes and query evaluators can operate generically on any field without
 *   hardcoding field-specific logic.
 */
struct Record {
    int id;             // Unique primary key (Numeric)
    string title;       // Movie title (Text)
    string director;    // Director full name (Text)
    string genre;       // Primary genre (Text)
    int year;           // Release year (Numeric)
    double rating;      // IMDb/Audience rating out of 10.0 (Numeric)

    Record() : id(0), title(""), director(""), genre(""), year(0), rating(0.0) {}

    Record(int id, const string& title, const string& director,
           const string& genre, int year, double rating)
        : id(id), title(title), director(director), genre(genre), year(year), rating(rating) {}

    // Checks whether a given field name exists in the schema
    static bool isValidField(const string& field) {
        return field == "id" || field == "title" || field == "director" ||
               field == "genre" || field == "year" || field == "rating";
    }

    // Distinguishes numeric fields from text fields for range queries, sorting, and hashing
    static bool isNumericField(const string& field) {
        return field == "id" || field == "year" || field == "rating";
    }

    // Returns the normalized string representation of any field (used by HashIndex and text comparisons)
    string getStringField(const string& field) const {
        if (field == "id") return to_string(id);
        if (field == "title") return title;
        if (field == "director") return director;
        if (field == "genre") return genre;
        if (field == "year") return to_string(year);
        if (field == "rating") return to_string(rating);
        return "";
    }

    // Returns the numeric value of a numeric field (used by AVLIndex and aggregations)
    double getNumericField(const string& field) const {
        if (field == "id") return static_cast<double>(id);
        if (field == "year") return static_cast<double>(year);
        if (field == "rating") return rating;
        return 0.0;
    }
};

#endif // RECORD_H