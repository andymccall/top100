// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/haiku/window.cpp
// Purpose: Haiku (BeAPI) main window implementation.
//-------------------------------------------------------------------------------
#include "window.h"

#ifdef __HAIKU__
#include <View.h>
#include <StringView.h>
#include <TextView.h>
#include <ScrollView.h>
#include <ListView.h>
#include <ListItem.h>
#include <StringItem.h>
#include <SplitView.h>
#include <MenuBar.h>
#include <Menu.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <Button.h>
#include <Url.h>
#include <Alert.h>
#include <Roster.h>
#include <sstream>
#include <optional>
#include <cmath>
#include <deque>
#include <TranslationUtils.h>
#include <cpr/cpr.h>

#include "../../lib/config.h"
#include "../../lib/top100.h"
#include "../../lib/omdb.h"
#include "add_dialog.h"
#include "../common/constants.h"
#include "../common/strings.h"

using namespace ui_constants;
using namespace ui_strings;

namespace {
float fromGrid(float v) { return v; }
}

Top100HaikuWindow::Top100HaikuWindow()
    : BWindow(BRect(100, 100, 100 + kInitialWidth * 1.2, 100 + kInitialHeight * 1.2),
              "Top100 — Haiku UI", B_TITLED_WINDOW, B_QUIT_ON_WINDOW_CLOSE)
{
    BuildLayout();
    BuildMenu();
    ReloadModel();
}

bool Top100HaikuWindow::QuitRequested() {
    be_app->PostMessage(B_QUIT_REQUESTED);
    return true;
}

