// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/gtk/rankdialog.cpp
// Purpose: Implementation of GTK rank dialog.
//-------------------------------------------------------------------------------
#include "rankdialog.h"

#include <sstream>
#include <random>

#include "../../lib/top100.h"
#include "../../lib/config.h"
#include "../common/constants.h"
#include <gdkmm/pixbufloader.h>
#include <glibmm/main.h>
#include <cpr/cpr.h>
#include <thread>
#include <algorithm>

using namespace ui_constants;

static std::string join(const std::vector<std::string>& v, const char* sep = ", ") {
    std::ostringstream oss; bool first = true; for (const auto& s : v) { if (!first) oss << sep; oss << s; first = false; } return oss.str();
}

Top100GtkRankDialog::Top100GtkRankDialog(Gtk::Window& parent)
    : Gtk::Dialog("", parent, true)
{
    // Ensure dialog is modal, fixed-size, and centered on its parent
    set_transient_for(parent);
    set_modal(true);
    set_resizable(false);
    set_default_size(900, 600);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
    auto* area = get_content_area();
    area->set_border_width(kGroupPadding*2);
    area->set_spacing(kSpacingLarge);

    heading_.set_markup("<b>Rank movies</b>");
    heading_.set_halign(Gtk::ALIGN_CENTER);
    area->pack_start(heading_, Gtk::PACK_SHRINK);

    prompt_.set_text("Which movie do you prefer?");
    prompt_.set_halign(Gtk::ALIGN_CENTER);
    area->pack_start(prompt_, Gtk::PACK_SHRINK);

    auto* row = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    row->set_spacing(kSpacingLarge);

    // Left pane (wrapped in EventBox to capture clicks)
    left_click_ = Gtk::manage(new Gtk::EventBox());
    left_box_ = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    left_title_.set_use_markup(true);
    left_title_.set_xalign(0.0f);
    left_details_.set_line_wrap(true);
    left_details_.set_xalign(0.0f);
    // Keep details in a fixed-height scroller to avoid jumping
    auto* left_scroller = Gtk::manage(new Gtk::ScrolledWindow);
    left_scroller->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    left_scroller->set_size_request(-1, 120);
    auto* left_details_wrap = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    left_details_wrap->pack_start(left_details_, Gtk::PACK_SHRINK);
    left_scroller->add(*left_details_wrap);
    left_box_->pack_start(left_title_, Gtk::PACK_SHRINK);
    left_box_->pack_start(*left_scroller, Gtk::PACK_SHRINK);
    left_box_->pack_start(left_poster_, Gtk::PACK_EXPAND_WIDGET);
    left_click_->add(*left_box_);
    left_click_->set_visible_window(false);
    left_click_->set_above_child(true);
    left_click_->set_tooltip_text("Click to choose this movie");
    left_click_->set_events(Gdk::BUTTON_PRESS_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK);
    left_click_->signal_button_press_event().connect(sigc::mem_fun(*this, &Top100GtkRankDialog::on_left_click), false);
    left_click_->signal_enter_notify_event().connect(sigc::mem_fun(*this, &Top100GtkRankDialog::on_left_enter), false);
    left_click_->signal_leave_notify_event().connect(sigc::mem_fun(*this, &Top100GtkRankDialog::on_left_leave), false);

    // Right pane (wrapped in EventBox to capture clicks)
    right_click_ = Gtk::manage(new Gtk::EventBox());
    right_box_ = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    right_title_.set_use_markup(true);
    right_title_.set_xalign(0.0f);
    right_details_.set_line_wrap(true);
    right_details_.set_xalign(0.0f);
    auto* right_scroller = Gtk::manage(new Gtk::ScrolledWindow);
    right_scroller->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    right_scroller->set_size_request(-1, 120);
    auto* right_details_wrap = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    right_details_wrap->pack_start(right_details_, Gtk::PACK_SHRINK);
    right_scroller->add(*right_details_wrap);
    right_box_->pack_start(right_title_, Gtk::PACK_SHRINK);
    right_box_->pack_start(*right_scroller, Gtk::PACK_SHRINK);
    right_box_->pack_start(right_poster_, Gtk::PACK_EXPAND_WIDGET);
    right_click_->add(*right_box_);
    right_click_->set_visible_window(false);
    right_click_->set_above_child(true);
    right_click_->set_tooltip_text("Click to choose this movie");
    right_click_->set_events(Gdk::BUTTON_PRESS_MASK | Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK);
    right_click_->signal_button_press_event().connect(sigc::mem_fun(*this, &Top100GtkRankDialog::on_right_click), false);
    right_click_->signal_enter_notify_event().connect(sigc::mem_fun(*this, &Top100GtkRankDialog::on_right_enter), false);
    right_click_->signal_leave_notify_event().connect(sigc::mem_fun(*this, &Top100GtkRankDialog::on_right_leave), false);

    row->pack_start(*left_click_, Gtk::PACK_EXPAND_WIDGET);
    // Vertical separator between the two movie panes
    auto* sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_VERTICAL));
    row->pack_start(*sep, Gtk::PACK_SHRINK);
    row->pack_start(*right_click_, Gtk::PACK_EXPAND_WIDGET);
    area->pack_start(*row, Gtk::PACK_EXPAND_WIDGET);

    auto* bottom = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    bottom->pack_start(*(Gtk::manage(new Gtk::Label())), Gtk::PACK_EXPAND_WIDGET); // spacer
    bottom->set_spacing(kSpacingSmall);
    bottom->pack_start(pass_btn_, Gtk::PACK_SHRINK);
    bottom->pack_start(finish_btn_, Gtk::PACK_SHRINK);
    area->pack_start(*bottom, Gtk::PACK_SHRINK);

    pass_btn_.signal_clicked().connect(sigc::mem_fun(*this, &Top100GtkRankDialog::pass_pair));
    finish_btn_.signal_clicked().connect([this]() { response(Gtk::RESPONSE_OK); });

    add_events(Gdk::KEY_PRESS_MASK);
    signal_key_press_event().connect([this](GdkEventKey* e) {
        switch (e->keyval) {
            case GDK_KEY_Left: choose_left(); return true;
            case GDK_KEY_Right: choose_right(); return true;
            case GDK_KEY_Down: pass_pair(); return true;
            case GDK_KEY_Return:
            case GDK_KEY_KP_Enter: pass_pair(); return true;
        }
        return false;
    });

    // Update poster scaling on size changes
    signal_size_allocate().connect([this](Gtk::Allocation&){ update_scaled_posters(); });
    pick_two();
    show_all_children();
}

