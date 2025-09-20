// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/main.cpp
// Purpose: GTK (gtkmm) UI with two-pane layout in parity with Qt/KDE UIs.
// Language: C++17 (gtkmm-3)
//-------------------------------------------------------------------------------
#include <gtkmm.h>
#include <gdk/gdkkeysyms.h>
#include <gdkmm/pixbufloader.h>
#include <cpr/cpr.h>
#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include "../../lib/top100.h"
#include "../../lib/config.h"
#include "../../lib/omdb.h"
#include "../common/strings.h"
#include "../common/constants.h"

using namespace ui_strings;
using namespace ui_constants;

class Top100GtkWindow : public Gtk::Window {
public:
    Top100GtkWindow()
    : main_box_{Gtk::ORIENTATION_VERTICAL},
      paned_{Gtk::ORIENTATION_HORIZONTAL},
      left_box_{Gtk::ORIENTATION_VERTICAL},
      sort_box_{Gtk::ORIENTATION_HORIZONTAL},
      sort_label_{kLabelSortOrder},
      list_store_{Gtk::ListStore::create(columns_)},
      right_box_{Gtk::ORIENTATION_VERTICAL},
      director_label_{kFieldDirector},
      actors_label_{kFieldActors},
      genres_label_{kFieldGenres},
      runtime_label_{kFieldRuntime},
      imdb_label_{kFieldImdbPage}
    {
        set_title(kAppDisplayName);
        set_default_size(int(kInitialWidth*1.2), int(kInitialHeight*1.2));

    add(main_box_);

    // Accel group and menu bar
    accel_group_ = Gtk::AccelGroup::create();
    add_accel_group(accel_group_);
    build_menubar();
    main_box_.pack_start(menubar_, Gtk::PACK_SHRINK);

        // Toolbar actions
        auto add_btn = Gtk::manage(new Gtk::ToolButton("Add"));
        auto del_btn = Gtk::manage(new Gtk::ToolButton("Delete"));
        auto refresh_btn = Gtk::manage(new Gtk::ToolButton("Refresh"));
        auto update_btn = Gtk::manage(new Gtk::ToolButton("Update (OMDb)"));
        toolbar_.append(*add_btn);
        toolbar_.append(*del_btn);
        toolbar_.append(*refresh_btn);
        toolbar_.append(*update_btn);
        main_box_.pack_start(toolbar_, Gtk::PACK_SHRINK);

        // Paned: left list, right details
        paned_.set_position(int(0.45 * kInitialWidth));
        main_box_.pack_start(paned_, Gtk::PACK_EXPAND_WIDGET);
        main_box_.pack_end(statusbar_, Gtk::PACK_SHRINK);

        // Left side
    auto movies_heading = Gtk::manage(new Gtk::Label);
    movies_heading->set_markup(std::string("<b>") + kHeadingMovies + "</b>");
    movies_heading->set_xalign(0.0f);
    movies_heading->set_halign(Gtk::ALIGN_START);
        left_box_.set_border_width(kGroupPadding);
        left_box_.set_spacing(kSpacingSmall);
        left_box_.pack_start(*movies_heading, Gtk::PACK_SHRINK);

        sort_box_.set_spacing(kSpacingSmall);
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

        // Toolbar actions
        refresh_btn->signal_clicked().connect([this]() { reload_model(current_selected_imdb()); show_status("Refreshed"); });
        update_btn->signal_clicked().connect(sigc::mem_fun(*this, &Top100GtkWindow::on_update_current));
        del_btn->signal_clicked().connect(sigc::mem_fun(*this, &Top100GtkWindow::on_delete_current));
        add_btn->signal_clicked().connect(sigc::mem_fun(*this, &Top100GtkWindow::on_add_movie));

        reload_model();
        show_all_children();
    }

private:
    // Model columns for list
    class Columns : public Gtk::TreeModel::ColumnRecord {
    public:
        Columns() { add(text); add(index); add(imdb); }
        Gtk::TreeModelColumn<Glib::ustring> text;
        Gtk::TreeModelColumn<int> index; // row index for retrieval
        Gtk::TreeModelColumn<Glib::ustring> imdb; // imdb id
    } columns_;

    Gtk::Box main_box_;
    Gtk::MenuBar menubar_;
    Glib::RefPtr<Gtk::AccelGroup> accel_group_;
    Gtk::Toolbar toolbar_;
    Gtk::Statusbar statusbar_;
    Gtk::Paned paned_;

    // Left
    Gtk::Box left_box_;
    Gtk::Box sort_box_;
    Gtk::Label sort_label_;
    Gtk::ComboBoxText sort_combo_;
    Glib::RefPtr<Gtk::ListStore> list_store_;
    Gtk::TreeView list_view_;

