// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/config.h
// Purpose: AppConfig struct and JSON (de)serialization declarations.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once

#include <string>

/** @defgroup config Configuration (persistent settings) */

/**
 * @brief Persistent application configuration stored in a single JSON file.
 *
 * Location selection:
 * - If environment variable TOP100_CONFIG_PATH is set, that path is used.
 * - Otherwise, default path is ~/.top100_config.json.
 *
 * Field defaults (on first run):
 * - dataFile: "$HOME/top100/top100.json" (directories created automatically)
 * - omdbEnabled: false; omdbApiKey: ""
 * - blueSkyEnabled: false; blueSkyService: "https://bsky.social"
 * - mastodonEnabled: false; mastodonInstance: "https://mastodon.social"
 * - postHeaderText: "I’d like to share one of my top 100 #movies!"
 * - postFooterText: "Posted with Top 100!"
 *
 * Notes:
 * - Exactly one config file is active per run (no merging).
 * - Sensitive fields (e.g., tokens/passwords) are stored locally; do not commit.
 *
 * @ingroup config
 */
struct AppConfig {
    std::string dataFile;                 ///< Absolute path to your movie data JSON
    bool        omdbEnabled = false;      ///< Whether OMDb features are enabled in the UI
    std::string omdbApiKey;               ///< OMDb API key (empty if not configured)

    // BlueSky integration
    bool        blueSkyEnabled = false;   ///< Whether BlueSky posting is enabled
    std::string blueSkyIdentifier;        ///< Handle or email used to login
    std::string blueSkyAppPassword;       ///< App password (keep private)
    std::string blueSkyService;           ///< Service base URL (default https://bsky.social)

    // Social post customization
    std::string postHeaderText;           ///< Header shown at top of social posts (may be empty)
    std::string postFooterText;           ///< Footer shown at bottom of social posts (may be empty)

    // Mastodon integration
    bool        mastodonEnabled = false;  ///< Whether Mastodon posting is enabled
    std::string mastodonInstance;         ///< Instance base URL (e.g., https://mastodon.social)
    std::string mastodonAccessToken;      ///< User access token (keep private)
};

/**
 * @brief Resolve the configuration file path for this run.
 * @return Absolute path to the active config file.
 * @note If TOP100_CONFIG_PATH is set, it takes precedence; otherwise ~/.top100_config.json is used.
 * @ingroup config
 */
std::string getConfigPath();

/**
 * @brief Compute the default data file path for movies JSON.
 * @return Absolute path "$HOME/top100/top100.json".
 * @ingroup config
 */
std::string getDefaultDataPath();

/**
 * @brief Load configuration from disk, creating defaults if missing.
 * @return Loaded configuration; if the file did not exist, a new one is created and returned.
 * @throws std::runtime_error on unrecoverable I/O errors (e.g., permission issues).
 * @ingroup config
 */
AppConfig loadConfig();

/**
 * @brief Persist configuration to disk.
 * @param cfg Configuration to save.
 * @note Creates parent directories for the config file if needed.
 * @ingroup config
 */
void saveConfig(const AppConfig& cfg);