void Top100HaikuWindow::MessageReceived(BMessage* msg) {
    switch (msg->what) {
        case kMsgDoAdd: {
            AddByImdbWindow* dlg = new AddByImdbWindow(BMessenger(this));
            dlg->Show();
            break;
        }
        case kMsgListSelection: {
            int32 index = -1;
            if (msg->FindInt32("index", &index) == B_OK && index >= 0) {
                UpdateDetails(index);
            }
            break;
        }
        case kMsgAddByImdbOk: {
            const char* id = nullptr;
            if (msg->FindString("imdbID", &id) != B_OK || !id) break;
            std::string imdbID = id;
            AppConfig cfg;
            try { cfg = loadConfig(); } catch (const std::exception& e) { (new BAlert("cfg", e.what(), "OK"))->Go(); break; }
            if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) {
                (new BAlert("omdb", "OMDb is not configured. Set omdbEnabled=true and an API key in your config.", "OK"))->Go();
                break;
            }
            try {
                auto om = omdbGetById(cfg.omdbApiKey, imdbID);
                if (!om) { (new BAlert("omdb2", "Could not fetch details from OMDb.", "OK"))->Go(); break; }
                Top100 list(cfg.dataFile);
                list.addMovie(*om);
                list.recomputeRanks();
                // Persist via destructor save; or call private save via scope end
            } catch (const std::exception& e) {
                (new BAlert("err", e.what(), "OK"))->Go();
            }
            ReloadModel(imdbID);
            break;
        }
        case kMsgSortChanged: {
            int32 sidx = 0;
            if (msg->FindInt32("sort", &sidx) == B_OK) {
                SetSortOrder((int)sidx);
            }
            break;
        }
        case kMsgDoRefresh: {
            if (!listView_) { break; }
            int row = listView_->CurrentSelection();
            std::string selId = (row >= 0 && (size_t)row < imdbForRow_.size()) ? imdbForRow_[row] : std::string();
            ReloadModel(selId);
            break;
        }
        case kMsgDoDelete: {
            if (!listView_) { break; }
            int row = listView_->CurrentSelection();
            if (row < 0 || (size_t)row >= imdbForRow_.size()) break;
            BAlert* confirm = new BAlert("delete", "Remove the selected movie?", "Cancel", "Remove", nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
            int32 choice = confirm->Go();
            if (choice != 1) break; // 0=Cancel, 1=Remove
            try {
                AppConfig cfg = loadConfig();
                Top100 list(cfg.dataFile);
                bool ok = list.removeByImdbId(imdbForRow_[row]);
                if (!ok) {
                    (new BAlert("notfound", "Could not find this movie in the data file.", "OK"))->Go();
                }
            } catch (const std::exception& e) {
                (new BAlert("error", e.what(), "OK"))->Go();
            }
            ReloadModel();
            break;
        }
        case kMsgDoUpdate: {
            if (!listView_) { break; }
            int row = listView_->CurrentSelection();
            if (row < 0 || (size_t)row >= imdbForRow_.size()) break;
            AppConfig cfg;
            try { cfg = loadConfig(); } catch (const std::exception& e) { (new BAlert("cfg", e.what(), "OK"))->Go(); break; }
            if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) {
                (new BAlert("omdb", "OMDb is not configured. Set omdbEnabled=true and an API key in your config.", "OK"))->Go();
                break;
            }
            try {
                auto om = omdbGetById(cfg.omdbApiKey, imdbForRow_[row]);
                if (!om) {
                    (new BAlert("omdb2", "Could not fetch details from OMDb.", "OK"))->Go();
                    break;
                }
                Top100 list(cfg.dataFile);
                if (!list.mergeFromOmdbByImdbId(*om)) {
                    (new BAlert("merge", "Update failed: movie not found in your list.", "OK"))->Go();
                }
                ReloadModel(imdbForRow_[row]);
            } catch (const std::exception& e) {
                (new BAlert("err", e.what(), "OK"))->Go();
            }
            break;
        }
    case kMsgDoRank: {
            // Open compare (ranking) dialog
            class CompareDialog : public BWindow {
            public:
                CompareDialog(BWindow* parent) : BWindow(BRect(0,0,900,600), "Rank movies", B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE), parentMsgr_(parent) {
                    SetFeel(B_MODAL_SUBSET_WINDOW_FEEL);
                    if (parent) AddToSubset(parent);
                    if (parent) {
                        // Center on parent
                        BRect p = parent->Frame();
                        MoveTo(p.left + (p.Width() - Frame().Width())/2, p.top + (p.Height() - Frame().Height())/2);
                    }
                    BuildUI();
                }
            private:
                class PosterView : public BView {
                public:
                    PosterView(BRect frame, const char* name) : BView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW) {
                        SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
                    }
                    ~PosterView() override { if (fBitmap) delete fBitmap; }
                    void SetBitmap(BBitmap* bm) { if (fBitmap) delete fBitmap; fBitmap = bm; Invalidate(); }
                    void Draw(BRect) override {
                        if (!fBitmap) return;
                        BRect bounds = Bounds();
                        float maxW = bounds.Width() * ui_constants::kPosterMaxWidthRatio;
                        float maxH = bounds.Height() * ui_constants::kPosterMaxHeightRatio;
                        BRect src = fBitmap->Bounds();
                        float sw = src.Width(); float sh = src.Height();
                        if (sw <= 0 || sh <= 0) return;
                        float sx = maxW / sw, sy = maxH / sh; float s = std::min(1.0f, std::min(sx, sy));
                        float tw = std::max(1.0f, sw * s), th = std::max(1.0f, sh * s);
                        BRect dst(0, 0, tw, th);
                        // center within view
                        dst.OffsetTo((bounds.Width() - tw)/2, (bounds.Height() - th)/2);
                        DrawBitmap(fBitmap, src, dst, B_FILTER_BITMAP_BILINEAR);
                    }
                private:
                    BBitmap* fBitmap { nullptr };
                };
                // UI elements
                BView* root { nullptr };
                BStringView* heading { nullptr };
                BStringView* prompt { nullptr };
                BView* leftPane { nullptr };
                BStringView* leftTitle { nullptr };
                BTextView* leftDetails { nullptr };
                PosterView* leftPoster { nullptr };
                BView* rightPane { nullptr };
                BStringView* rightTitle { nullptr };
                BTextView* rightDetails { nullptr };
                PosterView* rightPoster { nullptr };
                BButton* passBtn { nullptr };
                BButton* finishBtn { nullptr };
                int32 leftIdx { -1 };
                int32 rightIdx { -1 };
                int32 prevA { -1 }, prevB { -1 };
                std::deque<std::pair<int32,int32>> recentPairs; // history of recent pairs to avoid repeats
                static constexpr size_t kMaxRecentPairs = 20;
                BMessenger parentMsgr_;

                static std::string Join(const std::vector<std::string>& v, const char* sep = ", ") {
                    std::ostringstream oss; bool first = true; for (const auto& s : v) { if (!first) oss << sep; oss << s; first = false; } return oss.str();
                }
                void BuildUI() {
                    root = new BView(Bounds(), "root", B_FOLLOW_ALL, B_WILL_DRAW);
                    root->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
                    AddChild(root);
                    float x = 12, y = 12;
                    heading = new BStringView(BRect(x, y, Bounds().Width()-x, y+24), "hdr", "Rank movies"); y += 28;
                    prompt = new BStringView(BRect(x, y, Bounds().Width()-x, y+20), "prm", "Which movie do you prefer?"); y += 24;
                    root->AddChild(heading);
                    root->AddChild(prompt);
                    float midX = Bounds().Width()/2;
                    // Left
                    leftPane = new BView(BRect(x, y, midX-6, Bounds().Height()-56), "left", B_FOLLOW_ALL, B_WILL_DRAW);
                    leftPane->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
                    leftTitle = new BStringView(BRect(8, 8, leftPane->Bounds().Width()-8, 28), "lt", "");
                    leftDetails = new BTextView(BRect(8, 36, leftPane->Bounds().Width()-8, 36 + 120), "ld");
                    leftDetails->MakeEditable(false);
                    leftDetails->SetWordWrap(true);
                    leftPoster = new PosterView(BRect(8, 36 + 124, leftPane->Bounds().Width()-8, leftPane->Bounds().Height()-8), "lposter");
                    leftPane->AddChild(leftTitle);
                    leftPane->AddChild(leftDetails);
                    leftPane->AddChild(leftPoster);
                    root->AddChild(leftPane);
                    // Right
                    rightPane = new BView(BRect(midX+6, y, Bounds().Width()-x, Bounds().Height()-56), "right", B_FOLLOW_ALL, B_WILL_DRAW);
                    rightPane->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
                    rightTitle = new BStringView(BRect(8, 8, rightPane->Bounds().Width()-8, 28), "rt", "");
                    rightDetails = new BTextView(BRect(8, 36, rightPane->Bounds().Width()-8, 36 + 120), "rd");
                    rightDetails->MakeEditable(false);
                    rightDetails->SetWordWrap(true);
                    rightPoster = new PosterView(BRect(8, 36 + 124, rightPane->Bounds().Width()-8, rightPane->Bounds().Height()-8), "rposter");
                    rightPane->AddChild(rightTitle);
                    rightPane->AddChild(rightDetails);
                    rightPane->AddChild(rightPoster);
                    root->AddChild(rightPane);
                    // Bottom
                    passBtn = new BButton(BRect(Bounds().Width()-220, Bounds().Height()-40, Bounds().Width()-130, Bounds().Height()-16), "pass", "Pass", new BMessage('pass'));
                    finishBtn = new BButton(BRect(Bounds().Width()-120, Bounds().Height()-40, Bounds().Width()-12, Bounds().Height()-16), "done", "Finish Ranking", new BMessage(B_QUIT_REQUESTED));
                    root->AddChild(passBtn);
                    root->AddChild(finishBtn);
                    // Click handlers: mouse down selects that side
                    leftPane->SetEventMask(B_POINTER_EVENTS);
                    rightPane->SetEventMask(B_POINTER_EVENTS);
                    // Pointer events for hover + clicks
                    PickTwo();
                }

                void PickTwo() {
                    AppConfig cfg = loadConfig(); Top100 list(cfg.dataFile);
                    auto movies = list.getMovies(SortOrder::DEFAULT);
                    int n = (int)movies.size(); if (n < 2) { leftIdx = rightIdx = -1; return; }
                    int attempts = 0;
                    auto isRepeat = [this](int a, int b){
                        for (const auto& pr : recentPairs) {
                            if ((pr.first == a && pr.second == b) || (pr.first == b && pr.second == a)) return true;
                        }
                        return false;
                    };
                    int a = rand() % n; int b = rand() % n;
                    while ((b == a) || isRepeat(a,b)) {
                        b = rand() % n; a = rand() % n; if (++attempts > 50) break;
                    }
                    if (recentPairs.size() >= kMaxRecentPairs) recentPairs.pop_front();
                    recentPairs.emplace_back(a,b);
                    prevA = a; prevB = b; leftIdx = a; rightIdx = b; Refresh(true); Refresh(false);
                }
                void Refresh(bool left) {
                    AppConfig cfg = loadConfig(); Top100 list(cfg.dataFile);
                    auto movies = list.getMovies(SortOrder::DEFAULT);
                    int idx = left ? leftIdx : rightIdx; if (idx < 0 || idx >= (int)movies.size()) return;
                    const Movie& mv = movies[(size_t)idx];
                    std::string head = mv.title + " (" + std::to_string(mv.year) + ")";
                    std::string det = std::string("Director: ") + mv.director + "\nActors: " + Join(mv.actors, ", ") + "\nGenres: " + Join(mv.genres, ", ") + "\nRuntime: " + (mv.runtimeMinutes > 0 ? std::to_string(mv.runtimeMinutes) + " min" : "");
                    if (left) { leftTitle->SetText(head.c_str()); leftDetails->SetText(det.c_str()); }
                    else      { rightTitle->SetText(head.c_str()); rightDetails->SetText(det.c_str()); }
                    // Load posters asynchronously
                    if (!mv.posterUrl.empty()) LoadPoster(left, mv.posterUrl);
                }
                void Choose(bool left) {
                    if (leftIdx < 0 || rightIdx < 0) return;
                    AppConfig cfg = loadConfig(); Top100 list(cfg.dataFile);
                    auto movies = list.getMovies(SortOrder::DEFAULT);
                    if (leftIdx >= (int)movies.size() || rightIdx >= (int)movies.size()) return;
                    Movie L = movies[leftIdx], R = movies[rightIdx];
                    auto elo = [](double &a, double &b, double scoreA, double k=32.0){ double qa = pow(10.0, a/400.0), qb = pow(10.0, b/400.0); double ea = qa/(qa+qb), eb = qb/(qa+qb); a = a + k * (scoreA - ea); b = b + k * ((1.0 - scoreA) - eb); };
                    elo(L.userScore, R.userScore, left ? 1.0 : 0.0);
                    int li = list.findIndexByImdbId(L.imdbID); if (li >= 0) list.updateMovie((size_t)li, L);
                    int ri = list.findIndexByImdbId(R.imdbID); if (ri >= 0) list.updateMovie((size_t)ri, R);
                    list.recomputeRanks();
                    PickTwo();
                }

                void LoadPoster(bool leftSide, const std::string& url) {
                    // Capture messenger to post back to this window safely
                    BMessenger msgr(this);
                    std::thread([this, msgr, url, leftSide]() {
                        auto resp = cpr::Get(cpr::Url{url}, cpr::Timeout{8000});
                        if (resp.error || resp.status_code != 200 || resp.text.empty()) return;
                        // Decode via Translation Kit
                        BBitmap* bitmap = BTranslationUtils::GetBitmapFromBuffer(resp.text.data(), resp.text.size());
                        if (!bitmap) return;
                        // Apply on UI thread
                        BMessage m('pstr');
                        m.AddPointer("bm", bitmap);
                        m.AddBool("left", leftSide);
                        msgr.SendMessage(&m);
                    }).detach();
                }

                void MessageReceived(BMessage* msg) override {
                    switch (msg->what) {
                        case 'pass': PickTwo(); break;
                        case 'pstr': {
                            BBitmap* bm = nullptr; bool leftSide = true;
                            msg->FindPointer("bm", reinterpret_cast<void**>(&bm));
                            msg->FindBool("left", &leftSide);
                            if (bm) {
                                if (leftSide && leftPoster) leftPoster->SetBitmap(bm);
                                else if (rightPoster) rightPoster->SetBitmap(bm);
                            }
                            break;
                        }
                        default: BWindow::MessageReceived(msg);
                    }
                }
                bool QuitRequested() override {
                    // Ask parent to refresh list to reflect updated ranks
                    if (parentMsgr_.IsValid()) {
                        BMessage refresh('rfrs'); // matches kMsgDoRefresh
                        parentMsgr_.SendMessage(&refresh);
                    }
                    return BWindow::QuitRequested();
                }
                void MouseDown(BPoint pt) override {
                    BRect l = leftPane->Frame(); BRect r = rightPane->Frame();
                    if (l.Contains(pt)) { Choose(true); return; }
                    if (r.Contains(pt)) { Choose(false); return; }
                    BWindow::MouseDown(pt);
                }
                void MouseMoved(BPoint pt, uint32 transit, const BMessage* message) override {
                    BRect l = leftPane->Frame(); BRect r = rightPane->Frame();
                    if (l.Contains(pt) || r.Contains(pt)) be_app->SetCursor(B_HAND_CURSOR);
                    else be_app->SetCursor(B_CURSOR_SYSTEM_DEFAULT);
                    BWindow::MouseMoved(pt, transit, message);
                }
                void KeyDown(const char* bytes, int32 numBytes) override {
                    if (numBytes > 0) {
                        switch (bytes[0]) {
                            case B_LEFT_ARROW: Choose(true); return;
                            case B_RIGHT_ARROW: Choose(false); return;
                            case B_DOWN_ARROW:
                            case B_ENTER:
                            case B_RETURN: PickTwo(); return;
                        }
                    }
                    BWindow::KeyDown(bytes, numBytes);
                }
            };
            (new CompareDialog(this))->Show();
            break;
        }
        case kMsgOpenImdb: {
            if (!listView_) break;
            int row = listView_->CurrentSelection();
            if (row < 0 || (size_t)row >= imdbForRow_.size()) break;
            std::string url = std::string("https://www.imdb.com/title/") + imdbForRow_[row] + "/";
            BUrl burl(url.c_str());
            if (!burl.IsValid()) break;
            // On Haiku, launching URLs typically goes through B_OPEN_IN_BROWSER mime
            const char* arg = url.c_str();
            be_roster->Launch("application/x-vnd.Be.URL.http", 1, const_cast<char**>(&arg));
            break;
        }
        default:
            BWindow::MessageReceived(msg);
    }
}

