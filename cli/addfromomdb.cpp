// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/addfromomdb.cpp
// Purpose: CLI flow to search OMDb by title, select a result, and add it.
// Language: C++17 (CMake build)
//
// Summary:
// Provides interactive search via OMDb, fetches full details by IMDb ID,
// and persists the selected movie into the Top 100 list.
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "addfromomdb.h"
#include "omdb.h"
#include <iostream>
#include <string>
#include "dup_policy.h"

void addFromOmdb(Top100& top100, const std::string& apiKey) {
    std::cout << "Enter a title to search: ";
    std::string query;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::getline(std::cin, query);

    auto results = omdbSearch(apiKey, query);
    if (results.empty()) {
        std::cout << "No results found.\n";
        return;
    }

    std::cout << "\nResults:\n";
    for (size_t i = 0; i < results.size(); ++i) {
        std::cout << (i + 1) << ") " << results[i].title << " (" << results[i].year << ")\n";
    }
    std::cout << "Choose a number to add (or 0 to cancel): ";
    size_t choice = 0;
    std::cin >> choice;
    if (choice == 0 || choice > results.size()) {
        std::cout << "Cancelled.\n";
        return;
    }

    auto full = omdbGetById(apiKey, results[choice - 1].imdbID);
    if (!full) {
        std::cout << "Failed to fetch details.\n";
        return;
    }
    // mark source (already set in omdbGetById, but ensure)
    full->source = "omdb";

    // Try dedupe: prefer imdbID, fallback to title+year
    int idx = top100.findIndexByImdbId(full->imdbID);
    if (idx < 0) idx = top100.findIndexByTitleYear(full->title, full->year);

    if (idx >= 0) {
        auto policy = getDuplicatePolicy();
        bool overwrite = false;
        switch (policy) {
            case DuplicatePolicy::PreferOmdb: overwrite = true; break;
            case DuplicatePolicy::PreferManual:
            case DuplicatePolicy::Skip: overwrite = false; break;
            case DuplicatePolicy::Prompt:
            default: {
                std::cout << "This movie already exists in your list. Overwrite with OMDb data? (y/N): ";
                char ans = 'n';
                std::cin >> ans;
                overwrite = (ans == 'y' || ans == 'Y');
            } break;
        }
        if (overwrite) {
            top100.replaceMovie(static_cast<size_t>(idx), *full);
            std::cout << "Movie overwritten with OMDb data.\n";
        } else {
            std::cout << "Skipped adding duplicate.\n";
        }
    } else {
        top100.addMovie(*full);
        std::cout << "Added '" << full->title << "' (" << full->year << ")\n";
    }
}
