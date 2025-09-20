// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/app.cpp
// Purpose: GTK application entry point.
//-------------------------------------------------------------------------------
#include "app.h"
#include "window.h"
#include <gtkmm/application.h>

int run_top100_gtk_app(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "uk.co.andymccall.top100");
    Top100GtkWindow win;
    return app->run(win);
}

int main(int argc, char* argv[]) { return run_top100_gtk_app(argc, argv); }
