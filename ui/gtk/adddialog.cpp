// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/adddialog.cpp
// Purpose: GTK dialog to search OMDb, preview details, and pick a movie.
//-------------------------------------------------------------------------------
#include "adddialog.h"

#include <gdkmm/pixbufloader.h>
#include <glibmm/main.h>
#include <thread>
#include <cpr/cpr.h>

#include "../../lib/config.h"
#include "../../lib/omdb.h"
#include "../common/constants.h"

using namespace ui_constants;

Top100GtkAddDialog::Top100GtkAddDialog(Gtk::Window& parent) : Gtk::Dialog("Add Movie (OMDb)", parent) {
    set_modal(true);
    // Choose a sensible default size (~50% width, ~60% height of screen/parent)
    int pw = parent.get_allocated_width();
    int ph = parent.get_allocated_height();
    if (pw <= 1 || ph <= 1) {
        auto screen = parent.get_screen();
        if (screen) { pw = screen->get_width(); ph = screen->get_height(); }
    }
    if (pw <= 1 || ph <= 1) { pw = 1600; ph = 900; }
    set_default_size(static_cast<int>(pw * 0.5), static_cast<int>(ph * 0.6));
    set_resizable(false);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

    auto* area = get_content_area();
    root_.set_border_width(kGroupPadding);
    root_.set_spacing(kSpacingSmall);
    area->pack_start(root_, Gtk::PACK_EXPAND_WIDGET);

    // Search row
    search_row_.set_spacing(kSpacingSmall);
    // Placeholder aligns with Qt/KDE wording
    entry_query_.set_placeholder_text("title keyword");
    // Do not make Search the default action; default button should be Add
    btn_search_.set_can_default(false);
    search_row_.pack_start(lbl_search_, Gtk::PACK_SHRINK);
    search_row_.pack_start(entry_query_, Gtk::PACK_EXPAND_WIDGET);
    search_row_.pack_start(btn_search_, Gtk::PACK_SHRINK);
    root_.pack_start(search_row_, Gtk::PACK_SHRINK);

    // Split results/preview
        store_ = Gtk::ListStore::create(columns_);
        view_.set_model(store_);
    view_.append_column("Results", columns_.display);
    // Hide column header to avoid duplicate "Results" text (we have a heading above)
    view_.set_headers_visible(false);
        view_.get_selection()->signal_changed().connect(sigc::mem_fun(*this, &Top100GtkAddDialog::on_selection_changed));
        // Results frame with heading and scroller
        auto results_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
        results_box->set_border_width(kGroupPadding);
        results_box->set_spacing(kSpacingSmall);
        results_heading_.set_use_markup(true);
        results_heading_.set_xalign(0.0f);
        results_box->pack_start(results_heading_, Gtk::PACK_SHRINK);
        auto scrolled_results = Gtk::manage(new Gtk::ScrolledWindow());
        scrolled_results->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
        scrolled_results->add(view_);
        results_box->pack_start(*scrolled_results, Gtk::PACK_EXPAND_WIDGET);
        results_frame_.set_shadow_type(Gtk::SHADOW_ETCHED_IN);
        results_frame_.add(*results_box);
        split_.pack1(results_frame_, true, false);

    // Details frame: title + vertical split (poster 70% / plot 30%)
    preview_.set_border_width(kGroupPadding);
    preview_.set_spacing(kSpacingSmall);
    title_.set_xalign(0.0f);
    title_.set_markup("<b></b>");
    preview_.pack_start(title_, Gtk::PACK_SHRINK);

    // Top: Poster overlay
    poster_overlay_.add(poster_);
    poster_spinner_.set_no_show_all(true);
    poster_spinner_.set_halign(Gtk::ALIGN_CENTER);
    poster_spinner_.set_valign(Gtk::ALIGN_CENTER);
    poster_overlay_.add_overlay(poster_spinner_);

    // Bottom: Plot area with heading and scroller
    auto plot_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    plot_heading_.set_use_markup(true);
    plot_heading_.set_xalign(0.0f);
    plot_.set_editable(false);
    plot_.set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    auto scrolled_plot = Gtk::manage(new Gtk::ScrolledWindow());
    scrolled_plot->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    scrolled_plot->add(plot_);
    plot_box->pack_start(plot_heading_, Gtk::PACK_SHRINK);
    plot_box->pack_start(*scrolled_plot, Gtk::PACK_EXPAND_WIDGET);

    // Vertical paned split to enforce ~70/30 layout; position set on first allocation
    details_split_.pack1(poster_overlay_, true, false);
    details_split_.pack2(*plot_box, true, false);
    preview_.pack_start(details_split_, Gtk::PACK_EXPAND_WIDGET);

    details_frame_.set_shadow_type(Gtk::SHADOW_ETCHED_IN);
    details_frame_.add(preview_);
    split_.pack2(details_frame_, true, false);
    split_.set_position(300);
    // Keep main split fixed at ~35/65; reset if moved (listen to position property changes)
    split_.property_position().signal_changed().connect(sigc::mem_fun(*this, &Top100GtkAddDialog::on_main_split_position_changed));
    root_.pack_start(split_, Gtk::PACK_EXPAND_WIDGET);

    // Buttons
        auto enter_btn = add_button("Enter manually", Gtk::RESPONSE_REJECT);
        enter_btn->set_sensitive(false);
        auto cancel_btn = add_button("Cancel", Gtk::RESPONSE_CANCEL);
        auto add_btn = add_button("Add", Gtk::RESPONSE_OK);
        add_btn->set_sensitive(false);
    set_default_response(Gtk::RESPONSE_OK);

    btn_search_.signal_clicked().connect(sigc::mem_fun(*this, &Top100GtkAddDialog::on_search_clicked));
    entry_query_.signal_activate().connect(sigc::mem_fun(*this, &Top100GtkAddDialog::on_search_clicked));
    show_all_children();
}

