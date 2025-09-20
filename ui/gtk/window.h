// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/window.h
// Purpose: Declaration of the GTK (gtkmm) main window and its components.
// Language: C++17 (gtkmm-3)
//-------------------------------------------------------------------------------
#pragma once

#include <gtkmm.h>
#include <glibmm/refptr.h>
#include <string>
#include <vector>

namespace Gdk { class Pixbuf; }

class Top100GtkWindow : public Gtk::Window {
public:
    Top100GtkWindow();

private:
    // Model columns for list
    class Columns : public Gtk::TreeModel::ColumnRecord {
    public:
        Columns() { add(text); add(index); add(imdb); }
        Gtk::TreeModelColumn<Glib::ustring> text;
        Gtk::TreeModelColumn<int> index;        // row index for retrieval
        Gtk::TreeModelColumn<Glib::ustring> imdb; // imdb id
    } columns_;

    // Root layout
    Gtk::Box main_box_{Gtk::ORIENTATION_VERTICAL};
    Gtk::MenuBar menubar_;
    Glib::RefPtr<Gtk::AccelGroup> accel_group_;
    Gtk::Toolbar toolbar_;
    Gtk::Statusbar statusbar_;
    Gtk::Paned paned_{Gtk::ORIENTATION_HORIZONTAL};

    // Left pane
    Gtk::Box left_box_{Gtk::ORIENTATION_VERTICAL};
    Gtk::Box sort_box_{Gtk::ORIENTATION_HORIZONTAL};
    Gtk::Label sort_label_;
    Gtk::ComboBoxText sort_combo_;
    Glib::RefPtr<Gtk::ListStore> list_store_;
    Gtk::TreeView list_view_;

    // Right pane
    Gtk::Box right_box_{Gtk::ORIENTATION_VERTICAL};
    Gtk::Label title_label_;
    Gtk::Grid details_grid_;
    Gtk::Label director_label_, director_value_;
    Gtk::Label actors_label_, actors_value_;
    Gtk::Label genres_label_, genres_value_;
    Gtk::Label runtime_label_, runtime_value_;
    Gtk::Label imdb_label_;
    Gtk::LinkButton imdb_link_;
    Gtk::Image poster_;
    Gtk::TextView plot_view_;
    Glib::RefPtr<Gdk::Pixbuf> poster_pixbuf_original_;
    std::string current_imdb_id_;

    // --- Helpers split across implementation files ---
    // menu.cpp
    void build_menubar();
    void on_menu_quit();
    void on_menu_about();

    // toolbar.cpp
    void build_toolbar();
    Gtk::ToolButton* btn_add_ { nullptr };
    Gtk::ToolButton* btn_delete_ { nullptr };
    Gtk::ToolButton* btn_refresh_ { nullptr };
    Gtk::ToolButton* btn_update_ { nullptr };

    // poster.cpp
    void update_poster_scaled();
    void load_poster_async(const std::string& url, const std::string& imdb);

    // handlers.cpp
    void show_status(const std::string& msg);
    void add_form_row(int row, Gtk::Label& lbl, Gtk::Label& val);
    Glib::ustring current_selected_imdb();
    void reload_model(const Glib::ustring& select_imdb = {});
    void on_selection_changed();
    void on_delete_current();
    void on_update_current();
    void on_add_movie();
    static std::string join(const std::vector<std::string>& v, const std::string& sep);
};
