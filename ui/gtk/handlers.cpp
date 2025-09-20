// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/handlers.cpp
// Purpose: Model reload and UI event handlers for the GTK window.
//-------------------------------------------------------------------------------
#include "handlers.h"

#include <sstream>

#include "../../lib/top100.h"
#include "../../lib/config.h"
#include "../../lib/omdb.h"
#include "../common/strings.h"
#include "../common/constants.h"

using namespace ui_strings;
using namespace ui_constants;

// Helpers
void Top100GtkWindow::show_status(const std::string& msg) { statusbar_.push(msg); }

void Top100GtkWindow::add_form_row(int row, Gtk::Label& lbl, Gtk::Label& val) {
    lbl.set_xalign(0.0f);
    lbl.set_width_chars(kLabelMinWidth / 7); // approx: 7px per char
    val.set_xalign(0.0f);
    details_grid_.attach(lbl, 0, row, 1, 1);
    details_grid_.attach(val, 1, row, 1, 1);
}

Glib::ustring Top100GtkWindow::current_selected_imdb() {
    auto sel = list_view_.get_selection();
    if (!sel) return {};
    auto iter = sel->get_selected();
    if (iter) return (*iter)[columns_.imdb];
    return {};
}

void Top100GtkWindow::reload_model(const Glib::ustring& select_imdb) {
    list_store_->clear();
    AppConfig cfg;
    try { cfg = loadConfig(); } catch (...) { return; }
    Top100 list(cfg.dataFile);
    SortOrder order = SortOrder::DEFAULT;
    switch (sort_combo_.get_active_row_number()) {
        case 1: order = SortOrder::BY_YEAR; break;
        case 2: order = SortOrder::ALPHABETICAL; break;
        case 3: order = SortOrder::BY_USER_RANK; break;
        case 4: order = SortOrder::BY_USER_SCORE; break;
        default: order = SortOrder::DEFAULT; break;
    }
    auto movies = list.getMovies(order);
    int idx = 0;
    for (const auto& m : movies) {
        auto row = *(list_store_->append());
        Glib::ustring text = (m.userRank > 0 ? ("#" + std::to_string(m.userRank) + " ") : "") + m.title + " (" + std::to_string(m.year) + ")";
        row[columns_.text] = text;
        row[columns_.index] = idx++;
        row[columns_.imdb] = m.imdbID;
    }
    // Select requested or first row by default
    auto sel = list_view_.get_selection();
    if (!select_imdb.empty()) {
        for (auto it = list_store_->children().begin(); it != list_store_->children().end(); ++it) {
            if ((*it)[columns_.imdb] == select_imdb) { sel->select(it); break; }
        }
    }
    if (!(sel && sel->get_selected()) && list_store_->children().size() > 0) sel->select(list_store_->children().begin());
}

void Top100GtkWindow::on_selection_changed() {
    auto sel = list_view_.get_selection();
    auto iter = sel ? sel->get_selected() : Gtk::TreeModel::iterator{};
    if (!iter) return;
    Glib::ustring imdb = (*iter)[columns_.imdb];
    AppConfig cfg = loadConfig();
    Top100 list(cfg.dataFile);
    // Find by imdb
    int index = list.findIndexByImdbId(imdb);
    if (index < 0) return;
    auto movies = list.getMovies(SortOrder::DEFAULT);
    const Movie& mv = movies.at(static_cast<size_t>(index));
    // Title
    std::stringstream ss; ss << "<b>" << mv.title << " (" << mv.year << ")</b>";
    title_label_.set_markup(ss.str());
    // Details
    director_value_.set_text(mv.director);
    actors_value_.set_text(join(mv.actors, ", "));
    genres_value_.set_text(join(mv.genres, ", "));
    runtime_value_.set_text(mv.runtimeMinutes > 0 ? std::to_string(mv.runtimeMinutes) + " min" : "");
    // IMDb link
    if (!mv.imdbID.empty()) {
        std::string url = std::string("https://www.imdb.com/title/") + mv.imdbID + "/";
        imdb_link_.set_uri(url);
        imdb_link_.set_label("Open in IMDb");
        imdb_link_.set_sensitive(true);
    } else {
        imdb_link_.set_uri("");
        imdb_link_.set_label("");
        imdb_link_.set_sensitive(false);
    }
    // Plot
    plot_view_.get_buffer()->set_text(mv.plotFull.empty() ? mv.plotShort : mv.plotFull);
    // Poster (load async and scale)
    load_poster_async(mv.posterUrl, mv.imdbID);
}

void Top100GtkWindow::on_delete_current() {
    auto sel = list_view_.get_selection();
    auto iter = sel ? sel->get_selected() : Gtk::TreeModel::iterator{};
    if (!iter) return;
    Glib::ustring imdb = (*iter)[columns_.imdb];
    AppConfig cfg = loadConfig();
    Top100 list(cfg.dataFile);
    if (list.removeByImdbId(imdb)) { show_status("Deleted."); }
    reload_model();
}

void Top100GtkWindow::on_update_current() {
    auto sel = list_view_.get_selection();
    auto iter = sel ? sel->get_selected() : Gtk::TreeModel::iterator{};
    if (!iter) return;
    Glib::ustring imdb = (*iter)[columns_.imdb];
    AppConfig cfg = loadConfig();
    if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) { show_status("OMDb not configured"); return; }
    auto maybe = omdbGetById(cfg.omdbApiKey, imdb);
    if (!maybe) { show_status("OMDb fetch failed"); return; }
    Top100 list(cfg.dataFile);
    if (list.mergeFromOmdbByImdbId(*maybe)) {
        show_status("Updated from OMDb");
        reload_model(imdb);
    } else {
        show_status("Not in list");
    }
}

void Top100GtkWindow::on_add_movie() {
    Gtk::Dialog dialog("Add Movie by IMDb ID", *this);
    dialog.add_button("Cancel", Gtk::RESPONSE_CANCEL);
    dialog.add_button("Add", Gtk::RESPONSE_OK);
    Gtk::Box* box = dialog.get_content_area();
    Gtk::Label prompt("Enter IMDb ID (e.g., tt0133093):");
    Gtk::Entry entry;
    entry.set_placeholder_text("tt........");
    box->pack_start(prompt, Gtk::PACK_SHRINK);
    box->pack_start(entry, Gtk::PACK_SHRINK);
    dialog.show_all_children();
    if (dialog.run() == Gtk::RESPONSE_OK) {
        auto imdb = entry.get_text();
        if (imdb.empty()) { show_status("No IMDb ID entered"); return; }
        try {
            AppConfig cfg = loadConfig();
            if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) { show_status("OMDb not configured"); return; }
            auto maybe = omdbGetById(cfg.omdbApiKey, imdb);
            if (!maybe) { show_status("OMDb fetch failed"); return; }
            Top100 list(cfg.dataFile);
            list.addMovie(*maybe);
            list.recomputeRanks();
            show_status("Added movie");
            reload_model(imdb);
        } catch (...) {
            show_status("Error adding movie");
        }
    }
}

std::string Top100GtkWindow::join(const std::vector<std::string>& v, const std::string& sep) {
    std::ostringstream oss; bool first = true;
    for (const auto& s : v) { if (!first) oss << sep; oss << s; first = false; }
    return oss.str();
}
