//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: cli/main.cpp
// Purpose: CLI entry point and command routing (OMDb, ranking, details, BlueSky, Mastodon).
// Language: C++17 (CMake build)
//
// Summary:
// Orchestrates the interactive menu, loads configuration and data, and dispatches
// actions including adding/removing/listing movies, Elo ranking, OMDb integration,
// and social posting with character-limit handling.
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
#include "main.h"
#include "comparemovies.h"
#include "config.h"
#include "omdb.h"
#include "config_utils.h"
#include "bluesky.h"
#include "mastodon.h"
#include <cpr/cpr.h>
#include <limits>

int main()
{
    // Load or create configuration
    AppConfig cfg = loadConfig();

    Top100 top100(cfg.dataFile);
    // Ensure ranks exist on startup (for legacy data)
    top100.recomputeRanks();
    char input;

    do
    {
    displayMenu(cfg.omdbEnabled, cfg.blueSkyEnabled, cfg.mastodonEnabled);
        std::cin >> input;

        switch (input)
        {
        case '1':
            addMovie(top100);
            break;
        case '2':
            removeMovie(top100);
            break;
        case '3':
            listMovies(top100);
            break;
        case '4':
            if (cfg.omdbEnabled) {
                addFromOmdb(top100, cfg.omdbApiKey);
            } else {
                // Configure OMDb API key
                std::cout << "Enter OMDb API key: ";
                std::string key;
                std::cin >> key;
                std::cout << "Verifying key...\n";
                if (configureOmdb(cfg, key, omdbVerifyKey)) {
                    std::cout << "OMDb API key verified and saved.\n";
                } else {
                    std::cout << "Invalid OMDb API key. Please try again.\n";
                }
            }
            break;
        case '5':
            if (cfg.omdbEnabled) {
                // Disable OMDb
                disableOmdb(cfg);
                std::cout << "OMDb disabled.\n";
            } else {
                // Set data file path
                std::cout << "Enter path for data file (top100.json): ";
                std::string path;
                std::cin >> path;
                if (setDataFile(cfg, path)) {
                    std::cout << "Data path updated: " << cfg.dataFile << "\n";
                    // Reopen Top100 with new path
                    top100 = Top100(cfg.dataFile);
                    top100.recomputeRanks();
                } else {
                    std::cout << "Invalid path, not updated.\n";
                }
            }
            break;
        case '6':
            // consume leftover newline before getline in viewDetails
            if (std::cin.peek() == '\n') std::cin.get();
            viewDetails(top100);
            break;
        case '7':
            compareMovies(top100);
            break;
        case '8':
            if (cfg.blueSkyEnabled) {
                // Select a movie to post
                std::cout << "Enter the exact movie title to post: ";
                if (std::cin.peek() == '\n') std::cin.get();
                {
                    std::string title;
                    std::getline(std::cin, title);
                    // Try to find by exact title; if multiple, ask for year
                    std::vector<Movie> all = top100.getMovies(SortOrder::DEFAULT);
                    std::vector<const Movie*> matches;
                    for (const auto& m : all) if (m.title == title) matches.push_back(&m);
                    const Movie* chosen = nullptr;
                    if (matches.empty()) {
                        std::cout << "No movie found with that title.\n";
                    } else if (matches.size() == 1) {
                        chosen = matches.front();
                    } else {
                        std::cout << "Multiple matches. Enter year: ";
                        int y = 0; std::cin >> y;
                        for (auto* m : matches) if (m->year == y) { chosen = m; break; }
                        if (!chosen) { std::cout << "No exact title+year match.\n"; }
                    }
                    if (chosen) {
                        std::cout << "Posting to BlueSky...\n";

                        // Compose post components
                        const size_t LIMIT = 300; // BlueSky character limit
                        auto service = cfg.blueSkyService.empty() ? std::string("https://bsky.social") : cfg.blueSkyService;
                        const std::string header = cfg.postHeaderText;
                        const std::string footer = cfg.postFooterText;
                        const std::string titleLine = "🎬 " + chosen->title + " (" + std::to_string(chosen->year) + ")\n";
                        const std::string directorLine = chosen->director.empty() ? std::string("") : ("🎥 Director: " + chosen->director + "\n");
                        const std::string blankAfterDirector = "\n";
                        const std::string plotOriginal = !chosen->plotShort.empty() ? chosen->plotShort : chosen->plotFull;
                        const std::string rankingPrefix = "⭐ My ranking: ";
                        const std::string rankingValue = (chosen->userRank > 0) ? ("#" + std::to_string(chosen->userRank) + "/100") : std::string("");
                        const std::string rankingLine = rankingPrefix + rankingValue + "\n";
                        const std::string imdbLine = (chosen->imdbRating > 0.0) ? ("⭐ IMDb ranking: " + std::to_string(chosen->imdbRating) + "/10\n") : std::string("");
                        const std::string linkLine = chosen->imdbID.empty() ? std::string("") : ("🔗 https://www.imdb.com/title/" + chosen->imdbID + "/\n");

                        // Doxygen: count Unicode code points in a UTF-8 string (not bytes).
                        // @ingroup cli
                        auto utf8_length = [](const std::string& s) -> size_t {
                            size_t count = 0;
                            for (unsigned char c : s) {
                                // count bytes that are not continuation bytes (10xxxxxx)
                                if ((c & 0xC0) != 0x80) ++count;
                            }
                            return count;
                        };
                        // Doxygen: truncate a UTF-8 string by code points, preserving boundaries.
                        // Appends no suffix; caller may add an ellipsis.
                        // @ingroup cli
                        auto utf8_truncate = [](const std::string& s, size_t maxCodepoints) -> std::string {
                            if (maxCodepoints == 0) return std::string();
                            std::string out;
                            out.reserve(s.size());
                            size_t count = 0;
                            const unsigned char* p = reinterpret_cast<const unsigned char*>(s.data());
                            size_t i = 0, n = s.size();
                            while (i < n && count < maxCodepoints) {
                                unsigned char c = p[i];
                                size_t charLen = 1;
                                if ((c & 0x80) == 0x00) charLen = 1;           // 0xxxxxxx
                                else if ((c & 0xE0) == 0xC0) charLen = 2;     // 110xxxxx
                                else if ((c & 0xF0) == 0xE0) charLen = 3;     // 1110xxxx
                                else if ((c & 0xF8) == 0xF0) charLen = 4;     // 11110xxx
                                if (i + charLen > n) break; // safety
                                out.append(s, i, charLen);
                                i += charLen;
                                ++count;
                            }
                            return out;
                        };

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

                        // 1) Try full post
                        std::string bodyFull = buildBody(plotOriginal, true);
                        std::string finalBody;
                        if (utf8_length(bodyFull) <= LIMIT) {
                            finalBody = bodyFull;
                        } else {
                            // 2) Try removing footer (no trailing blank line when footer omitted)
                            std::string bodyNoFooter = buildBody(plotOriginal, false);
                            if (utf8_length(bodyNoFooter) <= LIMIT) {
                                finalBody = bodyNoFooter;
                            } else {
                                // 3) Keep footer, truncate plot to fit and add ellipsis
                                const std::string ellipsis = "...";
                                // Length of everything except the plot content
                                // We need the length excluding plot text but including the plot prefix/suffix and footer
                                // Rebuild the fixed parts explicitly
                                std::ostringstream fixed;
                                if (!header.empty()) fixed << header << "\n\n";
                                fixed << titleLine;
                                if (!directorLine.empty()) fixed << directorLine;
                                fixed << blankAfterDirector;
                                // Add plot prefix/suffix but not content
                                fixed << "Plot: " << "" << "\n\n";
                                fixed << rankingLine;
                                if (!imdbLine.empty()) fixed << imdbLine;
                                if (!linkLine.empty()) fixed << linkLine;
                                if (!footer.empty()) fixed << "\n" << footer;
                                size_t fixedLen = utf8_length(fixed.str());
                                // Budget in code points for plot content plus ellipsis
                                size_t ellLen = utf8_length(ellipsis);
                                size_t budget = (LIMIT > fixedLen) ? (LIMIT - fixedLen) : 0;
                                size_t keep = (budget > ellLen) ? (budget - ellLen) : 0;
                                std::string truncated = utf8_truncate(plotOriginal, keep) + ellipsis;
                                finalBody = buildBody(truncated, true);
                                if (utf8_length(finalBody) > LIMIT && keep > 0) {
                                    // Ensure under limit by shaving a few more chars if needed
                                    // Estimate overshoot in code points
                                    size_t low = 0, high = keep;
                                    while (low < high) {
                                        size_t mid = (low + high) / 2;
                                        std::string t = utf8_truncate(plotOriginal, mid) + ellipsis;
                                        std::string b = buildBody(t, true);
                                        if (utf8_length(b) <= LIMIT) { low = mid + 1; finalBody = b; truncated = t; }
                                        else { high = mid; }
                                    }
                                }
                            }
                        }

                        auto session = bskyCreateSession(service, cfg.blueSkyIdentifier, cfg.blueSkyAppPassword);
                        bool ok = false;
                        if (session) {
                            std::optional<std::string> blob;
                            if (!chosen->posterUrl.empty() && chosen->posterUrl != "N/A") {
                                auto img = cpr::Get(cpr::Url{chosen->posterUrl});
                                if (img.status_code == 200 && !img.text.empty()) {
                                    std::string contentType = "image/jpeg";
                                    auto it = img.header.find("content-type");
                                    if (it != img.header.end() && !it->second.empty()) contentType = it->second;
                                    std::vector<unsigned char> bytes(img.text.begin(), img.text.end());
                                    blob = bskyUploadImage(service, session->accessJwt, bytes, contentType);
                                }
                            }
                            ok = bskyCreatePost(service, session->accessJwt, session->did, finalBody, blob);
                        }
                        std::cout << (ok ? "Posted successfully.\n" : "Failed to post.\n");
                    }
                }
            } else {
                // Configure BlueSky account
                std::cout << "Enter BlueSky handle/email: ";
                std::string id; std::cin >> id;
                std::cout << "Enter BlueSky app password: ";
                std::string pw; std::cin >> pw;
                std::cout << "Service URL (default https://bsky.social): ";
                std::string svc; std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::getline(std::cin, svc);
                if (svc.empty()) svc = "https://bsky.social";
                auto session = bskyCreateSession(svc, id, pw);
                if (session) {
                    cfg.blueSkyEnabled = true;
                    cfg.blueSkyIdentifier = id;
                    cfg.blueSkyAppPassword = pw;
                    cfg.blueSkyService = svc;
                    saveConfig(cfg);
                    std::cout << "BlueSky account verified and saved.\n";
                } else {
                    std::cout << "Could not verify BlueSky credentials.\n";
                }
            }
            break;
        case '9':
            if (cfg.mastodonEnabled) {
                // Select a movie to post
                std::cout << "Enter the exact movie title to post: ";
                if (std::cin.peek() == '\n') std::cin.get();
                {
                    std::string title;
                    std::getline(std::cin, title);
                    std::vector<Movie> all = top100.getMovies(SortOrder::DEFAULT);
                    std::vector<const Movie*> matches;
                    for (const auto& m : all) if (m.title == title) matches.push_back(&m);
                    const Movie* chosen = nullptr;
                    if (matches.empty()) {
                        std::cout << "No movie found with that title.\n";
                    } else if (matches.size() == 1) {
                        chosen = matches.front();
                    } else {
                        std::cout << "Multiple matches. Enter year: ";
                        int y = 0; std::cin >> y;
                        for (auto* m : matches) if (m->year == y) { chosen = m; break; }
                        if (!chosen) { std::cout << "No exact title+year match.\n"; }
                    }
                    if (chosen) {
                        std::cout << "Posting to Mastodon...\n";

                        const size_t LIMIT = 500; // Mastodon typical character limit
                        const std::string header = cfg.postHeaderText;
                        const std::string footer = cfg.postFooterText;
                        const std::string titleLine = "🎬 " + chosen->title + " (" + std::to_string(chosen->year) + ")\n";
                        const std::string directorLine = chosen->director.empty() ? std::string("") : ("🎥 Director: " + chosen->director + "\n");
                        const std::string blankAfterDirector = "\n";
                        const std::string plotOriginal = !chosen->plotShort.empty() ? chosen->plotShort : chosen->plotFull;
                        const std::string rankingPrefix = "⭐ My ranking: ";
                        const std::string rankingValue = (chosen->userRank > 0) ? ("#" + std::to_string(chosen->userRank) + "/100") : std::string("");
                        const std::string rankingLine = rankingPrefix + rankingValue + "\n";
                        const std::string imdbLine = (chosen->imdbRating > 0.0) ? ("⭐ IMDb ranking: " + std::to_string(chosen->imdbRating) + "/10\n") : std::string("");
                        const std::string linkLine = chosen->imdbID.empty() ? std::string("") : ("🔗 https://www.imdb.com/title/" + chosen->imdbID + "/\n");

                        auto utf8_length = [](const std::string& s) -> size_t {
                            size_t count = 0;
                            for (unsigned char c : s) {
                                if ((c & 0xC0) != 0x80) ++count;
                            }
                            return count;
                        };
                        auto utf8_truncate = [](const std::string& s, size_t maxCodepoints) -> std::string {
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
                        };

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

                        std::string bodyFull = buildBody(plotOriginal, true);
                        std::string finalBody;
                        if (utf8_length(bodyFull) <= LIMIT) {
                            finalBody = bodyFull;
                        } else {
                            std::string bodyNoFooter = buildBody(plotOriginal, false);
                            if (utf8_length(bodyNoFooter) <= LIMIT) {
                                finalBody = bodyNoFooter;
                            } else {
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
                                size_t budget = (LIMIT > fixedLen) ? (LIMIT - fixedLen) : 0;
                                size_t keep = (budget > ellLen) ? (budget - ellLen) : 0;
                                std::string truncated = utf8_truncate(plotOriginal, keep) + ellipsis;
                                finalBody = buildBody(truncated, true);
                                if (utf8_length(finalBody) > LIMIT && keep > 0) {
                                    size_t low = 0, high = keep;
                                    while (low < high) {
                                        size_t mid = (low + high) / 2;
                                        std::string t = utf8_truncate(plotOriginal, mid) + ellipsis;
                                        std::string b = buildBody(t, true);
                                        if (utf8_length(b) <= LIMIT) { low = mid + 1; finalBody = b; truncated = t; }
                                        else { high = mid; }
                                    }
                                }
                            }
                        }

                        std::optional<std::string> mediaId;
                        if (!chosen->posterUrl.empty() && chosen->posterUrl != "N/A") {
                            auto img = cpr::Get(cpr::Url{chosen->posterUrl});
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
                        bool ok = mastoPostStatus(cfg.mastodonInstance, cfg.mastodonAccessToken, finalBody, mediaId);
                        std::cout << (ok ? "Posted successfully.\n" : "Failed to post.\n");
                    }
                }
            } else {
                // Configure Mastodon
                std::cout << "Enter Mastodon instance base URL (e.g., https://mastodon.social): ";
                std::string inst; std::cin >> inst;
                std::cout << "Enter Mastodon access token: ";
                std::string token; std::cin >> token;
                if (mastoVerify(inst, token)) {
                    cfg.mastodonEnabled = true;
                    cfg.mastodonInstance = inst;
                    cfg.mastodonAccessToken = token;
                    saveConfig(cfg);
                    std::cout << "Mastodon account verified and saved.\n";
                } else {
                    std::cout << "Could not verify Mastodon credentials.\n";
                }
            }
            break;
        case '0':
            // Edit post header/footer
            std::cout << "Enter new post header (empty to clear, leave blank to keep current):\n> ";
            if (std::cin.peek() == '\n') std::cin.get();
            {
                std::string header;
                std::getline(std::cin, header);
                if (!header.empty() || header.size() == 0) {
                    cfg.postHeaderText = header; // allow empty to clear
                }
            }
            std::cout << "Enter new post footer (empty to clear, leave blank to keep current):\n> ";
            {
                std::string footer;
                std::getline(std::cin, footer);
                if (!footer.empty() || footer.size() == 0) {
                    cfg.postFooterText = footer; // allow empty to clear
                }
            }
            saveConfig(cfg);
            std::cout << "Post header/footer updated.\n";
            break;
        default:
            break;
        }
        
    } while (input != 'q');
    
    return 0;
}