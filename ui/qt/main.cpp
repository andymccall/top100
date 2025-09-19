// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/qt/main.cpp
// Purpose: Qt Widgets UI entry point (two-pane layout, poster loading, async posting).
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------

#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QStringList>
#include <QListView>
#include <QTimer>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QTextBrowser>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include "../common/strings.h"

// Core library headers
#include "../../lib/config.h"
#include "../../lib/top100.h"
#include "../../lib/Movie.h"
#include "../common/Top100ListModel.h"

// Small adaptor: fetch titles from the top100 library and convert to QStringList.
// No longer needed: model handles loading

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow win;
    win.setWindowTitle(ui_strings::kQtWindowTitle);

    auto *central = new QWidget(&win);
    auto *vbox = new QVBoxLayout(central);

    // Splitter: left list, right details
    auto *splitter = new QSplitter(Qt::Horizontal, central);

    // Left: List view powered by shared model
    auto *listView = new QListView(splitter);
    listView->setSelectionMode(QAbstractItemView::SingleSelection);
    listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    auto *model = new Top100ListModel(&win);
    listView->setModel(model);

    // Right: Details pane
    auto *details = new QWidget(splitter);
    auto *detailsLayout = new QVBoxLayout(details);
    auto *titleLabel = new QLabel(details);
    titleLabel->setWordWrap(true);
    titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont f = titleLabel->font();
    f.setPointSize(f.pointSize() + 2);
    f.setBold(true);
    titleLabel->setFont(f);
    auto *posterLabel = new QLabel(details);
    posterLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    auto *plotView = new QTextBrowser(details);
    plotView->setReadOnly(true);
    plotView->setOpenExternalLinks(true);
    detailsLayout->addWidget(titleLabel);
    detailsLayout->addWidget(posterLabel);
    detailsLayout->addWidget(plotView, 1);
    details->setLayout(detailsLayout);

    splitter->addWidget(listView);
    splitter->addWidget(details);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    vbox->addWidget(splitter);
    central->setLayout(vbox);
    win.setCentralWidget(central);

    // Network for poster loading
    auto *nam = new QNetworkAccessManager(&win);

    auto updateDetails = [listView, model, titleLabel, posterLabel, plotView, nam]() {
        QModelIndex idx = listView->currentIndex();
        if (!idx.isValid()) {
            titleLabel->clear();
            posterLabel->clear();
            plotView->clear();
            return;
        }
        int row = idx.row();
        QVariantMap m = model->get(row);
        const QString t = m.value("title").toString();
        const int y = m.value("year").toInt();
        const int r = m.value("rank").toInt();
        QString head = t;
        if (y > 0) head += QString(" (%1)").arg(y);
        if (r > 0) head = QString("#%1 ").arg(r) + head;
        titleLabel->setText(head);
        plotView->setPlainText(m.value("plotFull").toString());
        posterLabel->setPixmap(QPixmap());
        const QString url = m.value("posterUrl").toString();
        if (!url.isEmpty() && url != "N/A") {
            QNetworkRequest req{QUrl(url)};
            QNetworkReply *reply = nam->get(req);
            QObject::connect(reply, &QNetworkReply::finished, posterLabel, [reply, posterLabel]() {
                reply->deleteLater();
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray data = reply->readAll();
                    QPixmap pm;
                    if (pm.loadFromData(data)) {
                        int w = posterLabel->width() > 0 ? posterLabel->width() : 300;
                        QPixmap scaled = pm.scaledToWidth(w, Qt::SmoothTransformation);
                        posterLabel->setPixmap(scaled);
                    }
                }
            });
        }
    };
    QObject::connect(listView->selectionModel(), &QItemSelectionModel::currentChanged, &win, [=](const QModelIndex&, const QModelIndex&) { updateDetails(); });
    // Select first item initially
    if (model->rowCount() > 0) {
        listView->setCurrentIndex(model->index(0, 0));
        updateDetails();
    }

    // Toolbar with actions: Add, Delete, Refresh, Post to BlueSky, Post to Mastodon
    QToolBar *toolbar = new QToolBar(&win);
    win.addToolBar(Qt::TopToolBarArea, toolbar);
    QAction *addAct = toolbar->addAction(QStringLiteral("Add"));
    QAction *delAct = toolbar->addAction(QStringLiteral("Delete"));
    QAction *refreshTbAct = toolbar->addAction(QStringLiteral("Refresh"));
    QAction *postBskyAct = toolbar->addAction(QStringLiteral("Post BlueSky"));
    QAction *postMastoAct = toolbar->addAction(QStringLiteral("Post Mastodon"));

    QObject::connect(addAct, &QAction::triggered, &win, [&win]() {
        QMessageBox::information(&win, QStringLiteral("Add"), QStringLiteral("Add is not implemented yet."));
    });
    QObject::connect(delAct, &QAction::triggered, &win, [&win]() {
        QMessageBox::information(&win, QStringLiteral("Delete"), QStringLiteral("Delete is not implemented yet."));
    });
    QObject::connect(refreshTbAct, &QAction::triggered, model, &Top100ListModel::reload);
    QObject::connect(postBskyAct, &QAction::triggered, &win, [model, listView, &win]() {
        int row = listView->currentIndex().row();
        if (row < 0) {
            QMessageBox::warning(&win, QStringLiteral("Post BlueSky"), QStringLiteral("Please select a movie first."));
            return;
        }
        win.statusBar()->showMessage(QStringLiteral("Posting to BlueSky..."), 3000);
        model->postToBlueSkyAsync(row);
    });
    QObject::connect(postMastoAct, &QAction::triggered, &win, [model, listView, &win]() {
        int row = listView->currentIndex().row();
        if (row < 0) {
            QMessageBox::warning(&win, QStringLiteral("Post Mastodon"), QStringLiteral("Please select a movie first."));
            return;
        }
        win.statusBar()->showMessage(QStringLiteral("Posting to Mastodon..."), 3000);
        model->postToMastodonAsync(row);
    });

    QObject::connect(model, &Top100ListModel::postingFinished, &win, [&win](const QString& service, int, bool ok) {
        win.statusBar()->showMessage((ok ? QStringLiteral("Posted to ") : QStringLiteral("Posting failed: ")) + service, 5000);
    });

    // Menu bar
    QMenuBar *menuBar = win.menuBar();
    QMenu *fileMenu = menuBar->addMenu(QString::fromUtf8(ui_strings::kMenuFile));
    QAction *refreshAct = fileMenu->addAction(QStringLiteral("Refresh"));
    QObject::connect(refreshAct, &QAction::triggered, model, &Top100ListModel::reload);

    QAction *quitAct = fileMenu->addAction(QString::fromUtf8(ui_strings::kActionQuit));
    QObject::connect(quitAct, &QAction::triggered, &app, &QCoreApplication::quit);

    QMenu *helpMenu = menuBar->addMenu(QString::fromUtf8(ui_strings::kMenuHelp));
    QAction *aboutAct = helpMenu->addAction(QString::fromUtf8(ui_strings::kActionAbout));
    QObject::connect(aboutAct, &QAction::triggered, &win, [&win]() {
        QMessageBox::about(&win,
                           QString::fromUtf8(ui_strings::kActionAbout),
                           QString::fromUtf8(ui_strings::kAboutDialogText));
    });

    win.resize(900, 600);
    win.show();

    return app.exec();
}
