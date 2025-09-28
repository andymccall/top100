// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/omdb.h
// Purpose: OMDb client declarations.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once
#include <string>
#include <vector>
#include <optional>
#include "Movie.h"

/**
 * @brief Basic OMDb search hit.
 * @ingroup services
 */
struct OmdbSearchResult {
    /** Movie title */
    std::string title;
    /** Release year */
    int year;
    /** IMDb identifier (e.g., tt0133093) */
    std::string imdbID;
    /** Type (usually "movie"; kept for completeness) */
    std::string type;
};

/**
 * @brief Query OMDb by title keyword and return basic results.
 * @param apiKey OMDb API key
 * @param query Title keyword
 * @return Vector of OmdbSearchResult
 * @ingroup services
 */
std::vector<OmdbSearchResult> omdbSearch(const std::string& apiKey, const std::string& query);

/**
 * @brief Fetch full details for a movie by imdbID and map to Movie.
 * @param apiKey OMDb API key
 * @param imdbID IMDb identifier
 * @return Movie on success
 * @ingroup services
 */
std::optional<Movie> omdbGetById(const std::string& apiKey, const std::string& imdbID);
/**
 * @brief Verify OMDb API key by issuing a simple request.
 * @param apiKey OMDb API key
 * @return true if the key appears valid
 * @ingroup services
 */
bool omdbVerifyKey(const std::string& apiKey);
