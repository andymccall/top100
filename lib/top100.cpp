// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/top100.cpp
// Purpose: Core list persistence, sorting, and ranking recompute.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "top100.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp> // still used for Movie (de)serialization and JSON fallback
#ifndef TOP100_NO_SQLITE
#include <sqlite3.h>
#endif
#include <stdexcept>
#include <cstdio>

Top100::Top100(const std::string& filename) : filename(filename) {
    load();
}

Top100::~Top100() {
    try { save(); } catch(...) { /* swallow exceptions in destructor */ }
#ifndef TOP100_NO_SQLITE
    if (db) { sqlite3_close(db); db = nullptr; }
#endif
}

void Top100::addMovie(const Movie& movie) {
    movies.push_back(movie);
}

void Top100::removeMovie(const std::string& title) {
    movies.erase(std::remove_if(movies.begin(), movies.end(), [&](const Movie& movie) {
        return movie.title == title;
    }), movies.end());
}

bool Top100::removeByImdbId(const std::string& imdbID) {
    if (imdbID.empty()) return false;
    auto it = std::remove_if(movies.begin(), movies.end(), [&](const Movie& m){ return m.imdbID == imdbID; });
    if (it == movies.end()) return false;
    movies.erase(it, movies.end());
    return true;
}

std::vector<Movie> Top100::getMovies(SortOrder order) const {
    std::vector<Movie> sorted_movies = movies; // Create a copy to sort

    switch (order) {
        case SortOrder::BY_USER_RANK:
            std::sort(sorted_movies.begin(), sorted_movies.end(), [](const Movie& a, const Movie& b) {
                // Unranked go last
                if (a.userRank == -1 && b.userRank == -1) return a.title < b.title;
                if (a.userRank == -1) return false;
                if (b.userRank == -1) return true;
                return a.userRank < b.userRank;
            });
            break;
        case SortOrder::BY_USER_SCORE:
            std::sort(sorted_movies.begin(), sorted_movies.end(), [](const Movie& a, const Movie& b) {
                if (a.userScore == b.userScore) return a.title < b.title; // stable tie-breaker
                return a.userScore > b.userScore; // high to low
            });
            break;
        case SortOrder::BY_YEAR:
            std::sort(sorted_movies.begin(), sorted_movies.end(), [](const Movie& a, const Movie& b) {
                return a.year < b.year;
            });
            break;
        case SortOrder::ALPHABETICAL:
            std::sort(sorted_movies.begin(), sorted_movies.end(), [](const Movie& a, const Movie& b) {
                return a.title < b.title;
            });
            break;
        case SortOrder::DEFAULT:
        default:
            // Do nothing, return in insertion order
            break;
    }
    return sorted_movies;
}

