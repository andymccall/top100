// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/window.cpp
// Purpose: GTK (gtkmm) main window layout and signal wiring.
// Language: C++17 (gtkmm-3)
//-------------------------------------------------------------------------------
#include "window.h"

#include <gdk/gdkkeysyms.h>
#include "../../lib/config.h"
#include "../common/strings.h"
#include "../common/constants.h"

using namespace ui_strings;
using namespace ui_constants;

Top100GtkWindow::Top100GtkWindow()
    : list_store_{Gtk::ListStore::create(columns_)},
      director_label_{kFieldDirector}, actors_label_{kFieldActors},
      genres_label_{kFieldGenres}, runtime_label_{kFieldRuntime},
      imdb_label_{kFieldImdbPage}
{
    set_title(kAppDisplayName);
    set_default_size(int(kInitialWidth * 1.2), int(kInitialHeight * 1.2));

    add(main_box_);

    // Accel group and menu bar
    accel_group_ = Gtk::AccelGroup::create();
    add_accel_group(accel_group_);
    build_menubar();
    main_box_.pack_start(menubar_, Gtk::PACK_SHRINK);

    // Toolbar
    build_toolbar();
    main_box_.pack_start(toolbar_, Gtk::PACK_SHRINK);

    // Paned: left list, right details
    paned_.set_position(int(0.45 * kInitialWidth));
    main_box_.pack_start(paned_, Gtk::PACK_EXPAND_WIDGET);
    // Statusbar at the bottom
    main_box_.pack_end(statusbar_, Gtk::PACK_SHRINK);
    status_ctx_movies_ = statusbar_.get_context_id("movies");

    // Left side
    auto movies_heading = Gtk::manage(new Gtk::Label);
    movies_heading->set_markup(std::string("<b>") + kHeadingMovies + "</b>");
    movies_heading->set_xalign(0.0f);
    movies_heading->set_halign(Gtk::ALIGN_START);
    left_box_.set_border_width(kGroupPadding);
    left_box_.set_spacing(kSpacingSmall);
    left_box_.pack_start(*movies_heading, Gtk::PACK_SHRINK);

    sort_box_.set_spacing(kSpacingSmall);
    sort_label_.set_text(kLabelSortOrder);
    sort_box_.pack_start(sort_label_, Gtk::PACK_SHRINK);
    sort_combo_.append(kSortInsertion);
    sort_combo_.append(kSortByYear);
    sort_combo_.append(kSortAlpha);
    sort_combo_.append(kSortByRank);
    sort_combo_.append(kSortByScore);
    sort_box_.pack_start(sort_combo_, Gtk::PACK_EXPAND_WIDGET);
    left_box_.pack_start(sort_box_, Gtk::PACK_SHRINK);

    list_view_.set_model(list_store_);
    list_view_.append_column("Movie", columns_.text);
    auto left_scroller = Gtk::manage(new Gtk::ScrolledWindow);
    left_scroller->add(list_view_);
    left_scroller->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    left_box_.pack_start(*left_scroller, Gtk::PACK_EXPAND_WIDGET);
    paned_.add1(left_box_);

    // Right side
    right_box_.set_border_width(kGroupPadding);
    right_box_.set_spacing(kSpacingSmall);
    auto details_hdr = Gtk::manage(new Gtk::Label);
    details_hdr->set_markup(std::string("<b>") + kHeadingDetails + "</b>");
    details_hdr->set_xalign(0.0f);
    details_hdr->set_halign(Gtk::ALIGN_START);
    right_box_.pack_start(*details_hdr, Gtk::PACK_SHRINK);
    title_label_.set_use_markup(true);
    title_label_.set_justify(Gtk::JUSTIFY_LEFT);
    title_label_.set_xalign(0.0f);
    title_label_.set_halign(Gtk::ALIGN_START);
    title_label_.set_ellipsize(Pango::ELLIPSIZE_END);
    right_box_.pack_start(title_label_, Gtk::PACK_SHRINK);

    details_grid_.set_row_spacing(kSpacingSmall);
    details_grid_.set_column_spacing(12);
    int r = 0;
    add_form_row(r++, director_label_, director_value_);
    // Actors single-line with ellipsize
    actors_value_.set_ellipsize(Pango::ELLIPSIZE_END);
    add_form_row(r++, actors_label_, actors_value_);
    add_form_row(r++, genres_label_, genres_value_);
    add_form_row(r++, runtime_label_, runtime_value_);
    imdb_label_.set_xalign(0.0f);
    imdb_link_.set_halign(Gtk::ALIGN_START);
    details_grid_.attach(imdb_label_, 0, r, 1, 1);
    details_grid_.attach(imdb_link_, 1, r, 1, 1);
    right_box_.pack_start(details_grid_, Gtk::PACK_SHRINK);

    poster_.set_hexpand(true);
    poster_.set_vexpand(true);
    right_box_.pack_start(poster_, Gtk::PACK_EXPAND_WIDGET);

    auto plot_hdr = Gtk::manage(new Gtk::Label);
    plot_hdr->set_markup(std::string("<b>") + kGroupPlot + "</b>");
    plot_hdr->set_xalign(0.0f);
    plot_hdr->set_halign(Gtk::ALIGN_START);
    right_box_.pack_start(*plot_hdr, Gtk::PACK_SHRINK);
    auto plot_scroll = Gtk::manage(new Gtk::ScrolledWindow);
    plot_scroll->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    plot_scroll->add(plot_view_);
    plot_view_.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    right_box_.pack_start(*plot_scroll, Gtk::PACK_EXPAND_WIDGET);

    paned_.add2(right_box_);

    // Initialize sort from config
    try {
        AppConfig cfg = loadConfig();
        int so = cfg.uiSortOrder;
        if (so < 0 || so > 4) so = 0;
        sort_combo_.set_active(so);
    } catch (...) {
        sort_combo_.set_active(0);
    }

    // Signals
    sort_combo_.signal_changed().connect([this]() {
        try {
            AppConfig cfg = loadConfig();
            cfg.uiSortOrder = sort_combo_.get_active_row_number();
            saveConfig(cfg);
        } catch (...) { /* ignore */ }
        reload_model();
    });
    list_view_.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &Top100GtkWindow::on_selection_changed));
    // On right pane resize, rescale poster
    right_box_.signal_size_allocate().connect([this](Gtk::Allocation&){ update_poster_scaled(); });

    reload_model();
    update_status_movie_count();
    show_all_children();
}

void Top100GtkWindow::update_status_movie_count() {
    // Pop the previous message in this context (if any), then push new count
    statusbar_.pop(status_ctx_movies_);
    const auto n = static_cast<unsigned>(list_store_->children().size());
    statusbar_.push(std::to_string(n) + (n == 1 ? " movie" : " movies"), status_ctx_movies_);
}
