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

/** Initial application window width in pixels. */
constexpr int kInitialWidth = 1024;
/** Initial application window height in pixels. */
constexpr int kInitialHeight = 768;

/** Poster max width as a fraction of the details pane width (0.0–1.0). */
constexpr double kPosterMaxWidthRatio = 0.90;  // 90% of details pane width
/** Poster max height as a fraction of the details pane height (0.0–1.0). */
constexpr double kPosterMaxHeightRatio = 0.90; // 90% of details pane height

// Shared spacing and margins for consistent look-and-feel
/** Small spacing between controls in pixels. */
constexpr int kSpacingSmall = 6;      // small spacing between controls
/** Medium spacing value in pixels. */
constexpr int kSpacingMedium = 10;    // medium spacing (unused yet)
/** Large spacing for dialog section separation in pixels. */
constexpr int kSpacingLarge = 16;     // large spacing for dialog sections
/** Inner padding inside frames/groups in pixels. */
constexpr int kGroupPadding = 8;      // inner padding inside frames/groups
/** Minimum label width for aligned form layouts in pixels. */
constexpr int kLabelMinWidth = 90;    // minimum width for labels in forms

} // namespace ui_constants
