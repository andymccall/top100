//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: tests/test_top100.cpp
// Purpose: Unit tests for Top100 core behaviors.
// Language: C++17 (Boost.Test)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE Top100Tests
#include <boost/test/included/unit_test.hpp>
#include "top100.h"
#include "Movie.h"
#include <fstream>
#include <cstdio>

// Test fixture for creating and cleaning up a test database file
struct TestFixture {
    std::string test_filename = "test_movies.json";

    TestFixture() {
        // Ensure no old test file exists
        std::remove(test_filename.c_str());
    }

    ~TestFixture() {
        // Clean up the test file after the test runs
        std::remove(test_filename.c_str());
    }
};

BOOST_FIXTURE_TEST_SUITE(Top100TestSuite, TestFixture)

BOOST_AUTO_TEST_CASE(add_and_get_movie)
{
    Top100 top100(test_filename);
    Movie movie = {"The Shawshank Redemption", 1994, "Frank Darabont"};
    top100.addMovie(movie);
    
    std::vector<Movie> movies = top100.getMovies();
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
    
    std::vector<Movie> movies = top100.getMovies();
    BOOST_CHECK_EQUAL(movies.size(), 1);
    BOOST_CHECK_EQUAL(movies[0].title, "The Dark Knight");
}

BOOST_AUTO_TEST_CASE(save_and_load)
{
    // Create a Top100 instance in a scope to trigger its destructor (and save)
    {
        Top100 top100(test_filename);
        Movie movie = {"Pulp Fiction", 1994, "Quentin Tarantino"};
        top100.addMovie(movie);
    }

    // Create a new instance that should load from the file
    Top100 new_top100(test_filename);
    std::vector<Movie> movies = new_top100.getMovies();
    
    BOOST_CHECK_EQUAL(movies.size(), 1);
    BOOST_CHECK_EQUAL(movies[0].title, "Pulp Fiction");
    BOOST_CHECK_EQUAL(movies[0].director, "Quentin Tarantino");
}

BOOST_AUTO_TEST_SUITE_END()