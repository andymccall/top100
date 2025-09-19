// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: tests/test_sorting.cpp
// Purpose: Unit tests for sorting by year and alphabetical.
// Language: C++17 (Boost.Test)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE Top100Sorting
#include <boost/test/included/unit_test.hpp>
#include "top100.h"
#include "Movie.h"
#include <cstdio>

struct SortingFixture {
    std::string test_filename = "test_movies_sorting.json";
    SortingFixture() { std::remove(test_filename.c_str()); }
    ~SortingFixture() { std::remove(test_filename.c_str()); }
};

BOOST_FIXTURE_TEST_SUITE(SortingSuite, SortingFixture)

BOOST_AUTO_TEST_CASE(sort_by_year)
{
    Top100 top100(test_filename);
    top100.addMovie({"Movie C", 2005, "Dir"});
    top100.addMovie({"Movie A", 1999, "Dir"});
    top100.addMovie({"Movie B", 2010, "Dir"});

    auto byYear = top100.getMovies(SortOrder::BY_YEAR);
    BOOST_REQUIRE_EQUAL(byYear.size(), 3);
    BOOST_CHECK_EQUAL(byYear[0].title, "Movie A");
    BOOST_CHECK_EQUAL(byYear[1].title, "Movie C");
    BOOST_CHECK_EQUAL(byYear[2].title, "Movie B");
}

BOOST_AUTO_TEST_CASE(sort_alphabetical)
{
    Top100 top100(test_filename);
    top100.addMovie({"Zulu", 2005, "Dir"});
    top100.addMovie({"alpha", 2001, "Dir"});
    top100.addMovie({"Beta", 2002, "Dir"});

    auto alpha = top100.getMovies(SortOrder::ALPHABETICAL);
    BOOST_REQUIRE_EQUAL(alpha.size(), 3);
    BOOST_CHECK_EQUAL(alpha[0].title, "Beta");
    BOOST_CHECK_EQUAL(alpha[1].title, "Zulu");
    BOOST_CHECK_EQUAL(alpha[2].title, "alpha");
}

BOOST_AUTO_TEST_SUITE_END()
