//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: tests/test_config.cpp
// Purpose: Unit tests for AppConfig defaults and round-trip.
// Language: C++17 (Boost.Test)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE Top100Config
#include <boost/test/included/unit_test.hpp>
#include "config.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>

struct ConfigFixture {
    std::string tmpCfg;
    ConfigFixture() {
        // Use a temp config path via env override
        tmpCfg = "test_config.json";
        setenv("TOP100_CONFIG_PATH", tmpCfg.c_str(), 1);
        std::remove(tmpCfg.c_str());
    }
    ~ConfigFixture() {
        std::remove(tmpCfg.c_str());
        unsetenv("TOP100_CONFIG_PATH");
    }
};

BOOST_FIXTURE_TEST_SUITE(ConfigSuite, ConfigFixture)

BOOST_AUTO_TEST_CASE(create_default_when_missing)
{
    // File is removed in fixture; loadConfig should create defaults
    AppConfig cfg = loadConfig();
    BOOST_CHECK(!cfg.dataFile.empty());
    BOOST_CHECK_EQUAL(cfg.omdbEnabled, false);
    BOOST_CHECK(cfg.omdbApiKey.empty());
    // BlueSky defaults
    BOOST_CHECK_EQUAL(cfg.blueSkyEnabled, false);
    BOOST_CHECK(cfg.blueSkyIdentifier.empty());
    BOOST_CHECK(cfg.blueSkyAppPassword.empty());
    BOOST_CHECK(!cfg.blueSkyService.empty());
    // Post header/footer defaults
    BOOST_CHECK(cfg.postHeaderText == "I'd like to share one of my top 100 #movies!");
    BOOST_CHECK(cfg.postFooterText == "Posted with Top 100!");
    BOOST_CHECK(!cfg.mastodonEnabled);
    BOOST_CHECK(cfg.mastodonInstance == "https://mastodon.social");
    BOOST_CHECK(cfg.mastodonAccessToken.empty());
    // and a file should now exist
    std::ifstream in(tmpCfg);
    BOOST_CHECK(in.is_open());
}

BOOST_AUTO_TEST_CASE(load_save_round_trip)
{
    AppConfig cfg = loadConfig();
    cfg.dataFile = "/tmp/custom_top100.json";
    cfg.omdbEnabled = true;
    cfg.omdbApiKey = "abc123";
    cfg.blueSkyEnabled = true;
    cfg.blueSkyIdentifier = "alice.bsky.social";
    cfg.blueSkyAppPassword = "app-xxxx-xxxx";
    cfg.blueSkyService = "https://bsky.social";
    cfg.postHeaderText = "Sharing a favorite film:";
    cfg.postFooterText = "— via Top100";
    cfg.mastodonEnabled = true;
    cfg.mastodonInstance = "https://mastodon.example";
    cfg.mastodonAccessToken = "token123";
    saveConfig(cfg);

    AppConfig again = loadConfig();
    BOOST_CHECK_EQUAL(again.dataFile, cfg.dataFile);
    BOOST_CHECK_EQUAL(again.omdbEnabled, true);
    BOOST_CHECK_EQUAL(again.omdbApiKey, "abc123");
    BOOST_CHECK_EQUAL(again.blueSkyEnabled, true);
    BOOST_CHECK_EQUAL(again.blueSkyIdentifier, "alice.bsky.social");
    BOOST_CHECK_EQUAL(again.blueSkyAppPassword, "app-xxxx-xxxx");
    BOOST_CHECK_EQUAL(again.blueSkyService, "https://bsky.social");
    BOOST_CHECK_EQUAL(again.postHeaderText, "Sharing a favorite film:");
    BOOST_CHECK_EQUAL(again.postFooterText, "— via Top100");
    BOOST_CHECK(again.mastodonEnabled);
    BOOST_CHECK_EQUAL(again.mastodonInstance, "https://mastodon.example");
    BOOST_CHECK_EQUAL(again.mastodonAccessToken, "token123");
}

BOOST_AUTO_TEST_SUITE_END()
