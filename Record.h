#ifndef RECORD_H
#define RECORD_H

#include <string>
#include <vector>

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
    int id;                 // Unique primary key (Numeric)
    std::string title;      // Movie title (Text)
    std::string director;   // Director full name (Text)
    std::string genre;      // Primary genre (Text)
    int year;               // Release year (Numeric)
    double rating;          // IMDb/Audience rating out of 10.0 (Numeric)

    Record() : id(0), title(""), director(""), genre(""), year(0), rating(0.0) {}

    Record(int id, const std::string& title, const std::string& director,
           const std::string& genre, int year, double rating)
        : id(id), title(title), director(director), genre(genre), year(year), rating(rating) {}

    // Checks whether a given field name exists in the schema
    static bool isValidField(const std::string& field) {
        return field == "id" || field == "title" || field == "director" ||
               field == "genre" || field == "year" || field == "rating";
    }

    // Distinguishes numeric fields from text fields for range queries, sorting, and hashing
    static bool isNumericField(const std::string& field) {
        return field == "id" || field == "year" || field == "rating";
    }

    // Returns the normalized string representation of any field (used by HashIndex and text comparisons)
    std::string getStringField(const std::string& field) const {
        if (field == "id") return std::to_string(id);
        if (field == "title") return title;
        if (field == "director") return director;
        if (field == "genre") return genre;
        if (field == "year") return std::to_string(year);
        if (field == "rating") return std::to_string(rating);
        return "";
    }

    // Returns the numeric value of a numeric field (used by AVLIndex and aggregations)
    double getNumericField(const std::string& field) const {
        if (field == "id") return static_cast<double>(id);
        if (field == "year") return static_cast<double>(year);
        if (field == "rating") return rating;
        return 0.0;
    }
};

#endif // RECORD_H