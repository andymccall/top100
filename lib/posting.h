// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List

// File: lib/posting.h
// Purpose: Shared helpers to compose and post to BlueSky and Mastodon.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once

#include <string>
#include <optional>

struct Movie;
struct AppConfig;

// Compose a post body following CLI rules: header, title, director, blank line,
// plot (possibly truncated with ellipsis), ranking, imdb rating, link, footer.
// The result will not exceed `limit` UTF-8 code points.
std::string composePostBody(const AppConfig& cfg, const Movie& m, size_t limit);

// High-level helpers that compose the body and post, including optional poster upload when available.
bool postMovieToBlueSky(const AppConfig& cfg, const Movie& m);
bool postMovieToMastodon(const AppConfig& cfg, const Movie& m);
