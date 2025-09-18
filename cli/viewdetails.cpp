//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/viewdetails.cpp
// Purpose: CLI details view; prefers full plot with metadata and ratings.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "viewdetails.h"
#include <iostream>
#include <limits>

static void printVec(const std::vector<std::string>& v, const char* label) {
    if (v.empty()) return;
    std::cout << label << ": ";
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) std::cout << ", ";
        std::cout << v[i];
    }
    std::cout << "\n";
}

void viewDetails(Top100& top100) {
    std::cout << "\n--- View Details ---\n";
    std::cout << "Enter exact title: ";
    std::string title;
    std::getline(std::cin, title);
    if (title.empty()) {
        std::getline(std::cin, title); // in case of leftover newline
    }

    auto movies = top100.getMovies();
    const Movie* found = nullptr;
    for (const auto& m : movies) {
        if (m.title == title) { found = &m; break; }
    }
    if (!found) {
        std::cout << "Not found.\n";
        return;
    }

    const Movie& m = *found;
    std::cout << "\nTitle: " << m.title << "\n";
    std::cout << "Year: " << m.year << "\n";
    std::cout << "Director: " << m.director << "\n";
    if (m.userRank > 0) std::cout << "My Rank: #" << m.userRank << "\n";
    std::cout << "My Score: " << m.userScore << "\n";
    if (m.runtimeMinutes > 0) std::cout << "Runtime: " << m.runtimeMinutes << " min\n";
    if (m.imdbRating > 0.0) std::cout << "IMDb Rating: " << m.imdbRating << "/10\n";
    if (m.metascore > 0) std::cout << "Metascore: " << m.metascore << "/100\n";
    if (m.rottenTomatoes > 0) std::cout << "Rotten Tomatoes: " << m.rottenTomatoes << "%\n";
    if (!m.plotFull.empty()) {
        std::cout << "Plot (full): " << m.plotFull << "\n";
    } else if (!m.plotShort.empty()) {
        std::cout << "Plot: " << m.plotShort << "\n";
    }
    printVec(m.genres, "Genres");
    printVec(m.actors, "Actors");
    printVec(m.countries, "Countries");
    if (!m.posterUrl.empty()) std::cout << "Poster: " << m.posterUrl << "\n";
    std::cout << "-----------------------\n\n";
}
