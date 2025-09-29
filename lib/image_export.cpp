// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
// Export a PNG summary image with a heading and a 5x20 grid of movies.
//-------------------------------------------------------------------------------
#include "image_export.h"
#include "Movie.h"
#include <cpr/cpr.h>
#include <cairo/cairo.h>
#include <cairo/cairo-pdf.h>
#include <filesystem>
#include <cstring>
#include <optional>
#ifndef TOP100_NO_SQLITE
#include <sqlite3.h>
#endif
#include "config.h"
#include "top100.h"
#if TOP100_HAVE_JPEG
extern "C" {
#include <jpeglib.h>
}
#endif
#include <sstream>

namespace {
struct Image {
    cairo_surface_t* surface = nullptr;
    int w = 0;
    int h = 0;
    Image() = default;
    // Unique ownership of the cairo surface
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;
    Image(Image&& other) noexcept { surface = other.surface; w = other.w; h = other.h; other.surface = nullptr; other.w = other.h = 0; }
    Image& operator=(Image&& other) noexcept {
        if (this != &other) {
            if (surface) cairo_surface_destroy(surface);
            surface = other.surface; w = other.w; h = other.h;
            other.surface = nullptr; other.w = other.h = 0;
        }
        return *this;
    }
    ~Image() { if (surface) cairo_surface_destroy(surface); }
};

// Load image bytes into a Cairo surface. Try PNG via Cairo stream first; optionally JPEG via libjpeg.
Image loadImageFromBytes(const std::vector<unsigned char>& bytes) {
    Image img;
    if (bytes.empty()) return img;
    struct ReadCtx { const unsigned char* p; size_t n; size_t off; } ctx{bytes.data(), bytes.size(), 0};
    auto read_cb = [](void* closure, unsigned char* data, unsigned int length) -> cairo_status_t {
        ReadCtx* c = static_cast<ReadCtx*>(closure);
        if (c->off + length > c->n) return CAIRO_STATUS_READ_ERROR;
        memcpy(data, c->p + c->off, length);
        c->off += length;
        return CAIRO_STATUS_SUCCESS;
    };
    cairo_surface_t* s = cairo_image_surface_create_from_png_stream(read_cb, &ctx);
    if (cairo_surface_status(s) == CAIRO_STATUS_SUCCESS) {
        img.surface = s;
        img.w = cairo_image_surface_get_width(s);
        img.h = cairo_image_surface_get_height(s);
        return img;
    }
    if (s) cairo_surface_destroy(s);
    #if TOP100_HAVE_JPEG
    // Minimal libjpeg decode path (RGB -> BGRA copy)
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, const_cast<unsigned char*>(bytes.data()), bytes.size());
    if (jpeg_read_header(&cinfo, TRUE) == JPEG_HEADER_OK) {
        cinfo.out_color_space = JCS_RGB; // Standard RGB output
        jpeg_start_decompress(&cinfo);
        int w = static_cast<int>(cinfo.output_width);
        int h = static_cast<int>(cinfo.output_height);
        cairo_surface_t* js = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        if (js && cairo_surface_status(js) == CAIRO_STATUS_SUCCESS) {
            unsigned char* dst = cairo_image_surface_get_data(js);
            int stride = cairo_image_surface_get_stride(js);
            // Allocate a row buffer in RGB
            JSAMPARRAY buffer;
            int row_stride = w * 3;
            buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
            while (cinfo.output_scanline < cinfo.output_height) {
                jpeg_read_scanlines(&cinfo, buffer, 1);
                unsigned char* src = buffer[0];
                unsigned char* drow = dst + (cinfo.output_scanline - 1) * stride;
                // Convert RGB -> BGRA with opaque alpha
                for (int x = 0; x < w; ++x) {
                    unsigned char r = src[x*3 + 0];
                    unsigned char g = src[x*3 + 1];
                    unsigned char b = src[x*3 + 2];
                    drow[x*4+0] = b; drow[x*4+1] = g; drow[x*4+2] = r; drow[x*4+3] = 255;
                }
            }
            cairo_surface_mark_dirty(js);
            img.surface = js; img.w = w; img.h = h;
        }
        jpeg_finish_decompress(&cinfo);
    }
    jpeg_destroy_decompress(&cinfo);
    if (img.surface) return img;
    #endif
    // Non-PNG input: skip image.
    return img;
}

void drawCenteredText(cairo_t* cr, double cx, double y, const std::string& text, bool bold=false, double size=14.0) {
    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, bold ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size);
    cairo_text_extents_t ext; cairo_text_extents(cr, text.c_str(), &ext);
    double x = cx - (ext.width / 2.0 + ext.x_bearing);
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, text.c_str());
}

} // namespace

