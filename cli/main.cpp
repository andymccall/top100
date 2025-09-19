// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/main.cpp
// Purpose: CLI entry point and command routing (OMDb, ranking, details, BlueSky, Mastodon).
// Language: C++17 (CMake build)
//
// Summary:
// Orchestrates the interactive menu, loads configuration and data, and dispatches
// actions including adding/removing/listing movies, Elo ranking, OMDb integration,
// and social posting with character-limit handling.
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "main.h"
#include "comparemovies.h"
#include "config.h"
#include "omdb.h"
#include "config_utils.h"
#include "bluesky.h"
#include "mastodon.h"
#include <cpr/cpr.h>
#include "posting.h"
#include <limits>

int main()
{
    // Load or create configuration
    AppConfig cfg = loadConfig();

    Top100 top100(cfg.dataFile);
    // Ensure ranks exist on startup (for legacy data)
    top100.recomputeRanks();
    char input;

    do
    {
    displayMenu(cfg.omdbEnabled, cfg.blueSkyEnabled, cfg.mastodonEnabled);
        std::cin >> input;

        switch (input)
        {
        case '1':
            addMovie(top100);
            break;
        case '2':
            removeMovie(top100);
            break;
        case '3':
            listMovies(top100);
            break;
        case '4':
            if (cfg.omdbEnabled) {
                addFromOmdb(top100, cfg.omdbApiKey);
            } else {
                // Configure OMDb API key
                std::cout << "Enter OMDb API key: ";
                std::string key;
                std::cin >> key;
                std::cout << "Verifying key...\n";
                if (configureOmdb(cfg, key, omdbVerifyKey)) {
                    std::cout << "OMDb API key verified and saved.\n";
                } else {
                    std::cout << "Invalid OMDb API key. Please try again.\n";
                }
            }
            break;
        case '5':
            if (cfg.omdbEnabled) {
                // Disable OMDb
                disableOmdb(cfg);
                std::cout << "OMDb disabled.\n";
            } else {
                // Set data file path
                std::cout << "Enter path for data file (top100.json): ";
                std::string path;
                std::cin >> path;
                if (setDataFile(cfg, path)) {
                    std::cout << "Data path updated: " << cfg.dataFile << "\n";
                    // Reopen Top100 with new path
                    top100 = Top100(cfg.dataFile);
                    top100.recomputeRanks();
                } else {
                    std::cout << "Invalid path, not updated.\n";
                }
            }
            break;
        case '6':
            // consume leftover newline before getline in viewDetails
            if (std::cin.peek() == '\n') std::cin.get();
            viewDetails(top100);
            break;
        case '7':
            compareMovies(top100);
            break;
        case '8':
            if (cfg.blueSkyEnabled) {
                // Select a movie to post
                std::cout << "Enter the exact movie title to post: ";
                if (std::cin.peek() == '\n') std::cin.get();
                {
                    std::string title;
                    std::getline(std::cin, title);
                    // Try to find by exact title; if multiple, ask for year
                    std::vector<Movie> all = top100.getMovies(SortOrder::DEFAULT);
                    std::vector<const Movie*> matches;
                    for (const auto& m : all) if (m.title == title) matches.push_back(&m);
                    const Movie* chosen = nullptr;
                    if (matches.empty()) {
                        std::cout << "No movie found with that title.\n";
                    } else if (matches.size() == 1) {
                        chosen = matches.front();
                    } else {
                        std::cout << "Multiple matches. Enter year: ";
                        int y = 0; std::cin >> y;
                        for (auto* m : matches) if (m->year == y) { chosen = m; break; }
                        if (!chosen) { std::cout << "No exact title+year match.\n"; }
                    }
                    if (chosen) {
                        std::cout << "Posting to BlueSky...\n";
                        bool ok = postMovieToBlueSky(cfg, *chosen);
                        std::cout << (ok ? "Posted successfully.\n" : "Failed to post.\n");
                    }
                }
            } else {
                // Configure BlueSky account
                std::cout << "Enter BlueSky handle/email: ";
                std::string id; std::cin >> id;
                std::cout << "Enter BlueSky app password: ";
                std::string pw; std::cin >> pw;
                std::cout << "Service URL (default https://bsky.social): ";
                std::string svc; std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::getline(std::cin, svc);
                if (svc.empty()) svc = "https://bsky.social";
                auto session = bskyCreateSession(svc, id, pw);
                if (session) {
                    cfg.blueSkyEnabled = true;
                    cfg.blueSkyIdentifier = id;
                    cfg.blueSkyAppPassword = pw;
                    cfg.blueSkyService = svc;
                    saveConfig(cfg);
                    std::cout << "BlueSky account verified and saved.\n";
                } else {
                    std::cout << "Could not verify BlueSky credentials.\n";
                }
            }
            break;
        case '9':
            if (cfg.mastodonEnabled) {
                // Select a movie to post
                std::cout << "Enter the exact movie title to post: ";
                if (std::cin.peek() == '\n') std::cin.get();
                {
                    std::string title;
                    std::getline(std::cin, title);
                    std::vector<Movie> all = top100.getMovies(SortOrder::DEFAULT);
                    std::vector<const Movie*> matches;
                    for (const auto& m : all) if (m.title == title) matches.push_back(&m);
                    const Movie* chosen = nullptr;
                    if (matches.empty()) {
                        std::cout << "No movie found with that title.\n";
                    } else if (matches.size() == 1) {
                        chosen = matches.front();
                    } else {
                        std::cout << "Multiple matches. Enter year: ";
                        int y = 0; std::cin >> y;
                        for (auto* m : matches) if (m->year == y) { chosen = m; break; }
                        if (!chosen) { std::cout << "No exact title+year match.\n"; }
                    }
                    if (chosen) {
                        std::cout << "Posting to Mastodon...\n";

                        bool ok = postMovieToMastodon(cfg, *chosen);
                        std::cout << (ok ? "Posted successfully.\n" : "Failed to post.\n");
                    }
                }
            } else {
                // Configure Mastodon
                std::cout << "Enter Mastodon instance base URL (e.g., https://mastodon.social): ";
                std::string inst; std::cin >> inst;
                std::cout << "Enter Mastodon access token: ";
                std::string token; std::cin >> token;
                if (mastoVerify(inst, token)) {
                    cfg.mastodonEnabled = true;
                    cfg.mastodonInstance = inst;
                    cfg.mastodonAccessToken = token;
                    saveConfig(cfg);
                    std::cout << "Mastodon account verified and saved.\n";
                } else {
                    std::cout << "Could not verify Mastodon credentials.\n";
                }
            }
            break;
        case '0':
            // Edit post header/footer
            std::cout << "Enter new post header (empty to clear, leave blank to keep current):\n> ";
            if (std::cin.peek() == '\n') std::cin.get();
            {
                std::string header;
                std::getline(std::cin, header);
                if (!header.empty() || header.size() == 0) {
                    cfg.postHeaderText = header; // allow empty to clear
                }
            }
            std::cout << "Enter new post footer (empty to clear, leave blank to keep current):\n> ";
            {
                std::string footer;
                std::getline(std::cin, footer);
                if (!footer.empty() || footer.size() == 0) {
                    cfg.postFooterText = footer; // allow empty to clear
                }
            }
            saveConfig(cfg);
            std::cout << "Post header/footer updated.\n";
            break;
        default:
            break;
        }
        
    } while (input != 'q');
    
    return 0;
}