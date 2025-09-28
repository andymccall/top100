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

/**
 * @brief BlueSky login session tokens.
 *
 * Holds the access JWT and DID returned after a successful login. Required for
 * authenticated API calls such as uploading images and creating posts.
 * @ingroup services
 */
struct BlueSkySession {
    /** Access JWT for authenticated requests */
    std::string accessJwt;
    /** DID (decentralized identifier) of the account */
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
 * @param serviceBase Base URL of the BlueSky service (e.g., https://bsky.social)
 * @param accessJwt Access token returned by bskyCreateSession
 * @param bytes Raw image bytes
 * @param contentType MIME type, typically "image/jpeg" or "image/png"
 * @return Serialized JSON blob string for app.bsky.embed.images on success; std::nullopt on failure
 * @ingroup services
 */
std::optional<std::string> bskyUploadImage(const std::string& serviceBase,
                                           const std::string& accessJwt,
                                           const std::vector<unsigned char>& bytes,
                                           const std::string& contentType);

/**
 * @brief Create a text post with optional image embed.
 *
 * @param serviceBase Base URL of the BlueSky service (e.g., https://bsky.social)
 * @param accessJwt Access token returned by bskyCreateSession
 * @param repoDid DID of the repository (the authenticated account's DID)
 * @param text Post body text
 * @param imageBlobJson Optional JSON blob returned by bskyUploadImage for embedding
 * @return true if the post was successfully created, false otherwise
 * @ingroup services
 */
bool bskyCreatePost(const std::string& serviceBase,
                    const std::string& accessJwt,
                    const std::string& repoDid,
                    const std::string& text,
                    const std::optional<std::string>& imageBlobJson);

/**
 * @brief High-level helper: login, optionally upload poster, and post a movie.
 *
 * @param serviceBase BlueSky base URL (e.g., https://bsky.social)
 * @param identifier Account handle or email
 * @param appPassword App password for the account
 * @param movie Movie to post
 * @return true on success, false on failure
 * @ingroup services
 */
bool bskyPostMovie(const std::string& serviceBase,
                   const std::string& identifier,
                   const std::string& appPassword,
                   const Movie& movie);
