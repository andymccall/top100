// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: tests/test_ui_constants.cpp
// Purpose: Sanity checks for UI constants used across UIs.
//-------------------------------------------------------------------------------
#define BOOST_TEST_MODULE UIConstantsSuite
#include <boost/test/included/unit_test.hpp>

#include "ui/common/constants.h"

BOOST_AUTO_TEST_CASE(window_sizes_positive) {
    BOOST_CHECK_GT(ui_constants::kInitialWidth, 100);
    BOOST_CHECK_GT(ui_constants::kInitialHeight, 100);
}

BOOST_AUTO_TEST_CASE(layout_spacings_reasonable) {
    using namespace ui_constants;
    BOOST_CHECK_GE(kSpacingSmall, 0);
    BOOST_CHECK_GE(kGroupPadding, 0);
    BOOST_CHECK_GE(kLabelMinWidth, 40);
    BOOST_CHECK_GT(kPosterMaxWidthRatio, 0.0);
    BOOST_CHECK_GT(kPosterMaxHeightRatio, 0.0);
    BOOST_CHECK_LE(kPosterMaxWidthRatio, 1.0);
    BOOST_CHECK_LE(kPosterMaxHeightRatio, 1.0);
}