void Top100::load() {
#ifndef TOP100_NO_SQLITE
    // Detect legacy JSON file (non-empty, not SQLite header, parses as JSON array) and migrate once.
    try {
        namespace fs = std::filesystem;
        if (fs::exists(filename) && fs::file_size(filename) > 0) {
            std::ifstream probe(filename, std::ios::binary);
            char hdr[16] = {0}; probe.read(hdr, 16); probe.close();
            bool isSqlite = std::string(hdr, 15) == "SQLite format 3"; // header is 15 chars + NUL
            if (!isSqlite) {
                std::ifstream jin(filename);
                nlohmann::json legacy = nlohmann::json::parse(jin, nullptr, false);
                if (!legacy.is_discarded() && legacy.is_array()) {
                    // Backup original
                    fs::path orig(filename); fs::path backup = orig; backup += ".legacy.json";
                    std::error_code ec; fs::rename(orig, backup, ec); // best-effort
                    sqlite3* mdb = nullptr;
                    if (sqlite3_open(filename.c_str(), &mdb) == SQLITE_OK) {
                        const char* createSql = R"SQL(
CREATE TABLE IF NOT EXISTS movies(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    title TEXT NOT NULL,
    year INTEGER NOT NULL,
    director TEXT NOT NULL,
    plotShort TEXT,
    plotFull TEXT,
    actors TEXT,
    genres TEXT,
    runtimeMinutes INTEGER,
    countries TEXT,
    posterUrl TEXT,
    imdbRating REAL,
    metascore INTEGER,
    rottenTomatoes INTEGER,
    source TEXT,
    imdbID TEXT UNIQUE,
    userScore REAL NOT NULL,
    userRank INTEGER
);)SQL";
                        sqlite3_exec(mdb, createSql, nullptr, nullptr, nullptr);
                        sqlite3_exec(mdb, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
                        const char* ins = "INSERT OR REPLACE INTO movies(title,year,director,plotShort,plotFull,actors,genres,runtimeMinutes,countries,posterUrl,imdbRating,metascore,rottenTomatoes,source,imdbID,userScore,userRank) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);";
                        sqlite3_stmt* stmt = nullptr;
                        if (sqlite3_prepare_v2(mdb, ins, -1, &stmt, nullptr) == SQLITE_OK) {
                            for (const auto& item : legacy) {
                                Movie m = item.get<Movie>();
                                int col = 1;
                                sqlite3_bind_text(stmt, col++, m.title.c_str(), -1, SQLITE_TRANSIENT);
                                sqlite3_bind_int(stmt, col++, m.year);
                                sqlite3_bind_text(stmt, col++, m.director.c_str(), -1, SQLITE_TRANSIENT);
                                sqlite3_bind_text(stmt, col++, m.plotShort.c_str(), -1, SQLITE_TRANSIENT);
                                sqlite3_bind_text(stmt, col++, m.plotFull.c_str(), -1, SQLITE_TRANSIENT);
                                nlohmann::json jActors = m.actors; auto sActors = jActors.dump(); sqlite3_bind_text(stmt, col++, sActors.c_str(), -1, SQLITE_TRANSIENT);
                                nlohmann::json jGenres = m.genres; auto sGenres = jGenres.dump(); sqlite3_bind_text(stmt, col++, sGenres.c_str(), -1, SQLITE_TRANSIENT);
                                sqlite3_bind_int(stmt, col++, m.runtimeMinutes);
                                nlohmann::json jCountries = m.countries; auto sCountries = jCountries.dump(); sqlite3_bind_text(stmt, col++, sCountries.c_str(), -1, SQLITE_TRANSIENT);
                                sqlite3_bind_text(stmt, col++, m.posterUrl.c_str(), -1, SQLITE_TRANSIENT);
                                sqlite3_bind_double(stmt, col++, m.imdbRating);
                                sqlite3_bind_int(stmt, col++, m.metascore);
                                sqlite3_bind_int(stmt, col++, m.rottenTomatoes);
                                sqlite3_bind_text(stmt, col++, m.source.c_str(), -1, SQLITE_TRANSIENT);
                                if (m.imdbID.empty()) sqlite3_bind_null(stmt, col++); else sqlite3_bind_text(stmt, col++, m.imdbID.c_str(), -1, SQLITE_TRANSIENT);
                                sqlite3_bind_double(stmt, col++, m.userScore);
                                if (m.userRank < 0) sqlite3_bind_null(stmt, col++); else sqlite3_bind_int(stmt, col++, m.userRank);
                                sqlite3_step(stmt); sqlite3_reset(stmt); sqlite3_clear_bindings(stmt);
                            }
                        }
                        if (stmt) sqlite3_finalize(stmt);
                        sqlite3_exec(mdb, "COMMIT;", nullptr, nullptr, nullptr);
                        sqlite3_close(mdb);
                    }
                }
            }
        }
    } catch (...) { /* ignore migration errors */ }
    // Open (possibly newly created) SQLite database
    if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open SQLite database: " + std::string(sqlite3_errmsg(db)));
    }
    // Pragmas: better concurrency & reasonable durability
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db, "PRAGMA foreign_keys=ON;", nullptr, nullptr, nullptr);
    const char* schemaSQL = R"SQL(
        CREATE TABLE IF NOT EXISTS movies(
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            year INTEGER NOT NULL,
            director TEXT NOT NULL,
            plotShort TEXT,
            plotFull TEXT,
            actors TEXT,
            genres TEXT,
            runtimeMinutes INTEGER,
            countries TEXT,
            posterUrl TEXT,
            imdbRating REAL,
            metascore INTEGER,
            rottenTomatoes INTEGER,
            source TEXT,
            imdbID TEXT UNIQUE,
            userScore REAL NOT NULL,
            userRank INTEGER
        );
    )SQL";
    char* errMsg = nullptr;
    if (sqlite3_exec(db, schemaSQL, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "unknown error";
        sqlite3_free(errMsg);
        throw std::runtime_error("Failed to create schema: " + msg);
    }
    const char* selectSQL = "SELECT title,year,director,plotShort,plotFull,actors,genres,runtimeMinutes,countries,posterUrl,imdbRating,metascore,rottenTomatoes,source,imdbID,userScore,userRank FROM movies ORDER BY id ASC";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, selectSQL, -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare select: " + std::string(sqlite3_errmsg(db)));
    }
    movies.clear();
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Movie m;
        m.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        m.year = sqlite3_column_int(stmt, 1);
        m.director = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        if (const unsigned char* t = sqlite3_column_text(stmt, 3)) m.plotShort = reinterpret_cast<const char*>(t);
        if (const unsigned char* t = sqlite3_column_text(stmt, 4)) m.plotFull = reinterpret_cast<const char*>(t);
        if (const unsigned char* t = sqlite3_column_text(stmt, 5)) { try { nlohmann::json j = nlohmann::json::parse(reinterpret_cast<const char*>(t)); m.actors = j.get<std::vector<std::string>>(); } catch(...) {} }
        if (const unsigned char* t = sqlite3_column_text(stmt, 6)) { try { nlohmann::json j = nlohmann::json::parse(reinterpret_cast<const char*>(t)); m.genres = j.get<std::vector<std::string>>(); } catch(...) {} }
        m.runtimeMinutes = sqlite3_column_int(stmt, 7);
        if (const unsigned char* t = sqlite3_column_text(stmt, 8)) { try { nlohmann::json j = nlohmann::json::parse(reinterpret_cast<const char*>(t)); m.countries = j.get<std::vector<std::string>>(); } catch(...) {} }
        if (const unsigned char* t = sqlite3_column_text(stmt, 9)) m.posterUrl = reinterpret_cast<const char*>(t);
        m.imdbRating = sqlite3_column_double(stmt, 10);
        m.metascore = sqlite3_column_int(stmt, 11);
        m.rottenTomatoes = sqlite3_column_int(stmt, 12);
        if (const unsigned char* t = sqlite3_column_text(stmt, 13)) m.source = reinterpret_cast<const char*>(t);
        if (const unsigned char* t = sqlite3_column_text(stmt, 14)) m.imdbID = reinterpret_cast<const char*>(t);
        m.userScore = sqlite3_column_double(stmt, 15);
        m.userRank = sqlite3_column_type(stmt, 16) == SQLITE_NULL ? -1 : sqlite3_column_int(stmt, 16);
        movies.push_back(std::move(m));
    }
    sqlite3_finalize(stmt);
