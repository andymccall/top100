// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: tests/test_menu.cpp
// Purpose: Unit tests for dynamic CLI menu content.
// Language: C++17 (Boost.Test)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE Top100Menu
#include <boost/test/included/unit_test.hpp>
#include "cli/displaymenu.h"
#include <sstream>
#include <iostream>

BOOST_AUTO_TEST_CASE(show_config_when_omdb_disabled)
{
    std::ostringstream oss;
    auto* old = std::cout.rdbuf(oss.rdbuf());
    displayMenu(false);
    std::cout.rdbuf(old);
    auto out = oss.str();
    BOOST_CHECK(out.find("Configure OMDb API key") != std::string::npos);
    BOOST_CHECK(out.find("Set data file path") != std::string::npos);
    BOOST_CHECK(out.find("Search and add from OMDb") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_omdb_when_enabled)
{
    std::ostringstream oss;
    auto* old = std::cout.rdbuf(oss.rdbuf());
    displayMenu(true);
    std::cout.rdbuf(old);
    auto out = oss.str();
    BOOST_CHECK(out.find("Search and add from OMDb") != std::string::npos);
    BOOST_CHECK(out.find("Disable OMDb") != std::string::npos);
    BOOST_CHECK(out.find("Configure OMDb API key") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_bsky_when_disabled)
{
    std::ostringstream oss;
    auto* old = std::cout.rdbuf(oss.rdbuf());
    displayMenu(true, false);
    std::cout.rdbuf(old);
    auto out = oss.str();
    BOOST_CHECK(out.find("Configure BlueSky account") != std::string::npos);
    BOOST_CHECK(out.find("Post a movie to BlueSky") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_bsky_when_enabled)
{
    std::ostringstream oss;
    auto* old = std::cout.rdbuf(oss.rdbuf());
    displayMenu(true, true);
    std::cout.rdbuf(old);
    auto out = oss.str();
    BOOST_CHECK(out.find("Post a movie to BlueSky") != std::string::npos);
    BOOST_CHECK(out.find("Configure BlueSky account") == std::string::npos);
}

BOOST_AUTO_TEST_CASE(show_mastodon_states_and_header_footer_option)
{
    {
        std::ostringstream oss;
        auto* old = std::cout.rdbuf(oss.rdbuf());
        displayMenu(true, true, false);
        std::cout.rdbuf(old);
        auto out = oss.str();
        BOOST_CHECK(out.find("Configure Mastodon account") != std::string::npos);
        BOOST_CHECK(out.find("Post a movie to Mastodon") == std::string::npos);
        BOOST_CHECK(out.find("Edit post header/footer text") != std::string::npos);
    }
    {
        std::ostringstream oss;
        auto* old = std::cout.rdbuf(oss.rdbuf());
        displayMenu(true, true, true);
        std::cout.rdbuf(old);
        auto out = oss.str();
        BOOST_CHECK(out.find("Post a movie to Mastodon") != std::string::npos);
        BOOST_CHECK(out.find("Configure Mastodon account") == std::string::npos);
        BOOST_CHECK(out.find("Edit post header/footer text") != std::string::npos);
    }
}
