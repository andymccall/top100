// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/top100.h
// Purpose: Declarations for Top100 container and operations.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once

#include <vector>
#include <string>
#include "Movie.h"

// Forward declaration to avoid leaking sqlite3 header to dependents
struct sqlite3;

/** @defgroup core Core models and containers */

/**
 * @brief Sort orders for listing movies.
 * @ingroup core
 */
// Enum to define the sort order
enum class SortOrder {
    DEFAULT,
    BY_YEAR,
    ALPHABETICAL,
    BY_USER_RANK,    // 1..N ascending (unranked last)
    BY_USER_SCORE    // high to low
};

/**
 * @brief Persistent container for up to 100 movies, with ranking.
 *
 * Loads from and saves to a JSON file. Maintains an Elo-like userScore per
 * movie and exposes recomputeRanks() to derive 1-based userRank ordering.
 *
 * @ingroup core
 */
class Top100 {
public:
    Top100(const std::string& filename);
    ~Top100();

    /** @brief Add a movie; replaces existing title+year duplicates. */
    void addMovie(const Movie& movie);
    /** @brief Remove by title (first match). No-op if not found. */
    void removeMovie(const std::string& title);
    /** @brief Remove by IMDb ID (preferred precise delete). */
    bool removeByImdbId(const std::string& imdbID);
    /** @brief Return a copy of movies in the requested sort order. */
    std::vector<Movie> getMovies(SortOrder order = SortOrder::DEFAULT) const;

    // Duplicate handling helpers
    /** @return index or -1 if not found. */
    int findIndexByImdbId(const std::string& imdbID) const;      // returns index or -1
    /** @return index or -1 if not found. */
    int findIndexByTitleYear(const std::string& title, int year) const; // returns index or -1
    /** @brief Replace movie at index (bounds-checked internally). */
    void replaceMovie(size_t index, const Movie& movie);
    // Direct update access for ranking adjustments (bounds-checked)
    /** @brief Update movie at index; returns false if index invalid. */
    bool updateMovie(size_t index, const Movie& movie);

    /**
     * @brief Merge updated metadata into an existing movie by IMDb ID.
     * Copies all metadata fields from the provided OMDb-sourced movie into the
     * stored record identified by imdbID, while preserving userScore/userRank.
     * If the movie is not found, returns false. Saves to disk on success.
     */
    bool mergeFromOmdbByImdbId(const Movie& omdbMovie);

    // Recompute 1-based userRank from userScore (descending). Unranked (-1) if list empty.
    /** @brief Recompute 1-based userRank from userScore descending. */
    void recomputeRanks();

private:
    // Load all movies from the backing SQLite database (creating schema as needed)
    void load();
    // Persist in‑memory vector to SQLite (full sync; inexpensive for <=100 rows)
    void save();

    std::string filename;          // Path to SQLite database file (was JSON file)
    std::vector<Movie> movies;     // In‑memory working set (authoritative ordering = insertion)
    sqlite3* db = nullptr;         // Open database handle
};