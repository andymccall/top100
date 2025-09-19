// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/common/Top100ListModel.h
// Purpose: Shared QAbstractListModel for Top100 UI (Qt/KDE).
// Language: C++17 (header)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
//
// Top100ListModel: Shared QAbstractListModel exposing movies to both Qt and KDE UIs.
// Roles: title, year, rank, posterUrl, plotFull; also supports Qt::DisplayRole for convenience.
// Provides reload(), get(row) for QML details binding, and async posting helpers.
#pragma once

#include <QAbstractListModel>
#include <QHash>
#include <QByteArray>
#include <vector>
#include <string>
#include <QString>
#include <QVariantMap>
#include <QFutureWatcher>
#include <QDebug>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QtConcurrent>
#else
#include <QtConcurrent/QtConcurrentRun>
#endif

#include "../../lib/Movie.h"
#include "../../lib/top100.h"
#include "../../lib/config.h"
#include "../../lib/posting.h"

class Top100ListModel : public QAbstractListModel {
    Q_OBJECT
public:
    /** Model data roles available to views and QML. */
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        YearRole,
        RankRole,
        PosterUrlRole,
        PlotFullRole
    };
    Q_ENUM(Roles)

    /** Construct and immediately load the current Top100 dataset. */
    explicit Top100ListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent) {
        reload();
    }

    int rowCount(const QModelIndex& parent = QModelIndex()) const override {
        if (parent.isValid()) return 0;
        return static_cast<int>(movies_.size());
    }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= static_cast<int>(movies_.size()))
            return {};
        const auto& m = movies_[index.row()];
        switch (role) {
            case Qt::DisplayRole: {
                QString prefix = (m.userRank > 0) ? QString("#%1 ").arg(m.userRank) : QString();
                return prefix + QString::fromStdString(m.title) + QString(" (%1)").arg(m.year);
            }
            case TitleRole: return QString::fromStdString(m.title);
            case YearRole: return m.year;
            case RankRole: return m.userRank;
            case PosterUrlRole: return QString::fromStdString(m.posterUrl);
            case PlotFullRole: return QString::fromStdString(m.plotFull);
            default: return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override {
        QHash<int, QByteArray> r;
        r[Qt::DisplayRole] = "display";
        r[TitleRole] = "title";
        r[YearRole] = "year";
        r[RankRole] = "rank";
        r[PosterUrlRole] = "posterUrl";
        r[PlotFullRole] = "plotFull";
        return r;
    }

    /** Reload movies from config-specified data file. Emits model reset. */
    Q_INVOKABLE void reload() {
        beginResetModel();
        movies_.clear();
        try {
            AppConfig cfg = loadConfig();
            Top100 list(cfg.dataFile);
            auto movies = list.getMovies(SortOrder::BY_USER_RANK);
            if (movies.empty()) movies = list.getMovies(SortOrder::ALPHABETICAL);
            movies_.assign(movies.begin(), movies.end());
        } catch (...) {
            movies_.clear();
        }
        endResetModel();
        qInfo() << "Top100ListModel: loaded" << movies_.size() << "movies";
    }

    /** Convenience accessor for QML/details panes. Returns a QVariantMap with keys:
     *  title, year, rank, posterUrl, plotFull. Returns empty map for invalid row. */
    Q_INVOKABLE QVariantMap get(int row) const {
        QVariantMap m;
        if (row < 0 || row >= static_cast<int>(movies_.size())) return m;
        const auto& mv = movies_[row];
        m["title"] = QString::fromStdString(mv.title);
        m["year"] = mv.year;
        m["rank"] = mv.userRank;
        m["posterUrl"] = QString::fromStdString(mv.posterUrl);
        m["plotFull"] = QString::fromStdString(mv.plotFull.empty() ? mv.plotShort : mv.plotFull);
        return m;
    }

    /** Post the selected movie to BlueSky synchronously. */
    Q_INVOKABLE bool postToBlueSky(int row) {
        if (row < 0 || row >= static_cast<int>(movies_.size())) return false;
        try {
            AppConfig cfg = loadConfig();
            if (!cfg.blueSkyEnabled || cfg.blueSkyIdentifier.empty() || cfg.blueSkyAppPassword.empty()) return false;
            const Movie& m = movies_[row];
            return postMovieToBlueSky(cfg, m);
        } catch (...) {
            return false;
        }
    }

    /** Post the selected movie to Mastodon synchronously. */
    Q_INVOKABLE bool postToMastodon(int row) {
        if (row < 0 || row >= static_cast<int>(movies_.size())) return false;
        try {
            AppConfig cfg = loadConfig();
            if (!cfg.mastodonEnabled || cfg.mastodonInstance.empty() || cfg.mastodonAccessToken.empty()) return false;
            const Movie& m = movies_[row];
            return postMovieToMastodon(cfg, m);
        } catch (...) {
            return false;
        }
    }

    /** Post to BlueSky asynchronously; emits postingFinished when done. */
    Q_INVOKABLE void postToBlueSkyAsync(int row) {
        if (row < 0 || row >= static_cast<int>(movies_.size())) return;
        AppConfig cfg;
        try { cfg = loadConfig(); } catch (...) { emit postingFinished("BlueSky", row, false); return; }
        if (!cfg.blueSkyEnabled || cfg.blueSkyIdentifier.empty() || cfg.blueSkyAppPassword.empty()) { emit postingFinished("BlueSky", row, false); return; }
        Movie mv = movies_[row];
        auto future = QtConcurrent::run([cfg, mv]() { return postMovieToBlueSky(cfg, mv); });
        auto *watcher = new QFutureWatcher<bool>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, row]() {
            bool ok = watcher->result();
            emit postingFinished("BlueSky", row, ok);
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

    /** Post to Mastodon asynchronously; emits postingFinished when done. */
    Q_INVOKABLE void postToMastodonAsync(int row) {
        if (row < 0 || row >= static_cast<int>(movies_.size())) return;
        AppConfig cfg;
        try { cfg = loadConfig(); } catch (...) { emit postingFinished("Mastodon", row, false); return; }
        if (!cfg.mastodonEnabled || cfg.mastodonInstance.empty() || cfg.mastodonAccessToken.empty()) { emit postingFinished("Mastodon", row, false); return; }
        Movie mv = movies_[row];
        auto future = QtConcurrent::run([cfg, mv]() { return postMovieToMastodon(cfg, mv); });
        auto *watcher = new QFutureWatcher<bool>(this);
        connect(watcher, &QFutureWatcher<bool>::finished, this, [this, watcher, row]() {
            bool ok = watcher->result();
            emit postingFinished("Mastodon", row, ok);
            watcher->deleteLater();
        });
        watcher->setFuture(future);
    }

signals:
    /** Emitted when an async post finishes. */
    void postingFinished(const QString& service, int row, bool success);

private:
    std::vector<Movie> movies_;
};
