// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/common/Top100ListModel.cpp
// Purpose: Implementation unit for Top100ListModel (shared by Qt and KDE UIs).
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
// Implementation unit for Top100ListModel. Most logic lives inline in the header for brevity.
#include "Top100ListModel.h"

namespace {
// Simple Elo update helper
static void eloUpdate(double &a, double &b, double scoreA, double k = 32.0) {
	const double qa = std::pow(10.0, a / 400.0);
	const double qb = std::pow(10.0, b / 400.0);
	const double ea = qa / (qa + qb);
	const double eb = qb / (qa + qb);
	a = a + k * (scoreA - ea);
	b = b + k * ((1.0 - scoreA) - eb);
}
}

bool Top100ListModel::recordPairwiseResult(int leftRow, int rightRow, int winner) {
	if (leftRow < 0 || rightRow < 0 || leftRow >= rowCount() || rightRow >= rowCount() || leftRow == rightRow)
		return false;
	if (winner == -1) return true; // pass, do nothing

	// Work on a fresh Top100 to persist changes by index mapping.
	try {
		AppConfig cfg = loadConfig();
		Top100 list(cfg.dataFile);
		// We need the current displayed order mapping to underlying indices.
		// Since Top100::getMovies returns a copy, we'll reconstruct index mapping by imdbID.
		auto current = list.getMovies(currentOrder_);
		auto getIndexByImdb = [&](const std::string &id) -> int {
			return list.findIndexByImdbId(id);
		};

		// Guard against out-of-range
		if (leftRow >= static_cast<int>(current.size()) || rightRow >= static_cast<int>(current.size()))
			return false;

		Movie left = current[static_cast<size_t>(leftRow)];
		Movie right = current[static_cast<size_t>(rightRow)];

		// Elo update: scoreA = 1.0 if left wins else 0.0
		double a = left.userScore;
		double b = right.userScore;
		double scoreA = (winner == 1) ? 1.0 : 0.0;
		eloUpdate(a, b, scoreA, 32.0);
		left.userScore = a;
		right.userScore = b;

		int li = getIndexByImdb(left.imdbID);
		int ri = getIndexByImdb(right.imdbID);
		if (li < 0 || ri < 0) return false;
		bool ok1 = list.updateMovie(static_cast<size_t>(li), left);
		bool ok2 = list.updateMovie(static_cast<size_t>(ri), right);
		if (!ok1 || !ok2) return false;
		list.recomputeRanks();
	} catch (...) {
		return false;
	}

	// Refresh model view and keep sort order
	reload();
	return true;
}
