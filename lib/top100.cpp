// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/top100.cpp
// Purpose: Core list persistence, sorting, and ranking recompute.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "top100.h"
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

Top100::Top100(const std::string& filename) : filename(filename) {
    load();
}

Top100::~Top100() {
    save();
}

void Top100::addMovie(const Movie& movie) {
    movies.push_back(movie);
}

void Top100::removeMovie(const std::string& title) {
    movies.erase(std::remove_if(movies.begin(), movies.end(), [&](const Movie& movie) {
        return movie.title == title;
    }), movies.end());
}

bool Top100::removeByImdbId(const std::string& imdbID) {
    if (imdbID.empty()) return false;
    auto it = std::remove_if(movies.begin(), movies.end(), [&](const Movie& m){ return m.imdbID == imdbID; });
    if (it == movies.end()) return false;
    movies.erase(it, movies.end());
    return true;
}

std::vector<Movie> Top100::getMovies(SortOrder order) const {
    std::vector<Movie> sorted_movies = movies; // Create a copy to sort

    switch (order) {
        case SortOrder::BY_USER_RANK:
            std::sort(sorted_movies.begin(), sorted_movies.end(), [](const Movie& a, const Movie& b) {
                // Unranked go last
                if (a.userRank == -1 && b.userRank == -1) return a.title < b.title;
                if (a.userRank == -1) return false;
                if (b.userRank == -1) return true;
                return a.userRank < b.userRank;
            });
            break;
        case SortOrder::BY_USER_SCORE:
            std::sort(sorted_movies.begin(), sorted_movies.end(), [](const Movie& a, const Movie& b) {
                if (a.userScore == b.userScore) return a.title < b.title; // stable tie-breaker
                return a.userScore > b.userScore; // high to low
            });
            break;
        case SortOrder::BY_YEAR:
            std::sort(sorted_movies.begin(), sorted_movies.end(), [](const Movie& a, const Movie& b) {
                return a.year < b.year;
            });
            break;
        case SortOrder::ALPHABETICAL:
            std::sort(sorted_movies.begin(), sorted_movies.end(), [](const Movie& a, const Movie& b) {
                return a.title < b.title;
            });
            break;
        case SortOrder::DEFAULT:
        default:
            // Do nothing, return in insertion order
            break;
    }
    return sorted_movies;
}

void Top100::load() {
    std::ifstream file(filename);
    if (file.is_open()) {
        nlohmann::json j;
        file >> j;
        // Add a check to ensure the file is not empty or malformed
        if (j.is_array()) {
            movies = j.get<std::vector<Movie>>();
        }
    }
}

void Top100::save() {
    nlohmann::json j = movies;
    std::filesystem::path p(filename);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
        // ignore ec; failing to create may be due to permissions; writing will fail
    }
    std::ofstream file(filename);
    file << j.dump(4);
}

int Top100::findIndexByImdbId(const std::string& imdbID) const {
    if (imdbID.empty()) return -1;
    for (size_t i = 0; i < movies.size(); ++i) {
        if (movies[i].imdbID == imdbID) return static_cast<int>(i);
    }
    return -1;
}

int Top100::findIndexByTitleYear(const std::string& title, int year) const {
    for (size_t i = 0; i < movies.size(); ++i) {
        if (movies[i].title == title && movies[i].year == year) return static_cast<int>(i);
    }
    return -1;
}

void Top100::replaceMovie(size_t index, const Movie& movie) {
    if (index < movies.size()) {
        movies[index] = movie;
    }
}

bool Top100::updateMovie(size_t index, const Movie& movie) {
    if (index >= movies.size()) return false;
    movies[index] = movie;
    return true;
}

void Top100::recomputeRanks() {
    if (movies.empty()) return;
    // Create index mapping to stable-sort by score desc, then title asc
    std::vector<size_t> idx(movies.size());
    for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](size_t i, size_t j) {
        const auto& a = movies[i];
        const auto& b = movies[j];
        if (a.userScore == b.userScore) return a.title < b.title;
        return a.userScore > b.userScore;
    });
    int rank = 1;
    for (size_t k = 0; k < idx.size(); ++k) {
        movies[idx[k]].userRank = rank++;
    }
}