// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
/*! \file ui/common/constants.h
    \brief Shared UI constants for both Qt Widgets and KDE/Kirigami apps.

    Centralizes values like initial window size and poster sizing ratios so
    both UIs remain in parity by referencing one place.
*/
// Top100 — Your Personal Movie List
//
// File: ui/common/constants.h
// Purpose: Shared UI constants (initial sizes, poster ratios) used by both UIs.
// Language: C++17 (header-only)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------

#pragma once

namespace ui_constants {

// Initial application window size
constexpr int kInitialWidth = 1024;
constexpr int kInitialHeight = 768;

// Poster image maximum size as a fraction of the details pane size (make poster larger)
constexpr double kPosterMaxWidthRatio = 0.90;  // 90% of details pane width
constexpr double kPosterMaxHeightRatio = 0.90; // 90% of details pane height

// Shared spacing and margins for consistent look-and-feel
constexpr int kSpacingSmall = 6;      // small spacing between controls
constexpr int kSpacingMedium = 10;    // medium spacing (unused yet)
constexpr int kGroupPadding = 8;      // inner padding inside frames/groups
constexpr int kLabelMinWidth = 90;    // minimum width for labels in forms

} // namespace ui_constants
