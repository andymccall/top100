// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/poster.cpp
// Purpose: Poster fetching helpers for Top100QtWindow.
//-------------------------------------------------------------------------------
#include "poster.h"

#include <QLabel>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QtConcurrent>

#include "../../lib/config.h"
#include "../../lib/omdb.h"
#include "../common/constants.h"

using namespace ui_constants;

void Top100QtWindow::fetchPosterByUrl(const QString& posterUrl) {
    if (!nam_) nam_ = new QNetworkAccessManager(this);
    if (posterSpinner_) posterSpinner_->start();
    QNetworkRequest req{QUrl(posterUrl)};
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Top100/1.0 (Qt)"));
    req.setRawHeader("Accept", "image/*, */*;q=0.8");
    QNetworkReply *reply = nam_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QPixmap pm;
            if (pm.loadFromData(data)) {
                posterLabel_->setProperty("origPm", pm);
                int maxW = posterContainer_->width() > 0 ? int(posterContainer_->width() * kPosterMaxWidthRatio) : 400;
                int maxH = posterContainer_->height() > 0 ? int(posterContainer_->height() * kPosterMaxHeightRatio) : 600;
                QPixmap scaled = pm.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                posterLabel_->setPixmap(scaled);
            }
        } else {
            posterLabel_->clear();
        }
        if (posterSpinner_) posterSpinner_->stop();
    });
}

void Top100QtWindow::fetchPosterViaOmdb(const QString& imdb) {
    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher]() mutable {
        QString poster = watcher->result();
        watcher->deleteLater();
        if (!poster.isEmpty()) fetchPosterByUrl(poster);
        else { if (posterSpinner_) posterSpinner_->stop(); }
    });
    QFuture<QString> fut = QtConcurrent::run([imdb]() -> QString {
        try {
            AppConfig cfg = loadConfig();
            if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) return QString();
            auto maybe = omdbGetById(cfg.omdbApiKey, imdb.toStdString());
            if (!maybe) return QString();
            return QString::fromStdString(maybe->posterUrl);
        } catch (...) { return QString(); }
    });
    watcher->setFuture(fut);
}
