//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: tests/test_core.cpp
// Purpose: Unit tests for core add/remove/save/load behavior.
// Language: C++17 (Boost.Test)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE Top100Core
#include <boost/test/included/unit_test.hpp>
#include "top100.h"
#include "Movie.h"
#include <cstdio>

struct CoreFixture {
    std::string test_filename = "test_movies_core.json";
    CoreFixture() { std::remove(test_filename.c_str()); }
    ~CoreFixture() { std::remove(test_filename.c_str()); }
};

BOOST_FIXTURE_TEST_SUITE(CoreSuite, CoreFixture)

BOOST_AUTO_TEST_CASE(add_and_get_movie)
{
    Top100 top100(test_filename);
    Movie movie = {"The Shawshank Redemption", 1994, "Frank Darabont"};
    top100.addMovie(movie);

    auto movies = top100.getMovies();
    BOOST_CHECK_EQUAL(movies.size(), 1);
    BOOST_CHECK_EQUAL(movies[0].title, "The Shawshank Redemption");
    BOOST_CHECK_EQUAL(movies[0].year, 1994);
}

BOOST_AUTO_TEST_CASE(remove_movie)
{
    Top100 top100(test_filename);
    Movie movie1 = {"The Godfather", 1972, "Francis Ford Coppola"};
    Movie movie2 = {"The Dark Knight", 2008, "Christopher Nolan"};
    top100.addMovie(movie1);
    top100.addMovie(movie2);

    top100.removeMovie("The Godfather");
    auto movies = top100.getMovies();
    BOOST_CHECK_EQUAL(movies.size(), 1);
    BOOST_CHECK_EQUAL(movies[0].title, "The Dark Knight");
}

BOOST_AUTO_TEST_CASE(save_and_load)
{
    {
        Top100 top100(test_filename);
        Movie movie = {"Pulp Fiction", 1994, "Quentin Tarantino"};
        top100.addMovie(movie);
    }

    Top100 new_top100(test_filename);
    auto movies = new_top100.getMovies();
    BOOST_CHECK_EQUAL(movies.size(), 1);
    BOOST_CHECK_EQUAL(movies[0].title, "Pulp Fiction");
    BOOST_CHECK_EQUAL(movies[0].director, "Quentin Tarantino");
}

BOOST_AUTO_TEST_SUITE_END()
