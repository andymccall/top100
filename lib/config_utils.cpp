//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/config_utils.cpp
// Purpose: Higher-level helpers for configuring OMDb, data path.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "config_utils.h"
#include <filesystem>

bool configureOmdb(AppConfig& cfg, const std::string& key,
                   const std::function<bool(const std::string&)>& verify) {
    if (!verify) return false;
    if (!verify(key)) return false;
    cfg.omdbApiKey = key;
    cfg.omdbEnabled = true;
    saveConfig(cfg);
    return true;
}

bool setDataFile(AppConfig& cfg, const std::string& newPath) {
    if (newPath.empty()) return false;
    cfg.dataFile = newPath;
    // Ensure directory exists for friendliness
    std::filesystem::path p(newPath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    saveConfig(cfg);
    return true;
}

void disableOmdb(AppConfig& cfg) {
    cfg.omdbEnabled = false;
    cfg.omdbApiKey.clear();
    saveConfig(cfg);
}