void Top100HaikuWindow::BuildMenu() {
    menubar_ = new BMenuBar(BRect(0, 0, Bounds().Width(), 18), "menubar");
    AddChild(menubar_);
    auto *file = new BMenu("File");
    file->AddItem(new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q'));
    menubar_->AddItem(file);

    // List actions
    auto* listMenu = new BMenu("List");
    listMenu->AddItem(new BMenuItem("Add movie…", new BMessage(kMsgDoAdd), 'A'));
    listMenu->AddItem(new BMenuItem("Delete", new BMessage(kMsgDoDelete), 'D'));
    listMenu->AddSeparatorItem();
    listMenu->AddItem(new BMenuItem("Refresh", new BMessage(kMsgDoRefresh), 'R'));
    listMenu->AddItem(new BMenuItem("Update from OMDb", new BMessage(kMsgDoUpdate), 'U'));
    listMenu->AddSeparatorItem();
    listMenu->AddItem(new BMenuItem("Rank…", new BMessage(kMsgDoRank), 'K'));
    menubar_->AddItem(listMenu);
}

void Top100HaikuWindow::BuildLayout() {
    // Root area below menubar
    float top = 20;
    split_ = new BSplitView(B_HORIZONTAL);
    split_->SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT));
    auto *root = new BView(BRect(0, top, Bounds().Width(), Bounds().Height()), "root", B_FOLLOW_ALL_SIDES, 0);
    root->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    AddChild(root);

    // Left box
    leftBox_ = new BView(BRect(0, 0, Bounds().Width() * 0.45, Bounds().Height()), "left", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
    leftBox_->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    moviesHdr_ = new BStringView(BRect(8, 8, 300, 28), "moviesHdr", kHeadingMovies);
    sortMenu_ = new BPopUpMenu("Sort");
    sortField_ = new BMenuField(BRect(8, 36, 320, 56), "sort", kLabelSortOrder, sortMenu_);
    listView_ = new BListView(BRect(8, 64, leftBox_->Bounds().Width() - 8, leftBox_->Bounds().Height() - 8), "list");
    listView_->SetSelectionMessage(new BMessage(kMsgListSelection));
    listView_->SetTarget(this);
    listScroll_ = new BScrollView("listScroll", listView_, B_FOLLOW_ALL, 0, false, true);
    leftBox_->AddChild(moviesHdr_);
    leftBox_->AddChild(sortField_);
    leftBox_->AddChild(listScroll_);

    // Right box
    rightBox_ = new BView(BRect(0, 0, Bounds().Width() * 0.55, Bounds().Height()), "right", B_FOLLOW_ALL_SIDES, B_WILL_DRAW);
    rightBox_->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    detailsHdr_ = new BStringView(BRect(8, 8, 300, 28), "detailsHdr", kHeadingDetails);
    titleLabel_ = new BStringView(BRect(8, 36, rightBox_->Bounds().Width() - 8, 56), "title", "");
    float y = 64;
    directorLabel_ = new BStringView(BRect(8, y, 8 + kLabelMinWidth, y + 18), "dirLbl", kFieldDirector); directorValue_ = new BStringView(BRect(16 + kLabelMinWidth, y, rightBox_->Bounds().Width()-8, y + 18), "dirVal", ""); y += 20;
    actorsLabel_   = new BStringView(BRect(8, y, 8 + kLabelMinWidth, y + 18), "actLbl", kFieldActors);   actorsValue_   = new BStringView(BRect(16 + kLabelMinWidth, y, rightBox_->Bounds().Width()-8, y + 18), "actVal", ""); y += 20;
    genresLabel_   = new BStringView(BRect(8, y, 8 + kLabelMinWidth, y + 18), "genLbl", kFieldGenres);   genresValue_   = new BStringView(BRect(16 + kLabelMinWidth, y, rightBox_->Bounds().Width()-8, y + 18), "genVal", ""); y += 20;
    runtimeLabel_  = new BStringView(BRect(8, y, 8 + kLabelMinWidth, y + 18), "runLbl", kFieldRuntime);  runtimeValue_  = new BStringView(BRect(16 + kLabelMinWidth, y, rightBox_->Bounds().Width()-8, y + 18), "runVal", ""); y += 20;
    imdbLabel_     = new BStringView(BRect(8, y, 8 + kLabelMinWidth, y + 18), "imdLbl", kFieldImdbPage);
    imdbButton_    = new BButton(BRect(16 + kLabelMinWidth, y-2, 140 + kLabelMinWidth, y + 18), "imdbBtn", "Open", new BMessage(kMsgOpenImdb)); y += 28;

    posterView_ = new BView(BRect(8, y, rightBox_->Bounds().Width() - 8, y + (Bounds().Height() * 0.60)), "poster", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW);
    posterView_->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    y = posterView_->Frame().bottom + 8;

    auto *plotHdr = new BStringView(BRect(8, y, 200, y + 18), "plotHdr", kGroupPlot); y += 20;
    plotText_ = new BTextView(BRect(8, y, rightBox_->Bounds().Width() - 8, y + (Bounds().Height() * 0.20)), "plot");
    plotText_->MakeEditable(false);
    plotText_->SetWordWrap(true);
    plotScroll_ = new BScrollView("plotScroll", plotText_, B_FOLLOW_ALL, 0, false, true);

    rightBox_->AddChild(detailsHdr_);
    rightBox_->AddChild(titleLabel_);
    rightBox_->AddChild(directorLabel_);
    rightBox_->AddChild(directorValue_);
    rightBox_->AddChild(actorsLabel_);
    rightBox_->AddChild(actorsValue_);
    rightBox_->AddChild(genresLabel_);
    rightBox_->AddChild(genresValue_);
    rightBox_->AddChild(runtimeLabel_);
    rightBox_->AddChild(runtimeValue_);
    rightBox_->AddChild(imdbLabel_);
    rightBox_->AddChild(imdbButton_);
    rightBox_->AddChild(posterView_);
    rightBox_->AddChild(plotHdr);
    rightBox_->AddChild(plotScroll_);

    // Split view assembly
    split_->AddChild(leftBox_);
    split_->AddChild(rightBox_);
    root->AddChild(split_);

    // Footer status (simple label at bottom-left)
    statusLabel_ = new BStringView(BRect(8, Bounds().Height() - 18, 200, Bounds().Height() - 4), "status", "");
    AddChild(statusLabel_);

    // Sort menu entries to saved value
    int savedSort = 0;
    try { savedSort = loadConfig().uiSortOrder; } catch (...) {}
    RebuildSortMenu(savedSort);
}