#else
    // JSON fallback (development environments without SQLite headers)
    std::ifstream file(filename);
    if (file.is_open()) {
        nlohmann::json j; file >> j; if (j.is_array()) { movies = j.get<std::vector<Movie>>(); }
    }
#endif
}

void Top100::save() {
#ifndef TOP100_NO_SQLITE
    if (!db) return;
    char* errMsg = nullptr;
    if (sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, &errMsg) != SQLITE_OK) { if (errMsg) sqlite3_free(errMsg); return; }
    if (sqlite3_exec(db, "DELETE FROM movies;", nullptr, nullptr, &errMsg) != SQLITE_OK) { if (errMsg) sqlite3_free(errMsg); sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr); return; }
    const char* insertSQL = R"SQL(
        INSERT INTO movies(title,year,director,plotShort,plotFull,actors,genres,runtimeMinutes,countries,posterUrl,imdbRating,metascore,rottenTomatoes,source,imdbID,userScore,userRank)
        VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?);
    )SQL";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, insertSQL, -1, &stmt, nullptr) != SQLITE_OK) { sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr); return; }
    for (const auto& m : movies) {
        int col = 1;
        sqlite3_bind_text(stmt, col++, m.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, col++, m.year);
        sqlite3_bind_text(stmt, col++, m.director.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, m.plotShort.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, m.plotFull.c_str(), -1, SQLITE_TRANSIENT);
        nlohmann::json jActors = m.actors; auto sActors = jActors.dump(); sqlite3_bind_text(stmt, col++, sActors.c_str(), -1, SQLITE_TRANSIENT);
        nlohmann::json jGenres = m.genres; auto sGenres = jGenres.dump(); sqlite3_bind_text(stmt, col++, sGenres.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, col++, m.runtimeMinutes);
        nlohmann::json jCountries = m.countries; auto sCountries = jCountries.dump(); sqlite3_bind_text(stmt, col++, sCountries.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, col++, m.posterUrl.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, col++, m.imdbRating);
        sqlite3_bind_int(stmt, col++, m.metascore);
        sqlite3_bind_int(stmt, col++, m.rottenTomatoes);
        sqlite3_bind_text(stmt, col++, m.source.c_str(), -1, SQLITE_TRANSIENT);
        if (m.imdbID.empty()) sqlite3_bind_null(stmt, col++); else sqlite3_bind_text(stmt, col++, m.imdbID.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, col++, m.userScore);
        if (m.userRank < 0) sqlite3_bind_null(stmt, col++); else sqlite3_bind_int(stmt, col++, m.userRank);
        sqlite3_step(stmt); sqlite3_reset(stmt); sqlite3_clear_bindings(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
#else
    nlohmann::json j = movies;
    std::filesystem::path p(filename);
    if (p.has_parent_path()) { std::error_code ec; std::filesystem::create_directories(p.parent_path(), ec); }
    std::ofstream file(filename); file << j.dump(4);
#endif
}

int Top100::findIndexByImdbId(const std::string& imdbID) const {
    if (imdbID.empty()) return -1;
    for (size_t i = 0; i < movies.size(); ++i) {
        if (movies[i].imdbID == imdbID) return static_cast<int>(i);
    }
    return -1;
}

int Top100::findIndexByTitleYear(const std::string& title, int year) const {
    for (size_t i = 0; i < movies.size(); ++i) {
        if (movies[i].title == title && movies[i].year == year) return static_cast<int>(i);
    }
    return -1;
}

void Top100::replaceMovie(size_t index, const Movie& movie) {
    if (index < movies.size()) {
        movies[index] = movie;
    }
}

bool Top100::updateMovie(size_t index, const Movie& movie) {
    if (index >= movies.size()) return false;
    movies[index] = movie;
    return true;
}

void Top100::recomputeRanks() {
    if (movies.empty()) return;
    // Create index mapping to stable-sort by score desc, then title asc
    std::vector<size_t> idx(movies.size());
    for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](size_t i, size_t j) {
        const auto& a = movies[i];
        const auto& b = movies[j];
        if (a.userScore == b.userScore) return a.title < b.title;
        return a.userScore > b.userScore;
    });
    int rank = 1;
    for (size_t k = 0; k < idx.size(); ++k) {
        movies[idx[k]].userRank = rank++;
    }
}

