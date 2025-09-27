// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/adddialog.h
// Purpose: GTK dialog to search OMDb, preview, and select a movie to add.
//-------------------------------------------------------------------------------
#pragma once

#include <gtkmm.h>
#include <glibmm/refptr.h>
#include <string>

namespace Gdk { class Pixbuf; }

class Top100GtkAddDialog : public Gtk::Dialog {
public:
    Top100GtkAddDialog(Gtk::Window& parent);
    std::string selected_imdb() const { return selected_imdb_; }

protected:
    void on_size_allocate(Gtk::Allocation& allocation) override;

private:
    // UI elements
    Gtk::Box root_{Gtk::ORIENTATION_VERTICAL};
    Gtk::Box search_row_{Gtk::ORIENTATION_HORIZONTAL};
    // Match KDE wording
    Gtk::Label lbl_search_{"Search for a movie"};
    Gtk::Entry entry_query_;
    Gtk::Button btn_search_{"Search"};
    Gtk::Paned split_{Gtk::ORIENTATION_HORIZONTAL};

    // Results list
    class Columns : public Gtk::TreeModel::ColumnRecord {
    public:
        Columns() { add(display); add(imdb); }
        Gtk::TreeModelColumn<Glib::ustring> display;
        Gtk::TreeModelColumn<Glib::ustring> imdb;
    } columns_;
    Glib::RefPtr<Gtk::ListStore> store_;
    Gtk::TreeView view_;
    Gtk::Label results_heading_ {"<b>Results</b>", true};
    Gtk::Frame results_frame_;

    // Preview panel
    Gtk::Box preview_{Gtk::ORIENTATION_VERTICAL};
    Gtk::Label title_;
    Gtk::Overlay poster_overlay_;
    Gtk::Image poster_;
    Gtk::Spinner poster_spinner_;
    Gtk::TextView plot_;
    Gtk::Label plot_heading_ {"<b>Plot</b>", true};
    Gtk::Frame details_frame_;
    Gtk::Paned details_split_{Gtk::ORIENTATION_VERTICAL};
    Glib::RefPtr<Gdk::Pixbuf> poster_orig_;
    std::string poster_for_imdb_;

    // State
    std::string selected_imdb_;
    bool details_split_position_initialized_ {false};

    // Actions
    void on_search_clicked();
    void on_selection_changed();
    void load_poster_async(const std::string& url, const std::string& imdb);
    void update_poster_scaled();
    // Keep the main split fixed (left 35% / right 65%)
    void on_main_split_position_changed();
    int main_split_target_pos_ { -1 };
    bool main_split_locking_ { false };
};
