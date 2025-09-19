// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: tests/test_movie_json.cpp
// Purpose: Unit tests for Movie JSON round-trip including plot fields.
// Language: C++17 (Boost.Test)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE MovieJson
#include <boost/test/included/unit_test.hpp>
#include "Movie.h"
#include <nlohmann/json.hpp>

BOOST_AUTO_TEST_CASE(json_round_trip_with_ratings)
{
    Movie m;
    m.title = "Inception";
    m.year = 2010;
    m.director = "Christopher Nolan";
    m.imdbRating = 8.8;
    m.metascore = 74;
    m.rottenTomatoes = 87;
    m.actors = {"Leonardo DiCaprio", "Joseph Gordon-Levitt"};
    m.genres = {"Action", "Sci-Fi"};
    m.runtimeMinutes = 148;
    m.countries = {"USA", "UK"};
    m.posterUrl = "http://example.com/poster.jpg";
    m.plotShort = "A thief who steals corporate secrets through dream-sharing tech.";
    m.plotFull = "A thief who steals corporate secrets through the use of dream-sharing technology is given the inverse task of planting an idea.";
    m.source = "omdb";
    m.imdbID = "tt1375666";

    nlohmann::json j = m;
    Movie out = j.get<Movie>();

    BOOST_CHECK_EQUAL(out.title, m.title);
    BOOST_CHECK_EQUAL(out.year, m.year);
    BOOST_CHECK_EQUAL(out.director, m.director);
    BOOST_CHECK_CLOSE(out.imdbRating, m.imdbRating, 1e-9);
    BOOST_CHECK_EQUAL(out.metascore, m.metascore);
    BOOST_CHECK_EQUAL(out.rottenTomatoes, m.rottenTomatoes);
    BOOST_CHECK_EQUAL(out.runtimeMinutes, m.runtimeMinutes);
    BOOST_CHECK_EQUAL(out.posterUrl, m.posterUrl);
    BOOST_CHECK_EQUAL(out.source, m.source);
    BOOST_CHECK_EQUAL(out.imdbID, m.imdbID);
    BOOST_CHECK_EQUAL(out.plotShort, m.plotShort);
    BOOST_CHECK_EQUAL(out.plotFull, m.plotFull);
    BOOST_REQUIRE_EQUAL(out.actors.size(), m.actors.size());
    BOOST_REQUIRE_EQUAL(out.genres.size(), m.genres.size());
    BOOST_REQUIRE_EQUAL(out.countries.size(), m.countries.size());
}
