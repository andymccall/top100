// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/comparemovies.cpp
// Purpose: Pairwise comparison loop implementing Elo-style updates.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "comparemovies.h"
#include <iostream>
#include <random>
#include <chrono>

static void updateElo(double& a, double& b, double scoreA, double k = 32.0) {
    // scoreA: 1.0 if A wins, 0.0 if A loses
    const double qa = std::pow(10.0, a / 400.0);
    const double qb = std::pow(10.0, b / 400.0);
    const double ea = qa / (qa + qb);
    const double eb = qb / (qa + qb);
    a = a + k * (scoreA - ea);
    b = b + k * ((1.0 - scoreA) - eb);
}

void compareMovies(Top100& top100) {
    auto seed = static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count());
    std::mt19937 rng(seed);

    while (true) {
        auto movies = top100.getMovies();
        if (movies.size() < 2) {
            std::cout << "Need at least two movies to compare.\n";
            return;
        }

        std::uniform_int_distribution<size_t> dist(0, movies.size() - 1);
        size_t i = dist(rng);
        size_t j = dist(rng);
        while (j == i) j = dist(rng);

        const Movie& A = movies[i];
        const Movie& B = movies[j];

        std::cout << "\nWhich movie do you prefer? (q to stop)\n";
        std::cout << "1. " << A.title << " (" << A.year << ")\n";
        std::cout << "2. " << B.title << " (" << B.year << ")\n";
        std::cout << "Enter 1 or 2, or q: ";
        char choice;
        std::cin >> choice;
        if (choice == 'q') return;
        if (choice != '1' && choice != '2') {
            std::cout << "Invalid choice. Try again.\n";
            continue;
        }

        Movie mA = A;
        Movie mB = B;
        if (choice == '1') {
            updateElo(mA.userScore, mB.userScore, 1.0);
        } else {
            updateElo(mA.userScore, mB.userScore, 0.0);
        }

        // Write back and recompute ranks
        if (!top100.updateMovie(i, mA)) return;
        if (!top100.updateMovie(j, mB)) return;
        top100.recomputeRanks();

        std::cout << "Updated scores: \n";
        std::cout << mA.title << ": " << static_cast<int>(mA.userScore) << "\n";
        std::cout << mB.title << ": " << static_cast<int>(mB.userScore) << "\n";
    }
}
