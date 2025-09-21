// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/haiku/add_dialog.h
// Purpose: Simple modal dialog to enter an IMDb ID and send back to parent.
//-------------------------------------------------------------------------------
#pragma once

#ifdef __HAIKU__
#include <Window.h>
#include <TextControl.h>
#include <Button.h>

// Message sent to the parent window when OK is pressed.
constexpr uint32 kMsgAddByImdbOk = 'adbk';

class AddByImdbWindow : public BWindow {
public:
    explicit AddByImdbWindow(BMessenger target);
    void MessageReceived(BMessage* msg) override;
    bool QuitRequested() override;

private:
    enum { kMsgOk = 'ok__', kMsgCancel = 'cncl' };
    BTextControl* imdbField_ = nullptr;
    BButton* okBtn_ = nullptr;
    BButton* cancelBtn_ = nullptr;
    BMessenger target_;
};

#endif // __HAIKU__