bool Top100::mergeFromOmdbByImdbId(const Movie& omdbMovie) {
    if (omdbMovie.imdbID.empty()) return false;
    int idx = findIndexByImdbId(omdbMovie.imdbID);
    if (idx < 0) return false;
    Movie& dest = movies[static_cast<size_t>(idx)];
    // Preserve ranking fields
    double score = dest.userScore;
    int rank = dest.userRank;
    // Overwrite metadata fields from OMDb payload
    dest.title = omdbMovie.title;
    dest.year = omdbMovie.year;
    dest.director = omdbMovie.director;
    dest.plotShort = omdbMovie.plotShort;
    dest.plotFull = omdbMovie.plotFull;
    dest.actors = omdbMovie.actors;
    dest.genres = omdbMovie.genres;
    dest.runtimeMinutes = omdbMovie.runtimeMinutes;
    dest.countries = omdbMovie.countries;
    dest.posterUrl = omdbMovie.posterUrl;
    dest.imdbRating = omdbMovie.imdbRating;
    dest.metascore = omdbMovie.metascore;
    dest.rottenTomatoes = omdbMovie.rottenTomatoes;
    dest.source = omdbMovie.source.empty() ? dest.source : omdbMovie.source;
    dest.imdbID = omdbMovie.imdbID; // same id
    // Restore ranking
    dest.userScore = score;
    dest.userRank = rank;
    // Save immediately
    save();
    return true;
}