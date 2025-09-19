// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/comparemovies.h
// Purpose: Declaration for Elo-style pairwise ranking routine.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once

#include "main.h"

// Prompt the user to compare two movies and update their scores/ranks
void compareMovies(Top100& top100);
