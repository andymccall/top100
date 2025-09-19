// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: tests/test_config_utils.cpp
// Purpose: Unit tests for configuration helper functions.
// Language: C++17 (Boost.Test)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE Top100ConfigUtils
#include <boost/test/included/unit_test.hpp>
#include "config_utils.h"
#include <cstdlib>
#include <cstdio>

struct CfgUtilFixture {
    std::string tmpCfg;
    CfgUtilFixture() {
        tmpCfg = "test_cfg_utils.json";
        setenv("TOP100_CONFIG_PATH", tmpCfg.c_str(), 1);
        std::remove(tmpCfg.c_str());
    }
    ~CfgUtilFixture() {
        std::remove(tmpCfg.c_str());
        unsetenv("TOP100_CONFIG_PATH");
    }
};

BOOST_FIXTURE_TEST_SUITE(ConfigUtilsSuite, CfgUtilFixture)

BOOST_AUTO_TEST_CASE(fails_when_verifier_rejects)
{
    AppConfig cfg = loadConfig();
    bool ok = configureOmdb(cfg, "bad", [](const std::string&){ return false; });
    BOOST_CHECK(!ok);
    AppConfig again = loadConfig();
    BOOST_CHECK_EQUAL(again.omdbEnabled, false);
    BOOST_CHECK(again.omdbApiKey.empty());
}

BOOST_AUTO_TEST_CASE(succeeds_and_persists_when_verifier_accepts)
{
    AppConfig cfg = loadConfig();
    bool ok = configureOmdb(cfg, "goodkey", [](const std::string& k){ return k == "goodkey"; });
    BOOST_CHECK(ok);
    AppConfig again = loadConfig();
    BOOST_CHECK_EQUAL(again.omdbEnabled, true);
    BOOST_CHECK_EQUAL(again.omdbApiKey, std::string("goodkey"));
}

BOOST_AUTO_TEST_CASE(set_data_file_and_disable_omdb)
{
    AppConfig cfg = loadConfig();
    bool ok = setDataFile(cfg, "/tmp/top100_test_dir/my_top100.json");
    BOOST_CHECK(ok);
    AppConfig again = loadConfig();
    BOOST_CHECK_EQUAL(again.dataFile, std::string("/tmp/top100_test_dir/my_top100.json"));

    // Now disable OMDb
    again.omdbEnabled = true; // simulate
    again.omdbApiKey = "x";
    saveConfig(again);
    disableOmdb(again);
    AppConfig final = loadConfig();
    BOOST_CHECK_EQUAL(final.omdbEnabled, false);
    BOOST_CHECK(final.omdbApiKey.empty());
}

BOOST_AUTO_TEST_SUITE_END()
