//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/Movie.h
// Purpose: Movie model and JSON (de)serialization fields.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

/**
 * @brief Movie domain model and metadata.
 *
 * Required fields: title, year, director.
 * Optional fields are populated via OMDb where available. Ranking fields
 * (userScore/userRank) are persisted to ensure stable ordering across sessions.
 *
 * @ingroup core
 */
struct Movie {
    std::string title;
    int year;
    std::string director;
    // Additional metadata (optional)
    std::string plotShort;              // OMDb short plot
    std::string plotFull;               // OMDb full plot
    std::vector<std::string> actors;     // up to first 10 from OMDb
    std::vector<std::string> genres;     // parsed from comma-separated "Genre"
    int runtimeMinutes = 0;              // parsed from "Runtime" (e.g., "148 min")
    std::vector<std::string> countries;  // parsed from comma-separated "Country"
    std::string posterUrl;               // from "Poster"
    // Ratings (optional)
    double imdbRating = 0.0;             // 0.0-10.0 from OMDb "imdbRating"
    int metascore = 0;                   // 0-100 from OMDb "Metascore"
    int rottenTomatoes = 0;              // 0-100 from OMDb Ratings[Source=="Rotten Tomatoes"]
    // Provenance and IDs
    std::string source;                  // "manual" or "omdb" (optional; default empty)
    std::string imdbID;                  // from OMDb; empty for manual entries
    // User ranking fields
    double userScore = 1500.0;           // Elo-like score for pairwise ranking
    int userRank = -1;                   // 1-based rank; -1 means unranked
};

/**
 * @brief Serialize Movie to JSON.
 * @param j Output JSON object
 * @param m Source movie
 * @note Always persists userScore and userRank for deterministic ranking.
 */
inline void to_json(nlohmann::json& j, const Movie& m) {
    j = nlohmann::json{{"title", m.title}, {"year", m.year}, {"director", m.director}};
    if (!m.plotShort.empty()) j["plotShort"] = m.plotShort;
    if (!m.plotFull.empty()) j["plotFull"] = m.plotFull;
    if (!m.actors.empty()) j["actors"] = m.actors;
    if (!m.genres.empty()) j["genres"] = m.genres;
    if (m.runtimeMinutes > 0) j["runtimeMinutes"] = m.runtimeMinutes;
    if (!m.countries.empty()) j["countries"] = m.countries;
    if (!m.posterUrl.empty()) j["posterUrl"] = m.posterUrl;
    if (m.imdbRating > 0.0) j["imdbRating"] = m.imdbRating;
    if (m.metascore > 0) j["metascore"] = m.metascore;
    if (m.rottenTomatoes > 0) j["rottenTomatoes"] = m.rottenTomatoes;
    if (!m.source.empty()) j["source"] = m.source;
    if (!m.imdbID.empty()) j["imdbID"] = m.imdbID;
    // Always persist ranking fields for stability across sessions
    j["userScore"] = m.userScore;
    j["userRank"] = m.userRank;
}

/**
 * @brief Deserialize Movie from JSON.
 * @param j Source JSON object
 * @param m Output movie
 * @note Missing optional fields are default-initialized to sensible values.
 */
inline void from_json(const nlohmann::json& j, Movie& m) {
    // Required legacy fields
    if (j.contains("title")) j.at("title").get_to(m.title); else m.title = "";
    if (j.contains("year")) j.at("year").get_to(m.year); else m.year = 0;
    if (j.contains("director")) j.at("director").get_to(m.director); else m.director = "";

    // Optional new fields (present in newer saves)
    m.plotShort = j.value("plotShort", std::string());
    m.plotFull = j.value("plotFull", std::string());
    if (j.contains("actors")) j.at("actors").get_to(m.actors); else m.actors.clear();
    if (j.contains("genres")) j.at("genres").get_to(m.genres); else m.genres.clear();
    m.runtimeMinutes = j.value("runtimeMinutes", 0);
    if (j.contains("countries")) j.at("countries").get_to(m.countries); else m.countries.clear();
    m.posterUrl = j.value("posterUrl", "");
    m.imdbRating = j.value("imdbRating", 0.0);
    m.metascore = j.value("metascore", 0);
    m.rottenTomatoes = j.value("rottenTomatoes", 0);
    m.source = j.value("source", "");
    m.imdbID = j.value("imdbID", "");
    // User ranking fields (default for legacy files)
    m.userScore = j.value("userScore", 1500.0);
    m.userRank = j.value("userRank", -1);
}
