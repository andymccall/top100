// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/mastodon.h
// Purpose: Mastodon client declarations.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once

#include <string>
#include <optional>
#include <vector>

/** @defgroup services External service clients */
/** Minimal Mastodon client helpers @ingroup services */

// Verify the access token by calling /api/v1/accounts/verify_credentials
/**
 * @brief Verify token via /api/v1/accounts/verify_credentials.
 * @param instanceBaseUrl Base URL of the Mastodon instance
 * @param accessToken Access token
 * @return true if the token is valid
 * @ingroup services
 */
bool mastoVerify(const std::string& instanceBaseUrl, const std::string& accessToken);

// Upload media and return the media id string
/**
 * @brief Upload media and return media id.
 * @param instanceBaseUrl Base URL of the Mastodon instance
 * @param accessToken Access token
 * @param bytes Raw image bytes
 * @param filename Suggested filename
 * @param contentType MIME type (e.g., image/jpeg)
 * @return media id string on success
 * @ingroup services
 */
std::optional<std::string> mastoUploadMedia(const std::string& instanceBaseUrl,
                                            const std::string& accessToken,
                                            const std::vector<unsigned char>& bytes,
                                            const std::string& filename,
                                            const std::string& contentType);

// Post a status with optional single media id
/**
 * @brief Post a status with optional media id.
 * @param instanceBaseUrl Base URL of the Mastodon instance
 * @param accessToken Access token
 * @param text Status text
 * @param mediaId Optional media id (from mastoUploadMedia)
 * @return true on success
 * @ingroup services
 */
bool mastoPostStatus(const std::string& instanceBaseUrl,
                     const std::string& accessToken,
                     const std::string& text,
                     const std::optional<std::string>& mediaId);
