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
class QStandardItemModel;
class QNetworkAccessManager;

class Top100ListModel;

class Top100QtAddDialog : public QDialog {
    Q_OBJECT
public:
    Top100QtAddDialog(QWidget* parent, Top100ListModel* model);
    QString selectedImdb() const { return selectedImdb_; }

protected:
    void showEvent(QShowEvent* ev) override;
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
    QTextBrowser* plotView_ = nullptr;
    QNetworkAccessManager* nam_ = nullptr;
    QPixmap origPoster_;
    QString selectedImdb_;

    void doSearch();
    void onResultSelectionChanged();
    void loadPoster(const QString& url);
    void rescalePoster();
};
