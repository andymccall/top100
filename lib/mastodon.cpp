// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: lib/mastodon.cpp
// Purpose: Mastodon API integration (verify, media upload, post status).
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "mastodon.h"
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

static std::string joinUrl(const std::string& base, const std::string& path) {
    if (base.empty()) return path;
    if (base.back() == '/' && !path.empty() && path.front() == '/') return base + path.substr(1);
    if (base.back() != '/' && !path.empty() && path.front() != '/') return base + "/" + path;
    return base + path;
}

bool mastoVerify(const std::string& instanceBaseUrl, const std::string& accessToken)
{
    if (instanceBaseUrl.empty() || accessToken.empty()) return false;
    auto url = joinUrl(instanceBaseUrl, "/api/v1/accounts/verify_credentials");
    auto r = cpr::Get(cpr::Url{url}, cpr::Bearer{accessToken});
    if (r.status_code != 200) return false;
    try {
        auto j = json::parse(r.text);
        return j.contains("id");
    } catch (...) {
        return false;
    }
}

std::optional<std::string> mastoUploadMedia(const std::string& instanceBaseUrl,
                                            const std::string& accessToken,
                                            const std::vector<unsigned char>& bytes,
                                            const std::string& filename,
                                            const std::string& contentType)
{
    if (bytes.empty()) return std::nullopt;
    auto url = joinUrl(instanceBaseUrl, "/api/v1/media");
    cpr::Buffer buf{bytes.begin(), bytes.end(), filename};
    cpr::Multipart mp{ { cpr::Part{"file", buf, contentType} } };
    auto r = cpr::Post(cpr::Url{url}, mp, cpr::Bearer{accessToken});
    if (r.status_code < 200 || r.status_code >= 300) return std::nullopt;
    try {
        auto j = json::parse(r.text);
        if (j.contains("id") && j["id"].is_string()) return j["id"].get<std::string>();
    } catch (...) {
    }
    return std::nullopt;
}

bool mastoPostStatus(const std::string& instanceBaseUrl,
                     const std::string& accessToken,
                     const std::string& text,
                     const std::optional<std::string>& mediaId)
{
    auto url = joinUrl(instanceBaseUrl, "/api/v1/statuses");
    cpr::Parameters params{};
    params.Add({"status", text});
    cpr::Header headers{{"Authorization", std::string("Bearer ") + accessToken}};
    if (mediaId && !mediaId->empty()) {
        // media_ids[] repeated parameter; cpr supports it by adding with same key
        params.Add({"media_ids[]", *mediaId});
    }
    auto r = cpr::Post(cpr::Url{url}, params, headers);
    return r.status_code >= 200 && r.status_code < 300;
}
