// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/haiku/add_dialog.cpp
// Purpose: Implementation of AddByImdbWindow prompt.
//-------------------------------------------------------------------------------
#include "add_dialog.h"

#ifdef __HAIKU__
#include <View.h>
#include <StringView.h>
#include <LayoutBuilder.h>

AddByImdbWindow::AddByImdbWindow(BMessenger target)
    : BWindow(BRect(200, 200, 600, 300), "Enter IMDb ID", B_TITLED_WINDOW, B_MODAL_APP_WINDOW_ALERT)
    , target_(target)
{
    auto* root = new BView(Bounds(), "root", B_FOLLOW_ALL, B_WILL_DRAW);
    root->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
    AddChild(root);

    imdbField_ = new BTextControl(BRect(10, 10, Bounds().Width() - 10, 40), "imdb", "IMDb ID:", "tt", nullptr);
    okBtn_ = new BButton(BRect(10, 60, 120, 90), "ok", "OK", new BMessage(kMsgOk));
    cancelBtn_ = new BButton(BRect(130, 60, 240, 90), "cancel", "Cancel", new BMessage(kMsgCancel));

    root->AddChild(imdbField_);
    root->AddChild(okBtn_);
    root->AddChild(cancelBtn_);
}

bool AddByImdbWindow::QuitRequested() {
    Hide();
    return true;
}

void AddByImdbWindow::MessageReceived(BMessage* msg) {
    switch (msg->what) {
        case kMsgOk: {
            BMessage out(kMsgAddByImdbOk);
            out.AddString("imdbID", imdbField_->Text());
            target_.SendMessage(&out);
            Quit();
            break;
        }
        case kMsgCancel:
            Quit();
            break;
        default:
            BWindow::MessageReceived(msg);
    }
}

#endif // __HAIKU__
