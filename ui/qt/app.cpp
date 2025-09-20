// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/app.cpp
// Purpose: Qt UI application entry.
//-------------------------------------------------------------------------------
#include "app.h"
#include "window.h"

#include <QApplication>

int run_top100_qt_app(int argc, char* argv[]) {
    QApplication app(argc, argv);
    Top100QtWindow win;
    win.show();
    return app.exec();
}

int main(int argc, char* argv[]) { return run_top100_qt_app(argc, argv); }
