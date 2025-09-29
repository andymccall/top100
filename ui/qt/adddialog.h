// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/adddialog.h
// Purpose: Qt dialog to search OMDb, preview details, and select a movie to add.
//-------------------------------------------------------------------------------
#pragma once

#include <QDialog>
#include <QVariantList>

class QLineEdit;
class QPushButton;
class QListView;
class QLabel;
class QTextBrowser;
#include "spinner.h"
class QStandardItemModel;
class QNetworkAccessManager;
class QFutureWatcherBase; // forward
template <typename T> class QFutureWatcher;

class Top100ListModel;

/**
 * @brief Qt dialog to search OMDb, preview details, and add a movie.
 */
class Top100QtAddDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief Construct the OMDb search/add dialog.
     * @param parent Parent widget
     * @param model Shared Top100 list model
     */
    Top100QtAddDialog(QWidget* parent, Top100ListModel* model);
    /** @brief IMDb ID selected from search results (empty if none).
     *  @return Selected IMDb ID string. */
    QString selectedImdb() const { return selectedImdb_; }

protected:
    /**
     * @brief Ensure initial focus and layout after show.
     * @param ev Show event
     */
    void showEvent(QShowEvent* ev) override;
    /**
     * @brief Rescale poster on resize.
     * @param ev Resize event
     */
    void resizeEvent(QResizeEvent* ev) override;

private:
    Top100ListModel* model_ = nullptr;
    QLineEdit* queryEdit_ = nullptr;
    QPushButton* searchBtn_ = nullptr;
    QPushButton* addBtn_ = nullptr;
    QListView* resultsView_ = nullptr;
    QStandardItemModel* resultsModel_ = nullptr;
    QLabel* titleLbl_ = nullptr;
    QLabel* posterLbl_ = nullptr;
    SpinnerWidget* posterSpinner_ = nullptr;
    QTextBrowser* plotView_ = nullptr;
    QNetworkAccessManager* nam_ = nullptr;
    QPixmap origPoster_;
    QString selectedImdb_;
    QFutureWatcher<QVariantList>* searchWatcher_ = nullptr; // async search watcher
    // spinner controlled directly via posterSpinner_

    void doSearch();
    void onResultSelectionChanged();
    void loadPoster(const QString& url);
    void rescalePoster();
};
