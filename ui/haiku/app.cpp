// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/haiku/app.cpp
// Purpose: Haiku (BeAPI) application entry point implementation.
//-------------------------------------------------------------------------------
#include "app.h"

#ifdef __HAIKU__
#include "window.h"

Top100HaikuApp::Top100HaikuApp() : BApplication("application/x-vnd.andymccall.top100") {}

void Top100HaikuApp::ReadyToRun() {
    auto *win = new Top100HaikuWindow();
    win->Show();
}

int main() {
    Top100HaikuApp app;
    app.Run();
    return 0;
}

#endif // __HAIKU__
