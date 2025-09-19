// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/common/strings.h
// Purpose: Shared UI strings for Qt/KDE.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
// UI string constants shared across Qt Widgets and KDE UIs.
#pragma once

namespace ui_strings {
// Application names/titles
inline constexpr const char* kAppName = "Top100";
inline constexpr const char* kAppDisplayName = "Top100 — Your Personal Movie List";

// Qt UI
inline constexpr const char* kQtWindowTitle = "Top100 — Qt UI";
inline constexpr const char* kQtHelloText = "Hello from Qt UI!";

// KDE UI
inline constexpr const char* kKdeWindowTitle = "Top100 — KDE UI";
inline constexpr const char* kKdeHelloText = "Hello from KDE/Kirigami UI!";

// Common menu labels/actions
inline constexpr const char* kMenuFile   = "File";
inline constexpr const char* kMenuHelp   = "Help";
inline constexpr const char* kActionQuit = "Quit";
inline constexpr const char* kActionAbout = "About";

// About dialog content
inline constexpr const char* kAboutDialogText = "Top 100 by Andy McCall";
}
