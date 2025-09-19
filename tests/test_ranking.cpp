// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: tests/test_ranking.cpp
// Purpose: Unit tests for ranking fields, recompute, and Elo update.
// Language: C++17 (Boost.Test)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE Top100Ranking
#include <boost/test/included/unit_test.hpp>
#include "top100.h"
#include "Movie.h"
#include <nlohmann/json.hpp>
#include <cstdio>

struct RankingFixture {
    std::string test_filename = "test_movies_ranking.json";
    RankingFixture() { std::remove(test_filename.c_str()); }
    ~RankingFixture() { std::remove(test_filename.c_str()); }
};

BOOST_FIXTURE_TEST_SUITE(RankingSuite, RankingFixture)

BOOST_AUTO_TEST_CASE(json_rank_fields_round_trip)
{
    Movie m;
    m.title = "Movie";
    m.year = 2000;
    m.director = "Dir";
    m.userScore = 1600.5;
    m.userRank = 7;

    nlohmann::json j = m;
    Movie out = j.get<Movie>();

    BOOST_CHECK_CLOSE(out.userScore, 1600.5, 1e-9);
    BOOST_CHECK_EQUAL(out.userRank, 7);
}

BOOST_AUTO_TEST_CASE(recompute_ranks_and_sorting)
{
    Top100 top100(test_filename);
    Movie a = {"Alpha", 2000, "Dir"};
    Movie b = {"Bravo", 2000, "Dir"};
    Movie c = {"Charlie", 2000, "Dir"};
    // Start all at default (unranked) and verify BY_USER_RANK falls back to alphabetical when all -1
    top100.addMovie(c);
    top100.addMovie(a);
    top100.addMovie(b);

    auto unrankedOrder = top100.getMovies(SortOrder::BY_USER_RANK);
    BOOST_REQUIRE_EQUAL(unrankedOrder.size(), 3);
    BOOST_CHECK_EQUAL(unrankedOrder[0].title, "Alpha");
    BOOST_CHECK_EQUAL(unrankedOrder[1].title, "Bravo");
    BOOST_CHECK_EQUAL(unrankedOrder[2].title, "Charlie");

    // Now set different scores and recompute ranks
    auto current = top100.getMovies(); // insertion order: c, a, b
    current[0].userScore = 1500.0; // Charlie
    current[1].userScore = 1700.0; // Alpha
    current[2].userScore = 1600.0; // Bravo
    BOOST_REQUIRE(top100.updateMovie(0, current[0]));
    BOOST_REQUIRE(top100.updateMovie(1, current[1]));
    BOOST_REQUIRE(top100.updateMovie(2, current[2]));

    top100.recomputeRanks();

    auto byRank = top100.getMovies(SortOrder::BY_USER_RANK);
    BOOST_REQUIRE_EQUAL(byRank.size(), 3);
    BOOST_CHECK_EQUAL(byRank[0].title, "Alpha");   // rank 1 (1700)
    BOOST_CHECK_EQUAL(byRank[1].title, "Bravo");   // rank 2 (1600)
    BOOST_CHECK_EQUAL(byRank[2].title, "Charlie"); // rank 3 (1500)

    auto byScore = top100.getMovies(SortOrder::BY_USER_SCORE);
    BOOST_REQUIRE_EQUAL(byScore.size(), 3);
    BOOST_CHECK_EQUAL(byScore[0].title, "Alpha");
    BOOST_CHECK_EQUAL(byScore[1].title, "Bravo");
    BOOST_CHECK_EQUAL(byScore[2].title, "Charlie");

    // Confirm ranks assigned 1..N
    BOOST_CHECK_EQUAL(byRank[0].userRank, 1);
    BOOST_CHECK_EQUAL(byRank[1].userRank, 2);
    BOOST_CHECK_EQUAL(byRank[2].userRank, 3);
}

// Deterministic Elo update check mirroring the CLI logic
static void testUpdateElo(double& a, double& b, double scoreA, double k = 32.0) {
    const double qa = std::pow(10.0, a / 400.0);
    const double qb = std::pow(10.0, b / 400.0);
    const double ea = qa / (qa + qb);
    const double eb = qb / (qa + qb);
    a = a + k * (scoreA - ea);
    b = b + k * ((1.0 - scoreA) - eb);
}

BOOST_AUTO_TEST_CASE(elo_update_changes_scores_and_order)
{
    Top100 top100(test_filename);
    Movie A = {"A", 2000, "Dir"};
    Movie B = {"B", 2000, "Dir"};
    // Start both at 1500 (defaults)
    top100.addMovie(A);
    top100.addMovie(B);

    // Simulate A wins with K=32 and expected Elo formula
    double sa = 1500.0, sb = 1500.0;
    testUpdateElo(sa, sb, 1.0, 32.0);

    auto cur = top100.getMovies();
    cur[0].userScore = sa;
    cur[1].userScore = sb;
    BOOST_REQUIRE(top100.updateMovie(0, cur[0]));
    BOOST_REQUIRE(top100.updateMovie(1, cur[1]));
    top100.recomputeRanks();

    auto ranked = top100.getMovies(SortOrder::BY_USER_RANK);
    BOOST_REQUIRE_EQUAL(ranked.size(), 2);
    // A should now rank above B and have a slightly higher score
    BOOST_CHECK_EQUAL(ranked[0].title, "A");
    BOOST_CHECK_GT(ranked[0].userScore, ranked[1].userScore);

    // Validate numerical values within a tight tolerance
    BOOST_CHECK_CLOSE(ranked[0].userScore, sa, 1e-6);
    BOOST_CHECK_CLOSE(ranked[1].userScore, sb, 1e-6);
}

BOOST_AUTO_TEST_SUITE_END()
