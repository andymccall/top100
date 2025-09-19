// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/removemovie.cpp
// Purpose: CLI handler to remove a movie by title/year.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "removemovie.h"
#include <iostream>
#include <string>

void removeMovie(Top100& top100) {
    std::string movie;
    std::cout << "Enter movie name to remove: ";
    std::cin.ignore();
    std::getline(std::cin, movie);
    top100.removeMovie(movie);
}
