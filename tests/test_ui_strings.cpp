// SPDX-License-Identifier: Apache-2.0
#define BOOST_TEST_MODULE UiStringsSuite
#include <boost/test/unit_test.hpp>
#include "../ui/common/strings.h"

BOOST_AUTO_TEST_CASE(constants_have_expected_values) {
    using namespace ui_strings;
    BOOST_CHECK_EQUAL(std::string{kAppName}, "Top100");
    BOOST_CHECK_EQUAL(std::string{kAppDisplayName}, "Top100 — Your Personal Movie List");
    BOOST_CHECK_EQUAL(std::string{kQtWindowTitle}, "Top100 — Qt UI");
    BOOST_CHECK_EQUAL(std::string{kQtHelloText}, "Hello from Qt UI!");
    BOOST_CHECK_EQUAL(std::string{kKdeWindowTitle}, "Top100 — KDE UI");
    BOOST_CHECK_EQUAL(std::string{kKdeHelloText}, "Hello from KDE/Kirigami UI!");
}