void Top100HaikuWindow::RebuildSortMenu(int currentIndex) {
    if (!sortMenu_ || !sortField_) return;
    sortMenu_->RemoveItems(0, sortMenu_->CountItems(), true);
    const char* opts[] = { kSortInsertion, kSortByYear, kSortAlpha, kSortByRank, kSortByScore };
    for (int i = 0; i < 5; ++i) {
        auto *mi = new BMenuItem(opts[i], new BMessage(kMsgSortChanged));
        mi->Message()->AddInt32("sort", i);
        sortMenu_->AddItem(mi);
        if (i == currentIndex) mi->SetMarked(true);
    }
}

void Top100HaikuWindow::SetSortOrder(int idx) {
    try {
        AppConfig cfg = loadConfig();
        cfg.uiSortOrder = idx;
        saveConfig(cfg);
    } catch (...) { /* ignore */ }
    ReloadModel();
}

void Top100HaikuWindow::ReloadModel(const std::string& selectImdb) {
    if (!listView_) return;
    listView_->MakeEmpty();
    imdbForRow_.clear();
    AppConfig cfg;
    try { cfg = loadConfig(); } catch (...) { return; }
    Top100 list(cfg.dataFile);
    SortOrder order = SortOrder::DEFAULT;
    switch (cfg.uiSortOrder) {
        case 1: order = SortOrder::BY_YEAR; break;
        case 2: order = SortOrder::ALPHABETICAL; break;
        case 3: order = SortOrder::BY_USER_RANK; break;
        case 4: order = SortOrder::BY_USER_SCORE; break;
        default: order = SortOrder::DEFAULT; break;
    }
    auto movies = list.getMovies(order);
    int selectIndex = -1;
    int row = 0;
    for (const auto& m : movies) {
        auto text = (m.userRank > 0 ? ("#" + std::to_string(m.userRank) + " ") : std::string()) + m.title + " (" + std::to_string(m.year) + ")";
        listView_->AddItem(new BStringItem(text.c_str()));
        imdbForRow_.push_back(m.imdbID);
        if (!selectImdb.empty() && m.imdbID == selectImdb) selectIndex = row;
        ++row;
    }
    if (selectIndex >= 0) listView_->Select(selectIndex);
    else if (listView_->CountItems() > 0) listView_->Select(0);
    UpdateStatusCount();
    if (listView_->CurrentSelection() >= 0) UpdateDetails(listView_->CurrentSelection());
}