    // Right
    Gtk::Box right_box_;
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

    // Scale and set the poster image to fit the right pane while preserving aspect
    void update_poster_scaled() {
        if (!poster_pixbuf_original_) return;
        auto alloc = right_box_.get_allocation();
        int maxW = static_cast<int>(alloc.get_width() * kPosterMaxWidthRatio);
        int maxH = static_cast<int>(alloc.get_height() * kPosterMaxHeightRatio);
        if (maxW <= 0 || maxH <= 0) return;
        int w = poster_pixbuf_original_->get_width();
        int h = poster_pixbuf_original_->get_height();
        if (w <= 0 || h <= 0) return;
        double sx = static_cast<double>(maxW) / w;
        double sy = static_cast<double>(maxH) / h;
        double s = std::min(1.0, std::min(sx, sy));
        int tw = std::max(1, static_cast<int>(w * s));
        int th = std::max(1, static_cast<int>(h * s));
        auto scaled = poster_pixbuf_original_->scale_simple(tw, th, Gdk::INTERP_BILINEAR);
        if (scaled)
            poster_.set(scaled);
    }

    // Async fetch poster bytes and update in UI thread
    void load_poster_async(const std::string& url, const std::string& imdb) {
        if (url.empty()) { poster_.clear(); poster_pixbuf_original_.reset(); return; }
        current_imdb_id_ = imdb;
        std::thread([this, url, imdb]() {
            // Basic GET with reasonable timeout
            auto resp = cpr::Get(cpr::Url{url}, cpr::Timeout{8000});
            if (resp.error || resp.status_code != 200 || resp.text.empty()) return;
            auto bytes = std::make_shared<std::string>(std::move(resp.text));
            Glib::signal_idle().connect_once([this, bytes, imdb]() {
                // Drop stale results
                if (imdb != current_imdb_id_) return;
                try {
                    auto loader = Gdk::PixbufLoader::create();
                    loader->write(reinterpret_cast<const guint8*>(bytes->data()), bytes->size());
                    loader->close();
                    poster_pixbuf_original_ = loader->get_pixbuf();
                    if (poster_pixbuf_original_) update_poster_scaled(); else poster_.clear();
                } catch (...) {
                    poster_.clear();
                }
            });
        }).detach();
    }

    // Helpers
    void show_status(const std::string& msg) { statusbar_.push(msg); }

    void build_menubar() {
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

    void on_menu_quit() { hide(); }
    void on_menu_about() {
        Gtk::MessageDialog dlg(*this, kAppDisplayName, false, Gtk::MESSAGE_INFO, Gtk::BUTTONS_OK, true);
        dlg.set_secondary_text(kAboutDialogText);
        dlg.run();
    }

    void add_form_row(int row, Gtk::Label& lbl, Gtk::Label& val) {
        lbl.set_xalign(0.0f);
        lbl.set_width_chars(kLabelMinWidth / 7); // approx: 7px per char
        val.set_xalign(0.0f);
        details_grid_.attach(lbl, 0, row, 1, 1);
        details_grid_.attach(val, 1, row, 1, 1);
    }

    Glib::ustring current_selected_imdb() {
        auto sel = list_view_.get_selection();
        if (!sel) return {};
        auto iter = sel->get_selected();
        if (iter) return (*iter)[columns_.imdb];
        return {};
    }

    void reload_model(const Glib::ustring& select_imdb = {}) {
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

    void on_selection_changed() {
        auto sel = list_view_.get_selection();
        {
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
    }

    void on_delete_current() {
        auto sel = list_view_.get_selection();
        {
            auto iter = sel ? sel->get_selected() : Gtk::TreeModel::iterator{};
            if (!iter) return;
            Glib::ustring imdb = (*iter)[columns_.imdb];
            AppConfig cfg = loadConfig();
            Top100 list(cfg.dataFile);
            if (list.removeByImdbId(imdb)) { show_status("Deleted."); }
            reload_model();
        }
    }

    void on_update_current() {
        auto sel = list_view_.get_selection();
        {
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
    }

    void on_add_movie() {
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

    static std::string join(const std::vector<std::string>& v, const std::string& sep) {
        std::ostringstream oss; bool first = true;
        for (const auto& s : v) { if (!first) oss << sep; oss << s; first = false; }
        return oss.str();
    }
};

int main(int argc, char* argv[]) {
    auto app = Gtk::Application::create(argc, argv, "uk.co.andymccall.top100");
    Top100GtkWindow win;
    return app->run(win);
}
