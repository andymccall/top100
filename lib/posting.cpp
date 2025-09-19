// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/posting.cpp
// Purpose: Shared helpers to compose and post to BlueSky and Mastodon.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "posting.h"
#include "config.h"
#include "Movie.h"
#include "bluesky.h"
#include "mastodon.h"
#include <sstream>
#include <cpr/cpr.h>

static size_t utf8_length(const std::string& s) {
    size_t count = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) ++count;
    }
    return count;
}

static std::string utf8_truncate(const std::string& s, size_t maxCodepoints) {
    if (maxCodepoints == 0) return std::string();
    std::string out;
    out.reserve(s.size());
    size_t count = 0;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
    size_t i = 0, n = s.size();
    while (i < n && count < maxCodepoints) {
        unsigned char c = p[i];
        size_t charLen = 1;
        if ((c & 0x80) == 0x00) charLen = 1;
        else if ((c & 0xE0) == 0xC0) charLen = 2;
        else if ((c & 0xF0) == 0xE0) charLen = 3;
        else if ((c & 0xF8) == 0xF0) charLen = 4;
        if (i + charLen > n) break;
        out.append(s, i, charLen);
        i += charLen;
        ++count;
    }
    return out;
}

std::string composePostBody(const AppConfig& cfg, const Movie& m, size_t limit)
{
    const std::string header = cfg.postHeaderText;
    const std::string footer = cfg.postFooterText;
    const std::string titleLine = "\xF0\x9F\x8E\xAC " + m.title + " (" + std::to_string(m.year) + ")\n"; // 🎬
    const std::string directorLine = m.director.empty() ? std::string("") : ("\xF0\x9F\x8E\xA5 Director: " + m.director + "\n"); // 🎥
    const std::string blankAfterDirector = "\n";
    const std::string plotOriginal = !m.plotShort.empty() ? m.plotShort : m.plotFull;
    const std::string rankingPrefix = "\xE2\xAD\x90 My ranking: "; // ⭐
    const std::string rankingValue = (m.userRank > 0) ? ("#" + std::to_string(m.userRank) + "/100") : std::string("");
    const std::string rankingLine = rankingPrefix + rankingValue + "\n";
    const std::string imdbLine = (m.imdbRating > 0.0) ? ("\xE2\xAD\x90 IMDb ranking: " + std::to_string(m.imdbRating) + "/10\n") : std::string("");
    const std::string linkLine = m.imdbID.empty() ? std::string("") : ("\xF0\x9F\x94\x97 https://www.imdb.com/title/" + m.imdbID + "/\n"); // 🔗

    auto buildBody = [&](const std::string& plotText, bool includeFooter) {
        std::ostringstream oss;
        if (!header.empty()) oss << header << "\n\n";
        oss << titleLine;
        if (!directorLine.empty()) oss << directorLine;
        oss << blankAfterDirector;
        if (!plotText.empty()) oss << "Plot: " << plotText << "\n\n";
        oss << rankingLine;
        if (!imdbLine.empty()) oss << imdbLine;
        if (!linkLine.empty()) oss << linkLine;
        if (includeFooter && !footer.empty()) oss << "\n" << footer;
        return oss.str();
    };

    // 1) Try full
    std::string bodyFull = buildBody(plotOriginal, true);
    if (utf8_length(bodyFull) <= limit) return bodyFull;

    // 2) Try without footer
    std::string bodyNoFooter = buildBody(plotOriginal, false);
    if (utf8_length(bodyNoFooter) <= limit) return bodyNoFooter;

    // 3) Truncate plot to fit with footer and ellipsis
    const std::string ellipsis = "...";
    std::ostringstream fixed;
    if (!header.empty()) fixed << header << "\n\n";
    fixed << titleLine;
    if (!directorLine.empty()) fixed << directorLine;
    fixed << blankAfterDirector;
    fixed << "Plot: " << "" << "\n\n";
    fixed << rankingLine;
    if (!imdbLine.empty()) fixed << imdbLine;
    if (!linkLine.empty()) fixed << linkLine;
    if (!footer.empty()) fixed << "\n" << footer;
    size_t fixedLen = utf8_length(fixed.str());
    size_t ellLen = utf8_length(ellipsis);
    size_t budget = (limit > fixedLen) ? (limit - fixedLen) : 0;
    size_t keep = (budget > ellLen) ? (budget - ellLen) : 0;
    std::string truncated = utf8_truncate(plotOriginal, keep) + ellipsis;
    std::string finalBody = buildBody(truncated, true);
    if (utf8_length(finalBody) <= limit) return finalBody;

    // Binary search to ensure under limit
    size_t low = 0, high = keep;
    while (low < high) {
        size_t mid = (low + high) / 2;
        std::string t = utf8_truncate(plotOriginal, mid) + ellipsis;
        std::string b = buildBody(t, true);
        if (utf8_length(b) <= limit) { low = mid + 1; finalBody = b; }
        else { high = mid; }
    }
    return finalBody;
}

bool postMovieToBlueSky(const AppConfig& cfg, const Movie& m)
{
    const size_t LIMIT = 300;
    std::string service = cfg.blueSkyService.empty() ? std::string("https://bsky.social") : cfg.blueSkyService;
    auto session = bskyCreateSession(service, cfg.blueSkyIdentifier, cfg.blueSkyAppPassword);
    if (!session) return false;

    std::optional<std::string> blob;
    if (!m.posterUrl.empty() && m.posterUrl != "N/A") {
        auto img = cpr::Get(cpr::Url{m.posterUrl});
        if (img.status_code == 200 && !img.text.empty()) {
            std::string contentType = "image/jpeg";
            auto it = img.header.find("content-type");
            if (it != img.header.end() && !it->second.empty()) contentType = it->second;
            std::vector<unsigned char> bytes(img.text.begin(), img.text.end());
            blob = bskyUploadImage(service, session->accessJwt, bytes, contentType);
        }
    }
    std::string body = composePostBody(cfg, m, LIMIT);
    return bskyCreatePost(service, session->accessJwt, session->did, body, blob);
}

bool postMovieToMastodon(const AppConfig& cfg, const Movie& m)
{
    const size_t LIMIT = 500;
    std::optional<std::string> mediaId;
    if (!m.posterUrl.empty() && m.posterUrl != "N/A") {
        auto img = cpr::Get(cpr::Url{m.posterUrl});
        if (img.status_code == 200 && !img.text.empty()) {
            std::string contentType = "image/jpeg";
            auto it = img.header.find("content-type");
            if (it != img.header.end() && !it->second.empty()) contentType = it->second;
            std::string filename = "poster";
            if (contentType.find("png") != std::string::npos) filename += ".png"; else filename += ".jpg";
            std::vector<unsigned char> bytes(img.text.begin(), img.text.end());
            mediaId = mastoUploadMedia(cfg.mastodonInstance, cfg.mastodonAccessToken, bytes, filename, contentType);
        }
    }
    std::string body = composePostBody(cfg, m, LIMIT);
    return mastoPostStatus(cfg.mastodonInstance, cfg.mastodonAccessToken, body, mediaId);
}
