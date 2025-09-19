// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/displaymenu.cpp
// Purpose: Renders the dynamic CLI menu based on enabled features.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "displaymenu.h"
#include <iostream>

void displayMenu(bool omdbEnabled) {
    displayMenu(omdbEnabled, false);
}

void displayMenu(bool omdbEnabled, bool blueSkyEnabled)
{
    displayMenu(omdbEnabled, blueSkyEnabled, false);
}

void displayMenu(bool omdbEnabled, bool blueSkyEnabled, bool mastodonEnabled)
{
    std::cout << "\n--- Top 100 Movies ---\n";
    std::cout << "1. Add a movie\n";
    std::cout << "2. Remove a movie\n";
    std::cout << "3. List movies\n";
    if (omdbEnabled) {
        std::cout << "4. Search and add from OMDb\n";
        std::cout << "5. Disable OMDb\n";
    } else {
        std::cout << "4. Configure OMDb API key\n";
        std::cout << "5. Set data file path\n";
    }
    std::cout << "6. View details\n";
    std::cout << "7. Compare two movies (rank)\n";
    if (blueSkyEnabled) {
        std::cout << "8. Post a movie to BlueSky\n";
    } else {
        std::cout << "8. Configure BlueSky account\n";
    }
    if (mastodonEnabled) {
        std::cout << "9. Post a movie to Mastodon\n";
    } else {
        std::cout << "9. Configure Mastodon account\n";
    }
    std::cout << "0. Edit post header/footer text\n";
    std::cout << "q. Quit\n";
    std::cout << "Enter your choice: ";
}