void Top100HaikuWindow::UpdateDetails(int rowIndex) {
    if (rowIndex < 0 || (size_t)rowIndex >= imdbForRow_.size()) return;
    AppConfig cfg = loadConfig();
    Top100 list(cfg.dataFile);
    int index = list.findIndexByImdbId(imdbForRow_[rowIndex]);
    if (index < 0) return;
    auto movies = list.getMovies(SortOrder::DEFAULT);
    const Movie& mv = movies.at((size_t)index);
    // Title
    std::string title = mv.title + " (" + std::to_string(mv.year) + ")";
    titleLabel_->SetText(title.c_str());
    directorValue_->SetText(mv.director.c_str());
    actorsValue_->SetText(Join(mv.actors, ", ").c_str());
    genresValue_->SetText(Join(mv.genres, ", ").c_str());
    runtimeValue_->SetText(mv.runtimeMinutes > 0 ? (std::to_string(mv.runtimeMinutes) + " min").c_str() : "");
    // IMDb button enabled/disabled
    imdbButton_->SetEnabled(!mv.imdbID.empty());
    // Plot
    plotText_->SetText((mv.plotFull.empty() ? mv.plotShort : mv.plotFull).c_str());
}

void Top100HaikuWindow::UpdateStatusCount() {
    if (!statusLabel_) return;
    int n = listView_ ? listView_->CountItems() : 0;
    std::string s = std::to_string(n) + (n == 1 ? " movie" : " movies");
    statusLabel_->SetText(s.c_str());
}

std::string Top100HaikuWindow::Join(const std::vector<std::string>& v, const std::string& sep) {
    std::ostringstream oss; bool first = true;
    for (const auto& s : v) { if (!first) oss << sep; oss << s; first = false; }
    return oss.str();
}

#endif // __HAIKU__
