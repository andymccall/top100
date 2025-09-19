// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: tests/test_find_replace.cpp
// Purpose: Unit tests for find/replace helpers.
// Language: C++17 (Boost.Test)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE FindReplace
#include <boost/test/included/unit_test.hpp>
#include "top100.h"
#include "Movie.h"
#include <cstdio>

struct FRFixture {
    std::string test_filename = "test_movies_fr.json";
    FRFixture() { std::remove(test_filename.c_str()); }
    ~FRFixture() { std::remove(test_filename.c_str()); }
};

BOOST_FIXTURE_TEST_SUITE(FindReplaceSuite, FRFixture)

BOOST_AUTO_TEST_CASE(find_by_imdb_and_title_year)
{
    Top100 top100(test_filename);
    Movie a = {"Heat", 1995, "Michael Mann"};
    a.imdbID = "tt0113277";
    Movie b = {"Heat", 1986, "Larry Pierce"};
    top100.addMovie(a);
    top100.addMovie(b);

    BOOST_CHECK_EQUAL(top100.findIndexByImdbId("tt0113277"), 0);
    BOOST_CHECK_EQUAL(top100.findIndexByImdbId(""), -1);
    BOOST_CHECK_EQUAL(top100.findIndexByTitleYear("Heat", 1995), 0);
    BOOST_CHECK_EQUAL(top100.findIndexByTitleYear("Heat", 1986), 1);
    BOOST_CHECK_EQUAL(top100.findIndexByTitleYear("Unknown", 2000), -1);
}

BOOST_AUTO_TEST_CASE(replace_movie)
{
    Top100 top100(test_filename);
    Movie a = {"Blade Runner", 1982, "Ridley Scott"};
    top100.addMovie(a);
    size_t idx = 0;
    Movie upd = a;
    upd.director = "R. Scott";
    top100.replaceMovie(idx, upd);
    auto movies = top100.getMovies();
    BOOST_REQUIRE_EQUAL(movies.size(), 1);
    BOOST_CHECK_EQUAL(movies[0].director, "R. Scott");
}

BOOST_AUTO_TEST_SUITE_END()
