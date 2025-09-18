//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/config_utils.h
// Purpose: Declarations for configuration utility functions.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once
#include "config.h"
#include <functional>
#include <string>

/** @ingroup config */
/**
 * @brief Configure OMDb credentials in the active config.
 * @param cfg Active configuration (modified in-place and persisted on success)
 * @param key Candidate OMDb API key
 * @param verify Function that returns true if the key is valid
 * @return true if configuration succeeded (key accepted and saved)
 * @post On success: cfg.omdbApiKey is set, cfg.omdbEnabled=true, and the config file is updated.
 */
bool configureOmdb(AppConfig& cfg, const std::string& key,
                   const std::function<bool(const std::string&)>& verify);

/**
 * @brief Update the data file path for the movies JSON and persist the change.
 * @param cfg Active configuration (modified in-place)
 * @param newPath Absolute or relative path to the desired data file
 * @return true if the path was accepted and saved (parents created as needed)
 * @note The Top100 instance should be re-opened after changing this path.
 */
bool setDataFile(AppConfig& cfg, const std::string& newPath);

/**
 * @brief Disable OMDb integration by clearing the API key and flag.
 * @param cfg Active configuration (modified in-place and persisted)
 */
void disableOmdb(AppConfig& cfg);
