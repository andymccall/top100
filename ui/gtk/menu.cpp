// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/menu.cpp
// Purpose: Menubar construction and menu handlers for GTK window.
//-------------------------------------------------------------------------------
#include "menu.h"

#include <gdk/gdkkeysyms.h>
#include "../common/strings.h"

using namespace ui_strings;

void Top100GtkWindow::build_menubar() {
    // File menu
    auto file_item = Gtk::manage(new Gtk::MenuItem("_" + std::string(kMenuFile), true));
    auto file_menu = Gtk::manage(new Gtk::Menu());
    menubar_.append(*file_item);
    file_item->set_submenu(*file_menu);

    auto quit_item = Gtk::manage(new Gtk::MenuItem(kActionQuit));
    file_menu->append(*quit_item);
    quit_item->signal_activate().connect(sigc::mem_fun(*this, &Top100GtkWindow::on_menu_quit));
    quit_item->add_accelerator("activate", accel_group_, GDK_KEY_q, Gdk::CONTROL_MASK, Gtk::ACCEL_VISIBLE);

    // Help menu
    auto help_item = Gtk::manage(new Gtk::MenuItem("_" + std::string(kMenuHelp), true));
    auto help_menu = Gtk::manage(new Gtk::Menu());
    menubar_.append(*help_item);
    help_item->set_submenu(*help_menu);

    auto about_item = Gtk::manage(new Gtk::MenuItem(kActionAbout));
    help_menu->append(*about_item);
    about_item->signal_activate().connect(sigc::mem_fun(*this, &Top100GtkWindow::on_menu_about));
    about_item->add_accelerator("activate", accel_group_, GDK_KEY_F1, static_cast<Gdk::ModifierType>(0), Gtk::ACCEL_VISIBLE);
}

void Top100GtkWindow::on_menu_quit() { hide(); }

void Top100GtkWindow::on_menu_about() {
    Gtk::MessageDialog dlg(*this, kAppDisplayName, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
    dlg.set_secondary_text(kAboutDialogText);
    dlg.run();
}
