// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/image_export.h
// Purpose: Export a PNG summary image of the Top 100 grid with posters and titles.
// Language: C++17 (header)
//-------------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>

struct Movie;

/**
 * @brief Export a PNG image of the movie grid.
 *
 * The image contains a heading text, and a grid with up to 5x20 cells; each cell draws
 * a bold index number centered above a resized poster, and a single-line title with year.
 * Posters are fetched from Movie::posterUrl; when unavailable, a placeholder is drawn.
 *
 * @param movies Source movies; only the first 100 are used.
 * @param outPath Output PNG file path.
 * @param heading Heading text (default: "My Top 100 Movies").
 * @return true on success.
 */
bool exportTop100Image(const std::vector<Movie>& movies,
                       const std::string& outPath,
                       const std::string& heading = "My Top 100 Movies");
