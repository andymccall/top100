// SPDX-License-Identifier: Apache-2.0
// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/displaymenu.h
// Purpose: Declarations for menu display overloads.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once

#include <string>

// Show main menu depending on whether OMDb/BlueSky/Mastodon are configured.
// Backwards-compatible overload keeps the original behavior (BlueSky/Mastodon hidden).
void displayMenu(bool omdbEnabled);
void displayMenu(bool omdbEnabled, bool blueSkyEnabled);
void displayMenu(bool omdbEnabled, bool blueSkyEnabled, bool mastodonEnabled);
