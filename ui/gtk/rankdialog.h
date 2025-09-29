// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/gtk/rankdialog.h
// Purpose: GTK dialog for pairwise ranking of movies.
//-------------------------------------------------------------------------------
#pragma once

#include <gtkmm.h>
//#include <gdkmm/cursor.h>

/**
 * @brief GTK dialog for pairwise ranking using simple Elo-like updates.
 */
class Top100GtkRankDialog : public Gtk::Dialog {
public:
    /** @brief Pairwise ranking dialog (Elo-style).
     *  @param parent Parent window */
    Top100GtkRankDialog(Gtk::Window& parent);
private:
    // Containers to size posters consistently
    Gtk::Box* left_box_ { nullptr };
    Gtk::Box* right_box_ { nullptr };
    Gtk::EventBox* left_click_ { nullptr };
    Gtk::EventBox* right_click_ { nullptr };

    Gtk::Label heading_;
    Gtk::Label prompt_;

    Gtk::Label left_title_;
    Gtk::Label left_details_;
    Gtk::Image left_poster_;

    Gtk::Label right_title_;
    Gtk::Label right_details_;
    Gtk::Image right_poster_;

    Gtk::Button pass_btn_ {"Pass"};
    Gtk::Button finish_btn_ {"Finish Ranking"};

    int left_index_ { -1 };
    int right_index_ { -1 };

    void pick_two();
    void refresh_side(bool left);
    void choose_left();
    void choose_right();
    void pass_pair();

    // Poster helpers
    Glib::RefPtr<Gdk::Pixbuf> left_orig_;
    Glib::RefPtr<Gdk::Pixbuf> right_orig_;
    static Glib::RefPtr<Gdk::Pixbuf> scale_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& src, int maxW, int maxH);
    void update_scaled_posters();
    void load_poster_async_for(Gtk::Image& image, Glib::RefPtr<Gdk::Pixbuf>& store, const std::string& url);

    // Click handlers
    bool on_left_click(GdkEventButton*);
    bool on_right_click(GdkEventButton*);

    // Hover handlers to set hand cursor
    bool on_left_enter(GdkEventCrossing*);
    bool on_left_leave(GdkEventCrossing*);
    bool on_right_enter(GdkEventCrossing*);
    bool on_right_leave(GdkEventCrossing*);
};

// Helper to open dialog (for toolbar wiring)
void gtk_open_rank_dialog(Gtk::Window& parent);
