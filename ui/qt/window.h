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
#include "spinner.h"
class QNetworkAccessManager;
class QToolBar;
class QAction;
class QWidget;

class Top100ListModel;
class QObject;

/**
 * @brief Qt Widgets main window showing the Top100 list and details pane.
 */
class Top100QtWindow : public QMainWindow {
    Q_OBJECT
public:
    /**
     * @brief Construct the main Qt Widgets window for Top100.
     * @param parent Optional parent widget
     */
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
    SpinnerWidget* posterSpinner_ = nullptr; // overlay spinner
    QWidget* detailsContainer_ = nullptr; // for width-based elide
    QWidget* posterContainer_ = nullptr;  // for poster scaling
    QTextBrowser* plotView_ = nullptr;
    QToolBar* toolbar_ = nullptr;
    QStatusBar* statusBar_ = nullptr;
    QLabel* statusCountLabel_ = nullptr;

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
    // spinner controlled directly via posterSpinner_

    // Construction helpers
    void buildLayout();
    void buildMenuBar();
    void buildToolbar();
    void connectSignals();
    void updateStatusMovieCount();

    // Handlers (implemented in handlers.cpp)
    void onAddMovie();
    void onDeleteCurrent();
    void onRefresh();
    void onPostBlueSky();
    void onPostMastodon();
    void onUpdateFromOmdb();
    void onOpenRankDialog();
    void onSortChanged();
    void updateDetails();

    // Poster (implemented in poster.cpp)
    void fetchPosterByUrl(const QString& url);
    void fetchPosterViaOmdb(const QString& imdb);
};