namespace {

#ifndef TOP100_NO_SQLITE
// Ensure posters cache table exists
void ensurePosterCache(sqlite3* db) {
    static bool done = false; if (done) return; done = true;
    const char* sql = R"SQL(
        CREATE TABLE IF NOT EXISTS posters(
            imdbID TEXT PRIMARY KEY,
            mime TEXT,
            data BLOB,
            updatedAt INTEGER
        );
    )SQL";
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

// Read cached poster bytes by imdbID
std::optional<std::vector<unsigned char>> readPoster(sqlite3* db, const std::string& imdbID) {
    if (!db || imdbID.empty()) return std::nullopt;
    ensurePosterCache(db);
    const char* sel = "SELECT data FROM posters WHERE imdbID=?";
    sqlite3_stmt* st = nullptr; if (sqlite3_prepare_v2(db, sel, -1, &st, nullptr) != SQLITE_OK) return std::nullopt;
    sqlite3_bind_text(st, 1, imdbID.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<std::vector<unsigned char>> out;
    if (sqlite3_step(st) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(st, 0);
        int n = sqlite3_column_bytes(st, 0);
        if (blob && n > 0) {
            const unsigned char* p = static_cast<const unsigned char*>(blob);
            out = std::vector<unsigned char>(p, p + n);
        }
    }
    sqlite3_finalize(st);
    return out;
}

// Upsert poster bytes into cache
void writePoster(sqlite3* db, const std::string& imdbID, const std::string& mime, const std::vector<unsigned char>& bytes) {
    if (!db || imdbID.empty() || bytes.empty()) return; ensurePosterCache(db);
    const char* ins = "INSERT INTO posters(imdbID,mime,data,updatedAt) VALUES(?,?,?,strftime('%s','now')) ON CONFLICT(imdbID) DO UPDATE SET mime=excluded.mime,data=excluded.data,updatedAt=excluded.updatedAt";
    sqlite3_stmt* st = nullptr; if (sqlite3_prepare_v2(db, ins, -1, &st, nullptr) != SQLITE_OK) return;
    sqlite3_bind_text(st, 1, imdbID.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(st, 2, mime.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_blob(st, 3, bytes.data(), static_cast<int>(bytes.size()), SQLITE_TRANSIENT);
    sqlite3_step(st);
    sqlite3_finalize(st);
}
#endif

// Best-effort get SQLite handle from the current data file
sqlite3* tryOpenDbFromConfig() {
#ifdef TOP100_NO_SQLITE
    return nullptr;
#else
    try {
        AppConfig cfg = loadConfig();
        sqlite3* db = nullptr;
        if (sqlite3_open(cfg.dataFile.c_str(), &db) == SQLITE_OK) {
            return db;
        }
        if (db) sqlite3_close(db);
    } catch (...) {}
    return nullptr;
#endif
}

// Determine default export path under ~/Pictures if available
std::string defaultExportPath() {
    const char* home = std::getenv("HOME");
    std::filesystem::path base = home ? std::filesystem::path(home) : std::filesystem::path(".");
    std::filesystem::path pics1 = base / "Pictures";
    std::filesystem::path pics2 = base / "pictures";
    if (std::filesystem::exists(pics1) && std::filesystem::is_directory(pics1)) return (pics1 / "top100.png").string();
    if (std::filesystem::exists(pics2) && std::filesystem::is_directory(pics2)) return (pics2 / "top100.png").string();
    return (base / "top100.png").string();
}

} // namespace

bool exportTop100Image(const std::vector<Movie>& movies,
                       const std::string& outPath,
                       const std::string& heading)
{
    // Layout: 5 columns x 20 rows
    const int cols = 5, rows = 20;
    const int cellW = 180, cellH = 240; // allows number + image + title
    const int margin = 40;
    const int headingH = 110; // more space for subtitle
    const int width = margin*2 + cols*cellW;
    const int height = margin*2 + headingH + rows*cellH;

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    if (!surface || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) return false;
    cairo_t* cr = cairo_create(surface);

    // Background
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_paint(cr);

    // Heading and subtitle
    cairo_set_source_rgb(cr, 0, 0, 0);
    const double titleY = margin + 44.0;
    drawCenteredText(cr, width/2.0, titleY, heading, true, 28.0);
    const std::string subtitle = "Generated by Top 100 - https://github.com/andymccall/top100";
    drawCenteredText(cr, width/2.0, titleY + 26.0, subtitle, false, 14.0);

    // For each movie cell
    const size_t n = std::min<size_t>(movies.size(), 100);
    for (size_t i = 0; i < n; ++i) {
        int r = i / cols;
        int c = i % cols;
        double x0 = margin + c*cellW;
    double y0 = margin + headingH + r*cellH;

        // Number label centered at top
        std::ostringstream num; num << (i+1) << ".";
        drawCenteredText(cr, x0 + cellW/2.0, y0 + 18, num.str(), true, 16.0);

        // Poster: fetch and scale to fit box
        double posterTop = y0 + 28;
        double posterMaxW = cellW - 10;
        double posterMaxH = cellH - 70; // leave space for title

    Image img;
        const auto& url = movies[i].posterUrl;
        const auto& imdb = movies[i].imdbID;
        std::vector<unsigned char> posterBytes;
#ifndef TOP100_NO_SQLITE
        // Try cache first
        sqlite3* db = tryOpenDbFromConfig();
        if (db && !imdb.empty()) {
            if (auto cached = readPoster(db, imdb)) {
                posterBytes = *cached;
            }
        }
#endif
        // If cache miss, fetch
        if (posterBytes.empty() && !url.empty() && url != "N/A") {
            auto r = cpr::Get(cpr::Url{url});
            if (r.status_code == 200 && !r.text.empty()) {
                posterBytes.assign(r.text.begin(), r.text.end());
#ifndef TOP100_NO_SQLITE
                if (db && !imdb.empty()) {
                    std::string mime;
                    auto it = r.header.find("content-type");
                    if (it != r.header.end()) mime = it->second;
                    if (mime.empty()) {
                        // naive mime by sniffing
                        if (posterBytes.size() >= 8 && posterBytes[0]==0x89 && posterBytes[1]=='P' && posterBytes[2]=='N' && posterBytes[3]=='G') mime = "image/png";
                        else if (posterBytes.size() > 2 && posterBytes[0]==0xFF && posterBytes[1]==0xD8) mime = "image/jpeg";
                        else mime = "application/octet-stream";
                    }
                    writePoster(db, imdb, mime, posterBytes);
                }
#endif
            }
        }
        if (!posterBytes.empty()) {
            img = loadImageFromBytes(posterBytes); // move into our unique owner
        }

        if (img.surface) {
            double iw = img.w, ih = img.h;
            double scale = std::min(posterMaxW/iw, posterMaxH/ih);
            double w = iw*scale, h = ih*scale;
            double px = x0 + (cellW - w) / 2.0;
            double py = posterTop + (posterMaxH - h) / 2.0;
            cairo_save(cr);
            cairo_translate(cr, px, py);
            cairo_scale(cr, scale, scale);
            cairo_set_source_surface(cr, img.surface, 0, 0);
            cairo_rectangle(cr, 0, 0, iw, ih);
            cairo_fill(cr);
            cairo_restore(cr);
        } else {
            // Draw placeholder rectangle
            cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
            cairo_rectangle(cr, x0 + 5, posterTop, posterMaxW, posterMaxH);
            cairo_fill(cr);
            cairo_set_source_rgb(cr, 0.7, 0.7, 0.7);
            cairo_rectangle(cr, x0 + 5, posterTop, posterMaxW, posterMaxH);
            cairo_stroke(cr);
        }

        // Title + (Year) centered near bottom, elide if too long
        std::ostringstream tt; tt << movies[i].title << " (" << movies[i].year << ")";
        cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, 12.0);
        cairo_text_extents_t ext{}; cairo_text_extents(cr, tt.str().c_str(), &ext);
        std::string t = tt.str();
        // naive elide: trim until fits within cellW - 8
        while (ext.width > cellW - 8 && t.size() > 4) {
            t.pop_back();
            t.back() = '.'; // ensure something changes
            cairo_text_extents(cr, t.c_str(), &ext);
        }
        double ty = y0 + cellH - 10;
        double tx = x0 + (cellW/2.0) - (ext.width/2.0 + ext.x_bearing);
        cairo_move_to(cr, tx, ty);
        cairo_set_source_rgb(cr, 0, 0, 0);
        cairo_show_text(cr, t.c_str());
    }

    cairo_destroy(cr);
    std::string out = outPath.empty() ? defaultExportPath() : outPath;
    cairo_status_t st = cairo_surface_write_to_png(surface, out.c_str());
    cairo_surface_destroy(surface);
    return st == CAIRO_STATUS_SUCCESS;
}