void Top100GtkAddDialog::on_size_allocate(Gtk::Allocation& allocation) {
    Gtk::Dialog::on_size_allocate(allocation);
    update_poster_scaled();
    // Initialize details split to 70/30 (poster/plot) once we know the height
    if (!details_split_position_initialized_) {
        int h = details_split_.get_allocated_height();
        if (h > 0) {
            details_split_.set_position(static_cast<int>(h * 0.65));
            details_split_position_initialized_ = true;
        }
    }
    // Initialize/update main split to 35/65 based on current width
    int w = split_.get_allocated_width();
    if (w > 0) {
        int target = static_cast<int>(w * 0.35);
        if (main_split_target_pos_ != target) {
            main_split_target_pos_ = target;
            main_split_locking_ = true; // avoid feedback loop on signal
            split_.set_position(main_split_target_pos_);
            main_split_locking_ = false;
        }
    }
}

void Top100GtkAddDialog::on_main_split_position_changed() {
    if (main_split_locking_) return;
    if (main_split_target_pos_ >= 0) {
        main_split_locking_ = true;
        split_.set_position(main_split_target_pos_);
        main_split_locking_ = false;
    }
}

void Top100GtkAddDialog::on_search_clicked() {
    store_->clear();
    selected_imdb_.clear();
    title_.set_markup("<b></b>");
    plot_.get_buffer()->set_text("");
        poster_.clear();
    poster_orig_.reset();

    auto q = entry_query_.get_text();
    if (q.empty()) return;
    try {
        AppConfig cfg = loadConfig();
        if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) { return; }
        auto results = omdbSearch(cfg.omdbApiKey, q);
        for (const auto& r : results) {
            auto row = *(store_->append());
            std::string text = r.title + " (" + std::to_string(r.year) + ")";
            row[columns_.display] = text;
            row[columns_.imdb] = r.imdbID;
        }
            if (store_->children().size() > 0) {
                auto sel = view_.get_selection();
                sel->select(store_->children().begin());
            }
    } catch (...) {
        // ignore
    }
}