void Top100GtkRankDialog::pick_two() {
    AppConfig cfg = loadConfig();
    Top100 list(cfg.dataFile);
    auto movies = list.getMovies(SortOrder::DEFAULT);
    int n = static_cast<int>(movies.size());
    if (n < 2) { left_index_ = right_index_ = -1; return; }
    static std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int> dist(0, n - 1);
    int a = dist(rng), b = dist(rng); if (b == a) b = (a + 1) % n;
    left_index_ = a; right_index_ = b;
    refresh_side(true);
    refresh_side(false);
}

void Top100GtkRankDialog::refresh_side(bool left) {
    AppConfig cfg = loadConfig();
    Top100 list(cfg.dataFile);
    auto movies = list.getMovies(SortOrder::DEFAULT);
    int idx = left ? left_index_ : right_index_;
    if (idx < 0 || idx >= static_cast<int>(movies.size())) return;
    const Movie& mv = movies[static_cast<size_t>(idx)];
    std::ostringstream title; title << "<b>" << mv.title << " (" << mv.year << ")</b>";
    Gtk::Label* t = left ? &left_title_ : &right_title_;
    Gtk::Label* d = left ? &left_details_ : &right_details_;
    t->set_markup(title.str());
    std::ostringstream det; det << "Director: " << mv.director << "\nActors: " << join(mv.actors)
        << "\nGenres: " << join(mv.genres) << "\nRuntime: " << (mv.runtimeMinutes > 0 ? std::to_string(mv.runtimeMinutes) + " min" : "");
    d->set_text(det.str());
    // Poster: load asynchronously
    if (!mv.posterUrl.empty()) {
        if (left) load_poster_async_for(left_poster_, left_orig_, mv.posterUrl);
        else      load_poster_async_for(right_poster_, right_orig_, mv.posterUrl);
    } else {
        if (left) { left_poster_.clear(); left_orig_.reset(); }
        else      { right_poster_.clear(); right_orig_.reset(); }
    }
}

void Top100GtkRankDialog::choose_left() {
    if (left_index_ < 0 || right_index_ < 0) return;
    // Elo update via core list
    AppConfig cfg = loadConfig();
    Top100 list(cfg.dataFile);
    auto movies = list.getMovies(SortOrder::DEFAULT);
    if (left_index_ >= static_cast<int>(movies.size()) || right_index_ >= static_cast<int>(movies.size())) return;
    Movie L = movies[left_index_], R = movies[right_index_];
    // Simple Elo update
    auto elo = [](double &a, double &b, double scoreA, double k=32.0){
        double qa = std::pow(10.0, a/400.0), qb = std::pow(10.0, b/400.0);
        double ea = qa/(qa+qb), eb = qb/(qa+qb);
        a = a + k * (scoreA - ea);
        b = b + k * ((1.0 - scoreA) - eb);
    };
    elo(L.userScore, R.userScore, 1.0);
    int li = list.findIndexByImdbId(L.imdbID);
    int ri = list.findIndexByImdbId(R.imdbID);
    if (li >= 0) list.updateMovie(static_cast<size_t>(li), L);
    if (ri >= 0) list.updateMovie(static_cast<size_t>(ri), R);
    list.recomputeRanks();
    pick_two();
}

