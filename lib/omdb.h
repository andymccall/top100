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

/** @ingroup services */
struct OmdbSearchResult {
    std::string title;
    int year;
    std::string imdbID;
    std::string type;
};

/** Query OMDb by title keyword and return basic results. @ingroup services */
std::vector<OmdbSearchResult> omdbSearch(const std::string& apiKey, const std::string& query);

/** Fetch full details for a movie by imdbID and map to Movie. @ingroup services */
std::optional<Movie> omdbGetById(const std::string& apiKey, const std::string& imdbID);
/** Verify OMDb API key by issuing a simple request. @ingroup services */
bool omdbVerifyKey(const std::string& apiKey);
