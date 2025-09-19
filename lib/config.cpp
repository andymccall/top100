// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/config.cpp
// Purpose: AppConfig load/save, defaults, and helpers.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "config.h"
#include <nlohmann/json.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>

using nlohmann::json;
namespace fs = std::filesystem;

static std::string getHomeDir() {
    const char* home = std::getenv("HOME");
    if (home && *home) return std::string(home);
    // Fallback: try USERPROFILE for Windows envs; else current dir
    const char* userprofile = std::getenv("USERPROFILE");
    if (userprofile && *userprofile) return std::string(userprofile);
    return ".";
}

std::string getConfigPath() {
    const char* cfg = std::getenv("TOP100_CONFIG_PATH");
    if (cfg && *cfg) return std::string(cfg);
    fs::path p = fs::path(getHomeDir()) / ".top100_config.json";
    return p.string();
}

std::string getDefaultDataPath() {
    fs::path p = fs::path(getHomeDir()) / "top100" / "top100.json";
    return p.string();
}

static void to_json(json& j, const AppConfig& c) {
    j = json{{"dataFile", c.dataFile}, {"omdbEnabled", c.omdbEnabled}};
    if (!c.omdbApiKey.empty()) j["omdbApiKey"] = c.omdbApiKey;
    // BlueSky fields
    j["blueSkyEnabled"] = c.blueSkyEnabled;
    if (!c.blueSkyIdentifier.empty()) j["blueSkyIdentifier"] = c.blueSkyIdentifier;
    if (!c.blueSkyAppPassword.empty()) j["blueSkyAppPassword"] = c.blueSkyAppPassword;
    if (!c.blueSkyService.empty()) j["blueSkyService"] = c.blueSkyService;
    // Social post customization
    if (!c.postHeaderText.empty()) j["postHeaderText"] = c.postHeaderText;
    if (!c.postFooterText.empty()) j["postFooterText"] = c.postFooterText;
    // Mastodon
    j["mastodonEnabled"] = c.mastodonEnabled;
    if (!c.mastodonInstance.empty()) j["mastodonInstance"] = c.mastodonInstance;
    if (!c.mastodonAccessToken.empty()) j["mastodonAccessToken"] = c.mastodonAccessToken;
    // UI prefs
    j["uiSortOrder"] = c.uiSortOrder;
}

static void from_json(const json& j, AppConfig& c) {
    c.dataFile = j.value("dataFile", getDefaultDataPath());
    c.omdbEnabled = j.value("omdbEnabled", false);
    c.omdbApiKey = j.value("omdbApiKey", std::string());
    // BlueSky fields with sensible defaults
    c.blueSkyEnabled = j.value("blueSkyEnabled", false);
    c.blueSkyIdentifier = j.value("blueSkyIdentifier", std::string());
    c.blueSkyAppPassword = j.value("blueSkyAppPassword", std::string());
    c.blueSkyService = j.value("blueSkyService", std::string("https://bsky.social"));
    // Social post customization with defaults
    c.postHeaderText = j.value("postHeaderText", std::string("I'd like to share one of my top 100 #movies!"));
    c.postFooterText = j.value("postFooterText", std::string("Posted with Top 100!"));
    // Mastodon
    c.mastodonEnabled = j.value("mastodonEnabled", false);
    c.mastodonInstance = j.value("mastodonInstance", std::string("https://mastodon.social"));
    c.mastodonAccessToken = j.value("mastodonAccessToken", std::string());
    // UI prefs
    c.uiSortOrder = j.value("uiSortOrder", 0);
}

AppConfig loadConfig() {
    const std::string path = getConfigPath();
    if (!fs::exists(path)) {
        AppConfig def;
        def.dataFile = getDefaultDataPath();
        def.omdbEnabled = false;
        def.omdbApiKey = "";
        def.blueSkyEnabled = false;
        def.blueSkyIdentifier = "";
        def.blueSkyAppPassword = "";
        def.blueSkyService = "https://bsky.social";
        def.postHeaderText = "I'd like to share one of my top 100 #movies!";
        def.postFooterText = "Posted with Top 100!";
        def.mastodonEnabled = false;
        def.mastodonInstance = "https://mastodon.social";
        def.mastodonAccessToken = "";
    def.uiSortOrder = 0;
        saveConfig(def);
        return def;
    }
    std::ifstream in(path);
    if (!in.is_open()) {
        throw std::runtime_error("Failed to open config file: " + path);
    }
    json j;
    in >> j;
    AppConfig cfg = j.get<AppConfig>();
    return cfg;
}

void saveConfig(const AppConfig& cfg) {
    const std::string path = getConfigPath();
    fs::path p(path);
    if (p.has_parent_path()) {
        fs::create_directories(p.parent_path());
    }
    std::ofstream out(path);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to write config file: " + path);
    }
    json j = cfg;
    out << j.dump(4);
}
