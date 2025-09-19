// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/dup_policy.h
// Purpose: Duplicate handling policy based on environment variable.
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once
#include <cstdlib>
#include <string>

enum class DuplicatePolicy {
    Prompt,     // ask the user (default)
    PreferOmdb, // overwrite with OMDb data automatically
    PreferManual, // keep manual entry automatically
    Skip        // never overwrite automatically
};

inline DuplicatePolicy getDuplicatePolicy() {
    const char* val = std::getenv("TOP100_DUPLICATE_POLICY");
    if (!val) return DuplicatePolicy::Prompt;
    std::string s(val);
    for (auto& c : s) c = std::tolower(c);
    if (s == "prefer_omdb" || s == "omdb") return DuplicatePolicy::PreferOmdb;
    if (s == "prefer_manual" || s == "manual") return DuplicatePolicy::PreferManual;
    if (s == "skip") return DuplicatePolicy::Skip;
    return DuplicatePolicy::Prompt;
}