void Top100GtkAddDialog::on_selection_changed() {
    auto sel = view_.get_selection();
    auto iter = sel ? sel->get_selected() : Gtk::TreeModel::iterator{};
        if (!iter) { selected_imdb_.clear();
            // disable Add button when nothing selected
            auto action_area = get_action_area();
            if (action_area) {
                for (auto* child : action_area->get_children()) {
                    if (auto* btn = dynamic_cast<Gtk::Button*>(child)) {
                        if (btn->get_label() == "Add") btn->set_sensitive(false);
                    }
                }
            }
            return; }
    std::string imdb = Glib::ustring((*iter)[columns_.imdb]).raw();
    selected_imdb_ = imdb;
        // enable Add button on selection
        auto action_area = get_action_area();
        if (action_area) {
            for (auto* child : action_area->get_children()) {
                if (auto* btn = dynamic_cast<Gtk::Button*>(child)) {
                    if (btn->get_label() == "Add") btn->set_sensitive(true);
                }
            }
        }
    try {
        AppConfig cfg = loadConfig();
        if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) return;
        auto maybe = omdbGetById(cfg.omdbApiKey, imdb);
        if (!maybe) return;
        std::string title = maybe->title;
        if (maybe->year > 0) title += " (" + std::to_string(maybe->year) + ")";
        title_.set_markup(std::string("<b>") + title + "</b>");
        std::string plot = !maybe->plotShort.empty() ? maybe->plotShort : maybe->plotFull;
        plot_.get_buffer()->set_text(plot);
        load_poster_async(maybe->posterUrl, imdb);
    } catch (...) { /* ignore */ }
}

void Top100GtkAddDialog::load_poster_async(const std::string& url, const std::string& imdb) {
    poster_orig_.reset();
    poster_.clear();
    poster_spinner_.show();
    poster_spinner_.start();
    if (url.empty() || url == "N/A") { poster_spinner_.stop(); poster_spinner_.hide(); return; }
        poster_for_imdb_ = imdb;
    std::thread([this, url, imdb]() {
        auto resp = cpr::Get(cpr::Url{url}, cpr::Timeout{8000});
        if (resp.error || resp.status_code != 200 || resp.text.empty()) {
            Glib::signal_idle().connect_once([this]() { poster_spinner_.stop(); poster_spinner_.hide(); });
            return;
        }
        auto bytes = std::make_shared<std::string>(std::move(resp.text));
        Glib::signal_idle().connect_once([this, bytes, imdb]() {
            if (imdb != poster_for_imdb_) return; // stale
            try {
                auto loader = Gdk::PixbufLoader::create();
                loader->write(reinterpret_cast<const guint8*>(bytes->data()), bytes->size());
                loader->close();
                poster_orig_ = loader->get_pixbuf();
                if (poster_orig_) update_poster_scaled(); else poster_.clear();
                poster_spinner_.stop();
                poster_spinner_.hide();
            } catch (...) { poster_.clear(); }
        });
    }).detach();
}

void Top100GtkAddDialog::update_poster_scaled() {
    if (!poster_orig_) return;
    int maxW = std::max(1, static_cast<int>(get_width() * kPosterMaxWidthRatio));
    int maxH = std::max(1, static_cast<int>(get_height() * kPosterMaxHeightRatio));
    int w = poster_orig_->get_width();
    int h = poster_orig_->get_height();
    if (w <= 0 || h <= 0) return;
    double sx = static_cast<double>(maxW) / w;
    double sy = static_cast<double>(maxH) / h;
    double s = std::min(1.0, std::min(sx, sy));
    int tw = std::max(1, static_cast<int>(w * s));
    int th = std::max(1, static_cast<int>(h * s));
    auto scaled = poster_orig_->scale_simple(tw, th, Gdk::INTERP_BILINEAR);
    if (scaled) poster_.set(scaled);
}
