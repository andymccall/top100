//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/omdb.cpp
// Purpose: OMDb REST client functions (search, get-by-id, verify key).
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "omdb.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <sstream>

static int parseYear(const std::string& y) {
    try {
        return std::stoi(y.substr(0, 4));
    } catch (...) {
        return 0;
    }
}

static int parseRuntimeMinutes(const std::string& runtime) {
    // e.g., "148 min" -> 148
    try {
        size_t pos = runtime.find(' ');
        std::string num = (pos == std::string::npos) ? runtime : runtime.substr(0, pos);
        return std::stoi(num);
    } catch (...) {
        return 0;
    }
}

static std::vector<std::string> splitCommaTrim(const std::string& s, size_t limit = SIZE_MAX) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, ',')) {
        // trim leading/trailing spaces
        size_t start = item.find_first_not_of(" \t\n\r");
        size_t end = item.find_last_not_of(" \t\n\r");
        std::string trimmed = (start == std::string::npos) ? std::string() : item.substr(start, end - start + 1);
        if (!trimmed.empty()) out.push_back(trimmed);
        if (out.size() >= limit) break;
    }
    return out;
}

std::vector<OmdbSearchResult> omdbSearch(const std::string& apiKey, const std::string& query) {
    std::vector<OmdbSearchResult> out;
    auto r = cpr::Get(cpr::Url{"https://www.omdbapi.com/"},
                      cpr::Parameters{{"apikey", apiKey}, {"s", query}});
    if (r.status_code != 200) return out;
    auto j = nlohmann::json::parse(r.text, nullptr, false);
    if (j.is_discarded() || !j.contains("Search")) return out;
    for (const auto& item : j["Search"]) {
        OmdbSearchResult s;
        s.title = item.value("Title", "");
        s.year = parseYear(item.value("Year", "0"));
        s.imdbID = item.value("imdbID", "");
        s.type = item.value("Type", "");
        out.push_back(std::move(s));
    }
    return out;
}

std::optional<Movie> omdbGetById(const std::string& apiKey, const std::string& imdbID) {
    // Fetch full plot first
    auto rFull = cpr::Get(cpr::Url{"https://www.omdbapi.com/"},
                          cpr::Parameters{{"apikey", apiKey}, {"i", imdbID}, {"plot", "full"}});
    if (rFull.status_code != 200) return std::nullopt;
    auto j = nlohmann::json::parse(rFull.text, nullptr, false);
    if (j.is_discarded()) return std::nullopt;

    Movie m;
    m.title = j.value("Title", "");
    m.year = parseYear(j.value("Year", "0"));
    m.director = j.value("Director", "");
    m.imdbID = j.value("imdbID", imdbID);
    m.source = "omdb";
    m.actors = splitCommaTrim(j.value("Actors", ""), 10);
    m.genres = splitCommaTrim(j.value("Genre", ""));
    m.runtimeMinutes = parseRuntimeMinutes(j.value("Runtime", "0"));
    m.countries = splitCommaTrim(j.value("Country", ""));
    m.posterUrl = j.value("Poster", "");
    // Plot fields
    m.plotFull = j.value("Plot", std::string());
    // If you want to also store the short plot, fetch it too
    auto rShort = cpr::Get(cpr::Url{"https://www.omdbapi.com/"},
                           cpr::Parameters{{"apikey", apiKey}, {"i", imdbID}, {"plot", "short"}});
    if (rShort.status_code == 200) {
        auto js = nlohmann::json::parse(rShort.text, nullptr, false);
        if (!js.is_discarded()) {
            m.plotShort = js.value("Plot", std::string());
        }
    }
    // Ratings
    // imdbRating can be "N/A" or a string like "7.8"
    {
        std::string ir = j.value("imdbRating", "0");
        try { m.imdbRating = (ir == "N/A" ? 0.0 : std::stod(ir)); } catch (...) { m.imdbRating = 0.0; }
    }
    {
        std::string ms = j.value("Metascore", "0");
        try { m.metascore = (ms == "N/A" ? 0 : std::stoi(ms)); } catch (...) { m.metascore = 0; }
    }
    // Ratings array: find Rotten Tomatoes
    if (j.contains("Ratings") && j["Ratings"].is_array()) {
        for (const auto& r : j["Ratings"]) {
            std::string src = r.value("Source", "");
            if (src == "Rotten Tomatoes") {
                std::string val = r.value("Value", "0%");
                // Typically like "87%" or "N/A"
                size_t pos = val.find('%');
                std::string num = (pos == std::string::npos) ? val : val.substr(0, pos);
                try { m.rottenTomatoes = (val == "N/A" ? 0 : std::stoi(num)); } catch (...) { m.rottenTomatoes = 0; }
                break;
            }
        }
    }
    return m;
}

// Simple API key verification: perform a benign query and check for valid JSON
bool omdbVerifyKey(const std::string& apiKey) {
    auto r = cpr::Get(cpr::Url{"https://www.omdbapi.com/"},
                      cpr::Parameters{{"apikey", apiKey}, {"s", "test"}});
    if (r.status_code != 200) return false;
    auto j = nlohmann::json::parse(r.text, nullptr, false);
    if (j.is_discarded()) return false;
    // OMDb returns { "Response":"False", "Error":"Invalid API key!" } for bad keys
    std::string resp = j.value("Response", "");
    if (resp == "False") return false;
    return true;
}
