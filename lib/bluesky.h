// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/bluesky.h
// Purpose: BlueSky client declarations.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once

#include <string>
#include <optional>
#include <vector>

struct Movie;

/** @ingroup services */
struct BlueSkySession {
    std::string accessJwt;
    std::string did;
};

/**
 * @brief Create a BlueSky session (login).
 * @param serviceBase Base URL (e.g., https://bsky.social)
 * @param identifier Handle or email
 * @param appPassword App password
 * @return Session on success, std::nullopt on failure
 * @ingroup services
 */
std::optional<BlueSkySession> bskyCreateSession(const std::string& serviceBase,
                                                const std::string& identifier,
                                                const std::string& appPassword);

/**
 * @brief Upload an image blob for embedding in a BlueSky post.
 * @param bytes Raw image bytes
 * @param contentType image/jpeg or image/png
 * @return Serialized JSON blob string for app.bsky.embed.images
 * @ingroup services
 */
std::optional<std::string> bskyUploadImage(const std::string& serviceBase,
                                           const std::string& accessJwt,
                                           const std::vector<unsigned char>& bytes,
                                           const std::string& contentType);

/** @brief Create a text post with optional image embed. @ingroup services */
bool bskyCreatePost(const std::string& serviceBase,
                    const std::string& accessJwt,
                    const std::string& repoDid,
                    const std::string& text,
                    const std::optional<std::string>& imageBlobJson);

/** @brief High-level helper: compose and post a movie with optional poster. @ingroup services */
bool bskyPostMovie(const std::string& serviceBase,
                   const std::string& identifier,
                   const std::string& appPassword,
                   const Movie& movie);
