// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/gtk/poster.cpp
// Purpose: Poster loading and scaling for GTK window.
//-------------------------------------------------------------------------------
#include "poster.h"

#include <gdkmm/pixbufloader.h>
#include <glibmm/main.h>
#include <cpr/cpr.h>
#include <thread>

#include "../common/constants.h"

using namespace ui_constants;

// Scale and set the poster image to fit the right pane while preserving aspect
void Top100GtkWindow::update_poster_scaled() {
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
void Top100GtkWindow::load_poster_async(const std::string& url, const std::string& imdb) {
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
