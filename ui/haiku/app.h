// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/haiku/app.h
// Purpose: Haiku (BeAPI) application entry point declaration.
//-------------------------------------------------------------------------------
#pragma once

#ifdef __HAIKU__
#include <Application.h>

class Top100HaikuApp : public BApplication {
public:
    Top100HaikuApp();
    void ReadyToRun() override;
};

#endif // __HAIKU__
