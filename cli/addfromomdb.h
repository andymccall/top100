//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/addfromomdb.h
// Purpose: Declaration for OMDb search-and-add CLI routine.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#pragma once
#include "top100.h"

void addFromOmdb(Top100& top100, const std::string& apiKey);