void Top100GtkRankDialog::choose_right() {
    if (left_index_ < 0 || right_index_ < 0) return;
    AppConfig cfg = loadConfig();
    Top100 list(cfg.dataFile);
    auto movies = list.getMovies(SortOrder::DEFAULT);
    if (left_index_ >= static_cast<int>(movies.size()) || right_index_ >= static_cast<int>(movies.size())) return;
    Movie L = movies[left_index_], R = movies[right_index_];
    auto elo = [](double &a, double &b, double scoreA, double k=32.0){
        double qa = std::pow(10.0, a/400.0), qb = std::pow(10.0, b/400.0);
        double ea = qa/(qa+qb), eb = qb/(qa+qb);
        a = a + k * (scoreA - ea);
        b = b + k * ((1.0 - scoreA) - eb);
    };
    elo(L.userScore, R.userScore, 0.0);
    int li = list.findIndexByImdbId(L.imdbID);
    int ri = list.findIndexByImdbId(R.imdbID);
    if (li >= 0) list.updateMovie(static_cast<size_t>(li), L);
    if (ri >= 0) list.updateMovie(static_cast<size_t>(ri), R);
    list.recomputeRanks();
    pick_two();
}

void Top100GtkRankDialog::pass_pair() { pick_two(); }

Glib::RefPtr<Gdk::Pixbuf> Top100GtkRankDialog::scale_pixbuf(const Glib::RefPtr<Gdk::Pixbuf>& src, int maxW, int maxH) {
    if (!src) return {};
    int w = src->get_width(), h = src->get_height();
    if (w <= 0 || h <= 0) return {};
    double sx = static_cast<double>(maxW) / w;
    double sy = static_cast<double>(maxH) / h;
    double s = std::min(1.0, std::min(sx, sy));
    int tw = std::max(1, static_cast<int>(w * s));
    int th = std::max(1, static_cast<int>(h * s));
    return src->scale_simple(tw, th, Gdk::INTERP_BILINEAR);
}

void Top100GtkRankDialog::update_scaled_posters() {
    auto allocL = left_box_->get_allocation();
    auto allocR = right_box_->get_allocation();
    int maxWL = static_cast<int>(allocL.get_width() * kPosterMaxWidthRatio);
    int maxHL = static_cast<int>(allocL.get_height() * kPosterMaxHeightRatio);
    int maxWR = static_cast<int>(allocR.get_width() * kPosterMaxWidthRatio);
    int maxHR = static_cast<int>(allocR.get_height() * kPosterMaxHeightRatio);
    if (left_orig_)  if (auto s = scale_pixbuf(left_orig_, maxWL, maxHL))  left_poster_.set(s); else left_poster_.clear();
    if (right_orig_) if (auto s = scale_pixbuf(right_orig_, maxWR, maxHR)) right_poster_.set(s); else right_poster_.clear();
}

void Top100GtkRankDialog::load_poster_async_for(Gtk::Image& image, Glib::RefPtr<Gdk::Pixbuf>& store, const std::string& url) {
    std::thread([this, &image, &store, url]() {
        auto resp = cpr::Get(cpr::Url{url}, cpr::Timeout{8000});
        if (resp.error || resp.status_code != 200 || resp.text.empty()) return;
        auto img_bytes = std::make_shared<std::string>(std::move(resp.text));
        Glib::signal_idle().connect_once([this, &image, &store, img_bytes]() {
            try {
                auto loader = Gdk::PixbufLoader::create();
                loader->write(reinterpret_cast<const guint8*>(img_bytes->data()), img_bytes->size());
                loader->close();
                store = loader->get_pixbuf();
                update_scaled_posters();
            } catch (...) {
                image.clear();
            }
        });
    }).detach();
}

bool Top100GtkRankDialog::on_left_click(GdkEventButton*) { choose_left(); return true; }
bool Top100GtkRankDialog::on_right_click(GdkEventButton*) { choose_right(); return true; }

bool Top100GtkRankDialog::on_left_enter(GdkEventCrossing*) {
    if (auto win = left_click_->get_window()) {
        auto display = Gdk::Display::get_default();
        if (display) {
            auto cursor = Gdk::Cursor::create(display, Gdk::HAND1);
            win->set_cursor(cursor);
        }
    }
    return false;
}
bool Top100GtkRankDialog::on_left_leave(GdkEventCrossing*) {
    if (auto win = left_click_->get_window()) win->set_cursor();
    return false;
}
bool Top100GtkRankDialog::on_right_enter(GdkEventCrossing*) {
    if (auto win = right_click_->get_window()) {
        auto display = Gdk::Display::get_default();
        if (display) {
            auto cursor = Gdk::Cursor::create(display, Gdk::HAND1);
            win->set_cursor(cursor);
        }
    }
    return false;
}
bool Top100GtkRankDialog::on_right_leave(GdkEventCrossing*) {
    if (auto win = right_click_->get_window()) win->set_cursor();
    return false;
}

void gtk_open_rank_dialog(Gtk::Window& parent) {
    Top100GtkRankDialog dlg(parent);
    dlg.run();
}
