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
