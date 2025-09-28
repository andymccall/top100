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

/**
 * @brief Compose a post body for social networks with length limits.
 *
 * Follows the same structure used by the CLI: optional header, title + year, optional
 * director line, blank line, plot (possibly truncated with an ellipsis to fit), user
 * ranking, optional IMDb rating, optional IMDb link, and optional footer.
 *
 * The returned string will not exceed the provided UTF‑8 codepoint limit; truncation
 * is performed at codepoint boundaries to avoid breaking multi‑byte sequences.
 *
 * @param cfg Application configuration providing header/footer texts.
 * @param m Movie to describe in the post.
 * @param limit Maximum number of UTF‑8 codepoints allowed in the post body.
 * @return Composed post text that fits within the limit.
 */
std::string composePostBody(const AppConfig& cfg, const Movie& m, size_t limit);

/**
 * @brief Post a movie to BlueSky, uploading the poster when available.
 *
 * Uses the BlueSky credentials from the configuration to create a session, optionally
 * uploads the poster image, composes the body with composePostBody(), and creates the post.
 *
 * \param cfg Application configuration (BlueSky credentials, header/footer texts)
 * \param m Movie to post
 * \return true on successful post, false otherwise
 */
bool postMovieToBlueSky(const AppConfig& cfg, const Movie& m);

/**
 * @brief Post a movie to Mastodon, uploading the poster when available.
 *
 * Uses the Mastodon access token and instance from the configuration, optionally uploads
 * the poster as media, composes the body with composePostBody(), and posts the status.
 *
 * \param cfg Application configuration (Mastodon instance and token, header/footer texts)
 * \param m Movie to post
 * \return true on successful post, false otherwise
 */
bool postMovieToMastodon(const AppConfig& cfg, const Movie& m);
