// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/listmovies.cpp
// Purpose: CLI listing and sorting (default, by year/title/rank/score).
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "listmovies.h"
#include <iostream>
#include <vector>
#include "Movie.h"

void listMovies(const Top100& top100) {
    char input = 0;
    SortOrder order = SortOrder::DEFAULT;

    std::cout << "\n--- List Movies ---\n";
    std::cout << "1. List by insertion order\n";
    std::cout << "2. List by year\n";
    std::cout << "3. List alphabetically by title\n";
    std::cout << "4. List by my rank (best first)\n";
    std::cout << "5. List by my score (Elo)\n";
    std::cout << "Enter your choice: ";
    std::cin >> input;

    switch (input) {
        case '1':
            order = SortOrder::DEFAULT;
            break;
        case '2':
            order = SortOrder::BY_YEAR;
            break;
        case '3':
            order = SortOrder::ALPHABETICAL;
            break;
        case '4':
            order = SortOrder::BY_USER_RANK;
            break;
        case '5':
            order = SortOrder::BY_USER_SCORE;
            break;
        default:
            std::cout << "Invalid option, defaulting to insertion order.\n";
            order = SortOrder::DEFAULT;
            break;
    }

    std::vector<Movie> movies = top100.getMovies(order);
    if (movies.empty()) {
        std::cout << "\nNo movies in your list." << std::endl;
    } else {
        std::cout << "\n--- Your Top Movies ---\n";
        for (const auto& movie : movies) {
            std::cout << (movie.userRank > 0 ? ("#" + std::to_string(movie.userRank) + " ") : "")
                      << "Title: " << movie.title 
                      << ", Year: " << movie.year 
                      << ", Director: " << movie.director
                      << ", Source: " << (movie.source.empty() ? "unknown" : movie.source)
                      << ", Score: " << static_cast<int>(movie.userScore)
                      << std::endl;
        }
        std::cout << "-----------------------\n";
    }
}