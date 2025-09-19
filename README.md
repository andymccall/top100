# 🎬 Top100 — Your Personal Movie List

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.11%2B-brightgreen.svg)](https://cmake.org/)
[![Boost.Test](https://img.shields.io/badge/Boost.Test-Unit%20Tests-orange.svg)](https://www.boost.org/doc/libs/release/libs/test/)
[![nlohmann/json](https://img.shields.io/badge/json-nlohmann%2Fjson-lightgrey.svg)](https://github.com/nlohmann/json)
[![cpr](https://img.shields.io/badge/HTTP-cpr-ff69b4.svg)](https://github.com/libcpr/cpr)

A lightweight C++17 library + CLI to curate a “Top 100” movies list. It persists to JSON, fetches details from OMDb over HTTPS, and lets you rank films via quick pairwise comparisons using an Elo-like system.


## ✨ Features

- 📦 Library-first design (`lib/top100.*`) so a future GUI can reuse it
- 💾 JSON persistence using `nlohmann/json`
- 🔎 OMDb integration (title search and add-by-ID) over HTTPS via `cpr`
- 🧠 Duplicate handling with policy (prompt/overwrite/skip)
- ⭐ Ratings support: IMDb, Metascore, Rotten Tomatoes
- 🏅 Personal ranking: Elo-style `userScore` and 1-based `userRank`
- 📋 CLI to add/remove/list/details/compare
- ✅ Unit tests with Boost.Test
- 🔧 Per-user config at `~/.top100_config.json` (data file path, OMDb enable, API key) with menu that adapts to settings
- 📁 Safe persistence: creates parent directories for your chosen data file automatically
 - 📝 Plot handling: stores both short and full plots (details view uses full; social posts use short)
 - 📣 Social sharing: post a selected movie to BlueSky and Mastodon (uploads poster image when available)
 - 🧩 Post customization: configurable header/footer text included in social posts


## 🧭 Project layout

```
lib/
  Movie.h           # Movie model + JSON (de)serialization
  top100.h/.cpp     # Core list persistence and sorting
  omdb.h/.cpp       # OMDb HTTP integration
  bluesky.h/.cpp    # BlueSky client (session, image upload, create post)
  mastodon.h/.cpp   # Mastodon client (verify, upload media, post status)
cli/
  main.cpp          # CLI entry
  displaymenu.cpp   # Menu
  addmovie.cpp      # Manual add
  removemovie.cpp   # Remove
  listmovies.cpp    # List & sort (incl. rank/score)
  viewdetails.cpp   # Details view
  addfromomdb.cpp   # OMDb search + add
  comparemovies.*   # Pairwise ranking loop (Elo)
  dup_policy.h      # Duplicate handling policy (env-driven)
lib/
  config.h/.cpp     # Per-user config: data path; OMDb; BlueSky; Mastodon; post header/footer
  config_utils.*    # High-level helpers (set data path, configure/disable OMDb)
tests/
  test_*.cpp        # Granular unit tests (incl. ranking)
CMakeLists.txt       # Build and test wiring
top100.json          # Example data file (actual location is configurable)
```


## 🏗️ Build

Prereqs: a C++17 compiler, CMake 3.11+, and OpenSSL dev headers (for HTTPS via `cpr`).

```bash
# Configure & build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# Run tests
cd build && ctest --output-on-failure

# Run the CLI
./build/top100_cli
```

If you use VS Code, this repo includes `.vscode/tasks.json` with handy tasks. In particular, “Clean & Rebuild (UIs + Tests)” will:
- Remove the `build/` folder
- Reconfigure with `-DTOP100_UI_QT=ON -DTOP100_UI_KDE=ON -DTOP100_ENABLE_TESTS=ON`
- Build everything and run the full test suite

You can run it via Terminal > Run Task… (or Ctrl/Cmd+Shift+P → “Run Task”).


## �️ Optional UI frontends

In addition to the CLI, you can build optional desktop UIs:

- Qt (cross‑platform) UI — Widgets‑based, no KDE dependencies
- KDE UI — Kirigami/QML frontend with KDE Frameworks integration

Both are disabled by default. Enable with CMake options:

- `-DTOP100_UI_QT=ON` to build `top100_qt`
- `-DTOP100_UI_KDE=ON` to build `top100_kde`

Dependencies:

- Qt UI: Qt 5 (Widgets) or Qt 6 (Widgets)
- KDE UI: Qt Quick/QML (5 or 6) and Kirigami2 from KDE Frameworks (KF5 or KF6)

Example build (out‑of‑tree):

```bash
# Qt UI only
cmake -S . -B build -DTOP100_UI_QT=ON
cmake --build build --target top100_qt

# KDE UI (Kirigami)
cmake -S . -B build -DTOP100_UI_KDE=ON
cmake --build build --target top100_kde
```

If both are enabled, both executables are built. The UIs link the same core libraries; we’ll progressively wire features without duplicating logic.

Consistent UI strings:
- Both UIs read common display text (app names, window titles, hello text) from `ui/common/strings.h` so wording stays in sync.
- A unit test named `ui_strings_constants` verifies these constants.


## �🖥️ CLI usage

By default, the first run creates `~/.top100_config.json` and stores your data at `~/top100/top100.json` (the folder is created if needed). You can change the data file path from the menu at any time. On startup, ranks are recomputed from scores to keep things consistent.

Common actions:
- Add a movie manually
- Remove a movie
- List movies (by default, by year, alphabetically, by rank, or by score)
- Add from OMDb (search, pick, and add) — shown only when OMDb is enabled
- View details of a selected movie
- Compare two random movies repeatedly to evolve your ranking (press `q` to stop)
 - Post a movie to BlueSky (configure once, then post)
 - Post a movie to Mastodon (configure once, then post)
 - Edit social post header/footer text


## 🌐 OMDb integration

- HTTPS is enabled. Requests use `https://www.omdbapi.com/` via `cpr`.
- OMDb is controlled by your per-user config. Use the CLI to enable/disable and set your API key.
- When enabling OMDb, the CLI verifies your key with a lightweight request and persists it if valid.

Dynamic menu items:
- When OMDb is disabled: you’ll see “Configure OMDb API key” and “Set data file path”.
- When OMDb is enabled: you’ll see “Search and add from OMDb” and “Disable OMDb”.

Search flow (`Add from OMDb`):
1) Enter a title query
2) Pick a result to add
3) The app fetches full details by IMDb ID and saves the movie with `source = "omdb"`

Plot length handling:
- When retrieving details by IMDb ID, both the full plot and the short plot are fetched and stored.
- The Details screen shows the full plot when available; social posts use the short plot (falling back to full if needed).

## 📣 Social posting

You can share a selected movie to BlueSky and Mastodon from the CLI. Posts include your configurable header/footer, the title/year, key credits, your personal ranking line, IMDb’s rating, a link to the IMDb page, and the poster image if available.

BlueSky:
- Configure once with your BlueSky identifier (handle or DID), app password, and service URL (default `https://bsky.social`).
- Posting uploads the poster as a blob and creates a post with the composed text and image.

Mastodon:
- Configure once with your instance base URL (default `https://mastodon.social`) and access token.
- Posting uploads the poster via multipart media, then posts the status with the attached image.

Post customization:
- Header text default: “I’d like to share one of my top 100 #movies!”
- Footer text default: “Posted with Top 100!”
- You can edit both from the CLI (they may be empty to omit).

Character limits (auto-fit behavior):
- BlueSky: 300 characters; Mastodon: typically 500 characters.
- The app automatically fits the post using this rule:
  1) If the full post (including footer) fits, post it as-is.
  2) Otherwise, remove the footer and try again.
  3) If still too long, keep the footer and truncate only the plot, appending “...”, until it fits.
  - Truncation is Unicode-safe (doesn’t split emojis or multibyte characters).

Example (BlueSky, auto-fit under 300 chars):

```
I’d like to share one of my top 100 #movies!

🎬 The Matrix (1999)
🎥 Director: Lana Wachowski, Lilly Wachowski

Plot: A hacker learns reality is a simulation and must fight to free humanity...

⭐ My ranking: 
⭐ IMDb ranking: 8.7/10
🔗 https://www.imdb.com/title/tt0133093/

Posted with Top 100!
```


## 🧩 Duplicate policy

When adding from OMDb, duplicates are detected by `imdbID` (preferred) or `title+year`. Control behavior via environment variable `TOP100_DUPLICATE_POLICY`:

- `prompt` (default) — Ask before overwriting
- `prefer_omdb` or `omdb` — Overwrite automatically
- `prefer_manual` or `manual` — Keep manual entry automatically
- `skip` — Never overwrite

Example:
```bash
export TOP100_DUPLICATE_POLICY=prefer_omdb
./build/top100_cli
```


## 🏅 Ranking (Elo-like)

- Each movie has `userScore` (default 1500.0) and `userRank` (-1 until ranked).
- Use the CLI option “Compare two movies (rank)” to see two random movies repeatedly and choose a winner. The Elo update moves the scores. After each comparison the app recomputes ranks so `1` is the highest score.
- You can list by rank or by score to see your evolving Top 100.

Notes:
- Ties are broken consistently by title when sorting by score.
- Ranking state is persisted in `top100.json`.


## 🧪 Tests

This project uses Boost.Test and registers individual test cases with CTest. Highlights include:
- Core: add/remove/save/load
- Sorting: by year, alphabetical
- Movie JSON: round-trip including ratings and new fields (incl. short/full plot)
- Find/replace helpers
- Ranking: JSON fields, recompute ordering, deterministic Elo update
- Config: default creation, load/save round trip, and high-level utilities (incl. BlueSky/Mastodon and header/footer defaults)
- Menu: dynamic items based on OMDb enabled/disabled, BlueSky, Mastodon, and the header/footer editor

Run with:
```bash
cd build && ctest --output-on-failure
# or only ranking-related
cd build && ctest -R ranking_ -V
```


## ⚙️ Configuration

Top100 keeps per-user settings in `~/.top100_config.json` and shows a menu that adapts to your choices.

Fields:
- `dataFile` — full path to your JSON data file
- `omdbEnabled` — show/hide OMDb features
- `omdbApiKey` — your OMDb API key
- `blueSkyEnabled`, `blueSkyIdentifier`, `blueSkyAppPassword`, `blueSkyService` — BlueSky settings (service default `https://bsky.social`)
- `mastodonEnabled`, `mastodonInstance`, `mastodonAccessToken` — Mastodon settings (instance default `https://mastodon.social`)
- `postHeaderText`, `postFooterText` — customizable text included in social posts

Defaults on first run:
- `dataFile`: `~/top100/top100.json` (its parent directories are created automatically)
- `omdbEnabled`: `false`, `omdbApiKey`: empty
- `blueSkyEnabled`: `false`, `blueSkyService`: `https://bsky.social`
- `mastodonEnabled`: `false`, `mastodonInstance`: `https://mastodon.social`
- `postHeaderText`: “I’d like to share one of my top 100 #movies!”, `postFooterText`: “Posted with Top 100!”

From the CLI you can:
- Configure OMDb API key (verifies the key and enables OMDb if valid)
- Disable OMDb (clears the key and hides OMDb menu options)
- Set data file path (updates the config and ensures directories exist)
- Configure BlueSky and post a movie
- Configure Mastodon and post a movie
- Edit the header/footer text for posts

Environment overrides:
- `TOP100_CONFIG_PATH` — path to an alternate config file (useful for testing or multiple profiles)
- `TOP100_DUPLICATE_POLICY` — duplicate handling policy (see section above)

Schema example:

```json
{
  "dataFile": "/home/you/top100/top100.json",
  "omdbEnabled": false,
  "omdbApiKey": "",
  "blueSkyEnabled": false,
  "blueSkyIdentifier": "",
  "blueSkyAppPassword": "",
  "blueSkyService": "https://bsky.social",
  "postHeaderText": "I’d like to share one of my top 100 #movies!",
  "postFooterText": "Posted with Top 100!",
  "mastodonEnabled": false,
  "mastodonInstance": "https://mastodon.social",
  "mastodonAccessToken": ""
}
```

Precedence notes:
- Exactly one config file is active per run. If `TOP100_CONFIG_PATH` is set, that file is used; otherwise `~/.top100_config.json`.
- No merging across files. Changing `TOP100_CONFIG_PATH` switches the entire profile.

### Using multiple configs with TOP100_CONFIG_PATH

You can keep separate profiles (e.g., home/work/testing) by pointing the app at a different config file.

Examples:

```bash
# One-off run with a custom config
TOP100_CONFIG_PATH=~/top100/dev_config.json ./build/top100_cli

# Use a temp config for testing
TOP100_CONFIG_PATH=$(mktemp) ./build/top100_cli

# Persistent shell session using an alternate profile
export TOP100_CONFIG_PATH=~/top100/work_profile.json
./build/top100_cli
```

Notes:
- If the file doesn’t exist, the app will create it with default values.
- This only changes where settings are stored; your movie data file location is still controlled by the `dataFile` setting inside the chosen config.


## 🐛 Troubleshooting

- Build fails finding OpenSSL: install development headers (e.g., `libssl-dev`) and reconfigure.
- HTTPS issues at runtime: ensure your system CA bundle is present and accessible; `cpr` uses libcurl + OpenSSL.
- Tests not discovered: ensure you configured and built from the project root with CMake and that you run `ctest` from the `build` folder.
- Default data path not writable: use “Set data file path” from the menu to choose a path you own; the app creates missing directories automatically.
- Want to reset settings: delete `~/.top100_config.json` (a fresh default will be created on next run).
- OMDb key rejected: re-run “Configure OMDb API key” and double-check the key; you can also disable OMDb from the menu.
- BlueSky login fails: ensure you’re using an app password and that the service URL matches your instance (`https://bsky.social` by default).
- Mastodon media upload fails: some instances reject very large images; try a smaller poster or ensure your token has write permissions.


## 📄 License

This project is licensed under the Apache License, Version 2.0. See `LICENSE` for details.

Third-party components are used under their respective licenses; see `THIRD-PARTY-NOTICES.md` for an overview and links, and `NOTICE` for attributions.

## 📚 Developer docs (Doxygen)

You can generate API documentation with Doxygen; it will be written to the `docs/` folder.

- Prerequisites: Doxygen (and Graphviz for call/caller graphs).
- Run the docs build via the CMake target named `docs`.
- Open `docs/html/index.html` in your browser when done. XML output is also generated at `docs/xml` for possible Sphinx/Breathe integration later.

Online docs:
- Canonical: https://andymccall.co.uk/top100/
- Mirror: https://andymccall.github.io/top100/

Scope and notes:
- The docs focus on public headers in `lib/` plus key CLI entry points in `cli/`.
- Tests are excluded from the generated documentation to keep the API surface clear.

## 🚀 Branching, releases, and docs

- Branch strategy:
  - `main`: stable; always releasable
  - `develop`: integration branch for completed features
  - `feature/*`: short-lived branches for focused changes; merge into `develop`
- Releases:
  - Create a tag like `vX.Y.Z` on `main` to trigger a GitHub Release build.
  - The CI bundles `top100_cli` (Linux) with `README.md` into `top100_cli-linux.tar.gz` attached to the release.
- Docs:
  - On pushes to `main`, CI builds Doxygen and publishes `docs/html` to GitHub Pages.
  - After enabling Pages for this repository (Source: GitHub Actions), the site will auto-update on merges to `main`.