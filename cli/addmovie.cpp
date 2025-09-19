// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/addmovie.cpp
// Purpose: CLI handler to add a movie manually.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "addmovie.h"
#include <iostream>
#include <string>
#include <limits>
#include "dup_policy.h"

void addMovie(Top100& top100) {
    Movie movie;
    
    std::cout << "Enter title: ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer
    std::getline(std::cin, movie.title);

    std::cout << "Enter year: ";
    std::cin >> movie.year;

    std::cout << "Enter director: ";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear the input buffer
    std::getline(std::cin, movie.director);

    movie.source = "manual";

    // Check for existing by exact title+year
    int idx = top100.findIndexByTitleYear(movie.title, movie.year);
    if (idx >= 0) {
        auto policy = getDuplicatePolicy();
        bool overwrite = false;
        switch (policy) {
            case DuplicatePolicy::PreferManual:
                // We are adding a manual entry; prefer manual means overwrite existing
                overwrite = true; break;
            case DuplicatePolicy::PreferOmdb:
            case DuplicatePolicy::Skip:
                overwrite = false; break;
            case DuplicatePolicy::Prompt:
            default: {
                std::cout << "A movie with this title and year already exists. Overwrite? (y/N): ";
                char ans = 'n';
                std::cin >> ans;
                overwrite = (ans == 'y' || ans == 'Y');
            } break;
        }
        if (overwrite) {
            top100.replaceMovie(static_cast<size_t>(idx), movie);
            std::cout << "Movie overwritten." << std::endl;
        } else {
            std::cout << "Skipped." << std::endl;
        }
    } else {
        top100.addMovie(movie);
        std::cout << "Movie added." << std::endl;
    }
}