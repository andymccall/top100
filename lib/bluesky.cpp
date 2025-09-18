//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/bluesky.cpp
// Purpose: BlueSky API integration (session, blob upload, create post).
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "bluesky.h"
#include "Movie.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <chrono>

using nlohmann::json;

static std::string nowIso8601() {
    using namespace std::chrono;
    auto tp = time_point_cast<seconds>(system_clock::now());
    std::time_t t = system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::optional<BlueSkySession> bskyCreateSession(const std::string& serviceBase,
                                                const std::string& identifier,
                                                const std::string& appPassword) {
    json body{{"identifier", identifier}, {"password", appPassword}};
    auto r = cpr::Post(cpr::Url{serviceBase + "/xrpc/com.atproto.server.createSession"},
                       cpr::Header{{"Content-Type", "application/json"}},
                       cpr::Body{body.dump()});
    if (r.status_code != 200) return std::nullopt;
    auto j = json::parse(r.text, nullptr, false);
    if (j.is_discarded()) return std::nullopt;
    BlueSkySession s;
    s.accessJwt = j.value("accessJwt", "");
    s.did = j.value("did", "");
    if (s.accessJwt.empty() || s.did.empty()) return std::nullopt;
    return s;
}

std::optional<std::string> bskyUploadImage(const std::string& serviceBase,
                                           const std::string& accessJwt,
                                           const std::vector<unsigned char>& bytes,
                                           const std::string& contentType) {
    auto r = cpr::Post(cpr::Url{serviceBase + "/xrpc/com.atproto.repo.uploadBlob"},
                       cpr::Header{{"Content-Type", contentType}, {"Authorization", std::string("Bearer ") + accessJwt}},
                       cpr::Body{std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size())});
    if (r.status_code != 200) return std::nullopt;
    auto j = json::parse(r.text, nullptr, false);
    if (j.is_discarded() || !j.contains("blob")) return std::nullopt;
    return j["blob"].dump();
}

bool bskyCreatePost(const std::string& serviceBase,
                    const std::string& accessJwt,
                    const std::string& repoDid,
                    const std::string& text,
                    const std::optional<std::string>& imageBlobJson) {
    json record = {
        {"$type", "app.bsky.feed.post"},
        {"text", text},
        {"createdAt", nowIso8601()}
    };
    if (imageBlobJson.has_value()) {
        record["embed"] = json{
            {"$type", "app.bsky.embed.images"},
            {"images", json::array({ json{{"alt", "Movie poster"}, {"image", json::parse(*imageBlobJson)} } })}
        };
    }

    json body = {
        {"repo", repoDid},
        {"collection", "app.bsky.feed.post"},
        {"record", record}
    };

    auto r = cpr::Post(cpr::Url{serviceBase + "/xrpc/com.atproto.repo.createRecord"},
                       cpr::Header{{"Content-Type", "application/json"}, {"Authorization", std::string("Bearer ") + accessJwt}},
                       cpr::Body{body.dump()});
    return r.status_code == 200;
}

bool bskyPostMovie(const std::string& serviceBase,
                   const std::string& identifier,
                   const std::string& appPassword,
                   const Movie& movie) {
    auto session = bskyCreateSession(serviceBase, identifier, appPassword);
    if (!session.has_value()) return false;

    // Create text content (header/title/rank/details/footer assembled by caller; default kept here for safety)
    std::ostringstream text;
    text << "🎬 " << movie.title << " (" << movie.year << ")\n";
    if (!movie.director.empty()) text << "🎥 Directed by: " << movie.director << "\n";
    if (movie.userRank > 0) text << "⭐ My ranking: #" << movie.userRank << "/100\n";
    if (movie.imdbRating > 0.0) text << "⭐ IMDb rating: " << movie.imdbRating << "/10\n";
    if (!movie.imdbID.empty()) text << "🔗 https://www.imdb.com/title/" << movie.imdbID << "/";

    std::optional<std::string> blob;
    if (!movie.posterUrl.empty() && movie.posterUrl != "N/A") {
        auto img = cpr::Get(cpr::Url{movie.posterUrl});
        if (img.status_code == 200 && !img.text.empty()) {
            // Try to detect content type; fallback to jpeg
            std::string contentType = "image/jpeg";
            auto it = img.header.find("content-type");
            if (it != img.header.end() && !it->second.empty()) {
                contentType = it->second;
            }
            std::vector<unsigned char> bytes(img.text.begin(), img.text.end());
            blob = bskyUploadImage(serviceBase, session->accessJwt, bytes, contentType);
        }
    }

    return bskyCreatePost(serviceBase, session->accessJwt, session->did, text.str(), blob);
}
