// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/toolbar.cpp
// Purpose: Toolbar construction for GTK window with themed icons and text.
//-------------------------------------------------------------------------------
#include "toolbar.h"

void Top100GtkWindow::build_toolbar() {
    toolbar_.set_toolbar_style(Gtk::TOOLBAR_BOTH); // text under icons
    toolbar_.set_icon_size(Gtk::ICON_SIZE_LARGE_TOOLBAR);

    // Add
    btn_add_ = Gtk::manage(new Gtk::ToolButton());
    btn_add_->set_label("Add");
    auto img_add = Gtk::manage(new Gtk::Image());
    img_add->set_from_icon_name("list-add", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    btn_add_->set_icon_widget(*img_add);
    btn_add_->set_is_important(true);
    toolbar_.append(*btn_add_);

    // Delete
    btn_delete_ = Gtk::manage(new Gtk::ToolButton());
    btn_delete_->set_label("Delete");
    auto img_del = Gtk::manage(new Gtk::Image());
    img_del->set_from_icon_name("edit-delete", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    btn_delete_->set_icon_widget(*img_del);
    toolbar_.append(*btn_delete_);

    // Refresh
    btn_refresh_ = Gtk::manage(new Gtk::ToolButton());
    btn_refresh_->set_label("Refresh");
    auto img_ref = Gtk::manage(new Gtk::Image());
    img_ref->set_from_icon_name("view-refresh", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    btn_refresh_->set_icon_widget(*img_ref);
    toolbar_.append(*btn_refresh_);

    // Rank
    auto btn_rank = Gtk::manage(new Gtk::ToolButton());
    btn_rank->set_label("Rank");
    auto img_rank = Gtk::manage(new Gtk::Image());
    img_rank->set_from_icon_name("favorites", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    btn_rank->set_icon_widget(*img_rank);
    toolbar_.append(*btn_rank);

    // Update (OMDb)
    btn_update_ = Gtk::manage(new Gtk::ToolButton());
    btn_update_->set_label("Update (OMDb)");
    auto img_upd = Gtk::manage(new Gtk::Image());
    // Use system-software-update if available, falls back to view-refresh theme appearance otherwise
    img_upd->set_from_icon_name("system-software-update", Gtk::ICON_SIZE_LARGE_TOOLBAR);
    btn_update_->set_icon_widget(*img_upd);
    toolbar_.append(*btn_update_);

    // Wire handlers
    btn_add_->signal_clicked().connect(sigc::mem_fun(*this, &Top100GtkWindow::on_add_movie));
    btn_delete_->signal_clicked().connect(sigc::mem_fun(*this, &Top100GtkWindow::on_delete_current));
    btn_refresh_->signal_clicked().connect([this]() {
        reload_model(current_selected_imdb());
        update_status_movie_count();
    });
    btn_update_->signal_clicked().connect(sigc::mem_fun(*this, &Top100GtkWindow::on_update_current));
    btn_rank->signal_clicked().connect([this]() {
        // Lazy include to avoid header in toolbar
        extern void gtk_open_rank_dialog(Gtk::Window& parent);
        gtk_open_rank_dialog(*this);
    });
}
