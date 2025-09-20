// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/qt/window.h
// Purpose: Declaration of the Qt Widgets main window and its components.
//-------------------------------------------------------------------------------
#pragma once

#include <QMainWindow>
#include <QString>

class QListView;
class QComboBox;
class QLabel;
class QTextBrowser;
class QNetworkAccessManager;
class QToolBar;
class QAction;
class QWidget;

class Top100ListModel;
class QObject;

class Top100QtWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit Top100QtWindow(QWidget* parent = nullptr);

private:
    // Layout widgets
    QWidget* central_ = nullptr;
    QListView* listView_ = nullptr;
    Top100ListModel* model_ = nullptr;
    QComboBox* sortCombo_ = nullptr;
    QLabel* titleLabel_ = nullptr;
    QLabel* directorValue_ = nullptr;
    QLabel* actorsValue_ = nullptr;
    QLabel* genresValue_ = nullptr;
    QLabel* runtimeValue_ = nullptr;
    QLabel* imdbLink_ = nullptr;
    QLabel* posterLabel_ = nullptr;
    QWidget* detailsContainer_ = nullptr; // for width-based elide
    QWidget* posterContainer_ = nullptr;  // for poster scaling
    QTextBrowser* plotView_ = nullptr;
    QToolBar* toolbar_ = nullptr;

    // Actions
    QAction* addAct_ = nullptr;
    QAction* delAct_ = nullptr;
    QAction* refreshTbAct_ = nullptr;
    QAction* postBskyAct_ = nullptr;
    QAction* postMastoAct_ = nullptr;
    QAction* updateAct_ = nullptr;

    // Helpers
    QNetworkAccessManager* nam_ = nullptr;
    QObject* posterResizer_ = nullptr;
    QObject* detailsResizer_ = nullptr;

    // Construction helpers
    void buildLayout();
    void buildMenuBar();
    void buildToolbar();
    void connectSignals();

    // Handlers (implemented in handlers.cpp)
    void onAddMovie();
    void onDeleteCurrent();
    void onRefresh();
    void onPostBlueSky();
    void onPostMastodon();
    void onUpdateFromOmdb();
    void onSortChanged();
    void updateDetails();

    // Poster (implemented in poster.cpp)
    void fetchPosterByUrl(const QString& url);
    void fetchPosterViaOmdb(const QString& imdb);
};
