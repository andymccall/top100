// SPDX-License-Identifier: Apache-2.0
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
    /** Movie title */
    std::string title;
    /** Release year */
    int year;
    /** Director name(s) */
    std::string director;
    // Additional metadata (optional)
    /** OMDb short plot */
    std::string plotShort;
    /** OMDb full plot */
    std::string plotFull;
    /** Up to first 10 actor names from OMDb */
    std::vector<std::string> actors;
    /** Parsed from comma-separated OMDb "Genre" */
    std::vector<std::string> genres;
    /** Parsed from OMDb "Runtime" (e.g., 148 for "148 min") */
    int runtimeMinutes = 0;
    /** Parsed from comma-separated OMDb "Country" */
    std::vector<std::string> countries;
    /** Poster URL from OMDb "Poster" */
    std::string posterUrl;
    // Ratings (optional)
    /** IMDb rating 0.0-10.0 from OMDb "imdbRating" */
    double imdbRating = 0.0;
    /** Metascore 0-100 from OMDb "Metascore" */
    int metascore = 0;
    /** Rotten Tomatoes 0-100 from OMDb Ratings[Source=="Rotten Tomatoes"] */
    int rottenTomatoes = 0;
    // Provenance and IDs
    /** Source tag: "manual" or "omdb" (optional; default empty) */
    std::string source;
    /** IMDb identifier (e.g., tt0133093); empty for manual entries */
    std::string imdbID;
    // User ranking fields
    /** Elo-like score for pairwise ranking */
    double userScore = 1500.0;
    /** 1-based rank; -1 means unranked */
    int userRank = -1;
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
