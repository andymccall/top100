// SPDX-License-Identifier: Apache-2.0
// Basic test validating that a .db file is created and persists a record when SQLite is active.
#define BOOST_TEST_MODULE SQLiteBackend
#include <boost/test/included/unit_test.hpp>
#include "top100.h"
#include <filesystem>
#include <cstdio>

namespace fs = std::filesystem;

BOOST_AUTO_TEST_CASE(create_and_persist_db_file)
{
#ifndef TOP100_NO_SQLITE
    const char* path = "temp_top100_test.db";
    std::remove(path);
    {
        Top100 t(path);
        Movie m{"Test Movie", 2024, "Dir"};
        t.addMovie(m);
        t.recomputeRanks();
    } // destructor saves & closes
    BOOST_CHECK(fs::exists(path));
    // Re-open and ensure movie loads back
    Top100 reopened(path);
    auto mv = reopened.getMovies();
    BOOST_REQUIRE_EQUAL(mv.size(), 1);
    BOOST_CHECK_EQUAL(mv[0].title, "Test Movie");
    std::remove(path);
#else
    BOOST_TEST_MESSAGE("SQLite not available; skipping DB-specific test (TOP100_NO_SQLITE defined)");
#endif
}
