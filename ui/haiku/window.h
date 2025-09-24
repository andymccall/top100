// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/haiku/window.h
// Purpose: Haiku (BeAPI) main window declaration.
//-------------------------------------------------------------------------------
#pragma once

#ifdef __HAIKU__
#include <Window.h>
#include <Message.h>

#include <string>
#include <vector>
#include <optional>

class BView;
class BListView;
class BStringView;
class BTextView;
class BScrollView;
class BMenuBar;
class BMenuField;
class BPopUpMenu;
class BButton;
class BSplitView;
class CompareDialog;

class Top100HaikuWindow : public BWindow {
public:
    Top100HaikuWindow();
    bool QuitRequested() override;
    void MessageReceived(BMessage* msg) override;

private:
    // Messages
    enum {
        kMsgListSelection = 'lsel',
        kMsgSortChanged   = 'srtC',
        kMsgOpenImdb      = 'oimb',
        kMsgDoAdd         = 'addM',
        kMsgDoDelete      = 'delM',
        kMsgDoRefresh     = 'rfrs',
        kMsgDoUpdate      = 'updm',
        kMsgDoRank        = 'rank'
    };

    // Message from Add dialog when user confirms
    static constexpr uint32 kMsgAddByImdbOk = 'adbk';

    // Layout
    BMenuBar*    menubar_      = nullptr;
    BSplitView*  split_        = nullptr;
    // Left
    BView*       leftBox_      = nullptr;
    BStringView* moviesHdr_    = nullptr;
    BMenuField*  sortField_    = nullptr;
    BPopUpMenu*  sortMenu_     = nullptr;
    BListView*   listView_     = nullptr;
    BScrollView* listScroll_   = nullptr;
    // Right
    BView*       rightBox_     = nullptr;
    BStringView* detailsHdr_   = nullptr;
    BStringView* titleLabel_   = nullptr;
    BStringView* directorLabel_= nullptr;
    BStringView* directorValue_= nullptr;
    BStringView* actorsLabel_  = nullptr;
    BStringView* actorsValue_  = nullptr;
    BStringView* genresLabel_  = nullptr;
    BStringView* genresValue_  = nullptr;
    BStringView* runtimeLabel_ = nullptr;
    BStringView* runtimeValue_ = nullptr;
    BStringView* imdbLabel_    = nullptr;
    BButton*     imdbButton_   = nullptr;
    // Top action buttons (simple toolbar row)
    BButton*     addBtn_       = nullptr;
    BButton*     delBtn_       = nullptr;
    BButton*     refreshBtn_   = nullptr;
    BButton*     updateBtn_    = nullptr;
    BView*       posterView_   = nullptr;
    BTextView*   plotText_     = nullptr;
    BScrollView* plotScroll_   = nullptr;
    // Footer
    BStringView* statusLabel_  = nullptr;

    // Data map: list row -> imdb id
    std::vector<std::string> imdbForRow_;

    // Helpers
    void BuildLayout();
    void BuildMenu();
    void RebuildSortMenu(int currentIndex);
    void ReloadModel(const std::string& selectImdb = std::string());
    void UpdateDetails(int rowIndex);
    void UpdateStatusCount();
    void SetSortOrder(int idx);
    static std::string Join(const std::vector<std::string>& v, const std::string& sep);
};

#endif // __HAIKU__
