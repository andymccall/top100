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
/*! \file ui/common/Top100ListModel.h
    \brief Shared QAbstractListModel exposing movies to both Qt and KDE UIs.
    
    Roles: title, year, rank, posterUrl, plotFull, imdbID; also supports
    Qt::DisplayRole for convenience. Provides reload(), get(row) for QML
    details binding, OMDb-assisted add, delete by IMDb ID, and async posting.
*/
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
#include "../../lib/omdb.h"

class Top100ListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int sortOrder READ sortOrder WRITE setSortOrder NOTIFY sortOrderChanged)
public:
    /** Model data roles available to views and QML. */
    enum Roles {
        TitleRole = Qt::UserRole + 1,
        YearRole,
        RankRole,
        PosterUrlRole,
        PlotFullRole,
        ImdbIdRole,
        DirectorRole,
        ActorsRole,
        GenresRole,
        RuntimeMinutesRole
    };
    Q_ENUM(Roles)

    /** Construct and immediately load the current Top100 dataset. */
    explicit Top100ListModel(QObject* parent = nullptr)
        : QAbstractListModel(parent) {
        // Initialize sort order from config
        try {
            AppConfig cfg = loadConfig();
            setSortOrder(cfg.uiSortOrder);
        } catch (...) {
            // fall back to default
            currentOrder_ = SortOrder::DEFAULT;
        }
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
            case ImdbIdRole: return QString::fromStdString(m.imdbID);
            case DirectorRole: return QString::fromStdString(m.director);
            case ActorsRole: {
                QStringList list;
                for (const auto& a : m.actors) list << QString::fromStdString(a);
                return list;
            }
            case GenresRole: {
                QStringList list;
                for (const auto& g : m.genres) list << QString::fromStdString(g);
                return list;
            }
            case RuntimeMinutesRole: return m.runtimeMinutes;
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
        r[ImdbIdRole] = "imdbID";
    r[DirectorRole] = "director";
    r[ActorsRole] = "actors";
    r[GenresRole] = "genres";
    r[RuntimeMinutesRole] = "runtimeMinutes";
        return r;
    }

    /** Reload movies from config-specified data file. Emits model reset. */
    Q_INVOKABLE void reload() {
        beginResetModel();
        movies_.clear();
        try {
            AppConfig cfg = loadConfig();
            Top100 list(cfg.dataFile);
            // Load using current sort order (defaults to insertion order)
            auto movies = list.getMovies(currentOrder_);
            movies_.assign(movies.begin(), movies.end());
        } catch (...) {
            movies_.clear();
        }
        endResetModel();
        qInfo() << "Top100ListModel: loaded" << movies_.size() << "movies";
        emit reloadCompleted();
    }

    // Current sort order as int (maps to SortOrder enum). Useful for QML bindings.
    int sortOrder() const { return static_cast<int>(currentOrder_); }

    /** Set sort order (0..4) corresponding to SortOrder enum; triggers reload(). */
    Q_INVOKABLE void setSortOrder(int order) {
        SortOrder newOrder = currentOrder_;
        switch (order) {
            case static_cast<int>(SortOrder::DEFAULT): newOrder = SortOrder::DEFAULT; break;
            case static_cast<int>(SortOrder::BY_YEAR): newOrder = SortOrder::BY_YEAR; break;
            case static_cast<int>(SortOrder::ALPHABETICAL): newOrder = SortOrder::ALPHABETICAL; break;
            case static_cast<int>(SortOrder::BY_USER_RANK): newOrder = SortOrder::BY_USER_RANK; break;
            case static_cast<int>(SortOrder::BY_USER_SCORE): newOrder = SortOrder::BY_USER_SCORE; break;
            default: newOrder = SortOrder::DEFAULT; break;
        }
        if (newOrder == currentOrder_) return;
        currentOrder_ = newOrder;
        // Persist preference
        try {
            AppConfig cfg = loadConfig();
            cfg.uiSortOrder = static_cast<int>(currentOrder_);
            saveConfig(cfg);
        } catch (...) { /* ignore */ }
        emit sortOrderChanged(static_cast<int>(currentOrder_));
        reload();
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
        m["imdbID"] = QString::fromStdString(mv.imdbID);
        m["director"] = QString::fromStdString(mv.director);
        {
            QVariantList actors;
            for (const auto& a : mv.actors) actors << QString::fromStdString(a);
            m["actors"] = actors;
        }
        {
            QVariantList genres;
            for (const auto& g : mv.genres) genres << QString::fromStdString(g);
            m["genres"] = genres;
        }
        m["runtimeMinutes"] = mv.runtimeMinutes;
        return m;
    }

    /** Add a movie by imdbID using OMDb lookup; appends to end of list and refreshes. */
    Q_INVOKABLE bool addMovieByImdbId(const QString& imdbId) {
        try {
            AppConfig cfg = loadConfig();
            if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) return false;
            auto maybe = omdbGetById(cfg.omdbApiKey, imdbId.toStdString());
            if (!maybe) return false;
            Movie mv = *maybe;
            // Ensure it's appended at the end by direct add (insertion order)
            Top100 list(cfg.dataFile);
            list.addMovie(mv);
            list.recomputeRanks();
            // Force persistence by destructing list (save in destructor)
        } catch (...) { return false; }
        reload();
        return true;
    }

    /** Delete a movie by imdbID; refreshes list. */
    Q_INVOKABLE bool deleteByImdbId(const QString& imdbId) {
        try {
            AppConfig cfg = loadConfig();
            Top100 list(cfg.dataFile);
            bool removed = list.removeByImdbId(imdbId.toStdString());
            if (!removed) return false;
            list.recomputeRanks();
        } catch (...) { return false; }
        reload();
        return true;
    }

    /** Delete a movie by title (first match); refreshes list. */
    Q_INVOKABLE bool deleteByTitle(const QString& title) {
        try {
            AppConfig cfg = loadConfig();
            Top100 list(cfg.dataFile);
            list.removeMovie(title.toStdString());
        } catch (...) { return false; }
        reload();
        return true;
    }

    /** Search OMDb for a query and return a list of { title, year, imdbID } objects. */
    Q_INVOKABLE QVariantList searchOmdb(const QString& query) {
        QVariantList out;
        try {
            AppConfig cfg = loadConfig();
            if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) return out;
            auto results = omdbSearch(cfg.omdbApiKey, query.toStdString());
            for (const auto& r : results) {
                QVariantMap m;
                m["title"] = QString::fromStdString(r.title);
                m["year"] = r.year;
                m["imdbID"] = QString::fromStdString(r.imdbID);
                out.push_back(m);
            }
        } catch (...) {
            // ignore, return empty
        }
        return out;
    }

    /** Fetch full OMDb details by IMDb ID and return a QVariantMap with keys:
     *  title, year, posterUrl, plotShort, plotFull. Returns empty map on error. */
    Q_INVOKABLE QVariantMap omdbGetByIdMap(const QString& imdbId) const {
        QVariantMap m;
        try {
            AppConfig cfg = loadConfig();
            if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) return m;
            auto maybe = omdbGetById(cfg.omdbApiKey, imdbId.toStdString());
            if (!maybe) return m;
            const Movie& mv = *maybe;
            m["title"] = QString::fromStdString(mv.title);
            m["year"] = mv.year;
            m["posterUrl"] = QString::fromStdString(mv.posterUrl);
            m["plotShort"] = QString::fromStdString(mv.plotShort);
            m["plotFull"] = QString::fromStdString(mv.plotFull);
        } catch (...) {
            // ignore
        }
        return m;
    }
    /** Update an existing movie by IMDb ID from OMDb and refresh. */
    Q_INVOKABLE bool updateFromOmdbByImdbId(const QString& imdbId) {
        try {
            AppConfig cfg = loadConfig();
            if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) return false;
            auto maybe = omdbGetById(cfg.omdbApiKey, imdbId.toStdString());
            if (!maybe) return false;
            Top100 list(cfg.dataFile);
            bool ok = list.mergeFromOmdbByImdbId(*maybe);
            if (!ok) return false;
        } catch (...) { return false; }
        // Preserve current selection by imdb when reloading
        QString imdb = imdbId;
        QMetaObject::Connection conn;
        conn = connect(this, &Top100ListModel::reloadCompleted, this, [this, imdb, &conn]() {
            for (int i = 0; i < rowCount(); ++i) {
                if (get(i).value("imdbID").toString() == imdb) { emit requestSelectRow(i); break; }
            }
            disconnect(conn);
        });
        reload();
        return true;
    }

    /** @brief QML-friendly row count accessor. */
    Q_INVOKABLE int count() const { return rowCount(); }

    /**
     * @brief Record a pairwise ranking result between two rows in the current model view.
     * @param leftRow Index of the left movie in the current model (0..rowCount-1)
     * @param rightRow Index of the right movie in the current model
     * @param winner Which movie won: 1 = left wins, 0 = right wins, -1 = pass (no change)
     * @return true if the operation succeeded (or pass), false on invalid input or storage error
     *
     * Applies an Elo-style update to userScore for the two movies, persists to disk,
     * recomputes ranks, and reloads the model. When winner == -1, no scores are changed
     * and the function returns true.
     */
    Q_INVOKABLE bool recordPairwiseResult(int leftRow, int rightRow, int winner);

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
    /** Emitted after reload to request view re-select a row by index. */
    void requestSelectRow(int row);
    /** Emitted when an async post finishes. */
    void postingFinished(const QString& service, int row, bool success);
    /** Emitted when sort order changes (value matches SortOrder enum). */
    void sortOrderChanged(int sortOrder);
    /** Emitted after model reload finishes (for selection preservation). */
    void reloadCompleted();
    /** Emitted when an asynchronous OMDb search completes. */
    void omdbSearchFinished(const QVariantList& results);
    /** Emitted when an asynchronous OMDb get-by-id completes. */
    void omdbGetFinished(const QVariantMap& movie);
    /** Emitted when an asynchronous add-by-imdb completes. */
    void addMovieFinished(const QString& imdbId, bool success);

private:
    std::vector<Movie> movies_;
    SortOrder currentOrder_ = SortOrder::DEFAULT;
    // Async watchers (owned by model)
    QFutureWatcher<QVariantList>* searchWatcher_ { nullptr };
    QFutureWatcher<QVariantMap>* getWatcher_ { nullptr };
    QFutureWatcher<bool>* addWatcher_ { nullptr };

public: // Async API (kept public for QML invocation)
    /** Fire-and-forget asynchronous OMDb search; emits omdbSearchFinished */
    Q_INVOKABLE void searchOmdbAsync(const QString& query) {
        if (query.trimmed().isEmpty()) return;
        if (searchWatcher_) { searchWatcher_->cancel(); searchWatcher_->deleteLater(); searchWatcher_ = nullptr; }
        searchWatcher_ = new QFutureWatcher<QVariantList>(this);
        auto fut = QtConcurrent::run([this, query]() { return this->searchOmdb(query); });
        connect(searchWatcher_, &QFutureWatcher<QVariantList>::finished, this, [this]() {
            if (!searchWatcher_) return;
            if (!searchWatcher_->isCanceled()) emit omdbSearchFinished(searchWatcher_->result());
            searchWatcher_->deleteLater(); searchWatcher_ = nullptr;
        });
        searchWatcher_->setFuture(fut);
    }
    /** Asynchronous fetch of full OMDb details; emits omdbGetFinished */
    Q_INVOKABLE void fetchOmdbByIdAsync(const QString& imdbId) {
        if (imdbId.trimmed().isEmpty()) return;
        if (getWatcher_) { getWatcher_->cancel(); getWatcher_->deleteLater(); getWatcher_ = nullptr; }
        getWatcher_ = new QFutureWatcher<QVariantMap>(this);
        auto fut = QtConcurrent::run([this, imdbId]() { return this->omdbGetByIdMap(imdbId); });
        connect(getWatcher_, &QFutureWatcher<QVariantMap>::finished, this, [this, imdbId]() {
            if (!getWatcher_) return;
            if (!getWatcher_->isCanceled()) emit omdbGetFinished(getWatcher_->result());
            getWatcher_->deleteLater(); getWatcher_ = nullptr;
        });
        getWatcher_->setFuture(fut);
    }
    /** Asynchronous add via OMDb ID; emits addMovieFinished */
    Q_INVOKABLE void addMovieByImdbIdAsync(const QString& imdbId) {
        if (imdbId.trimmed().isEmpty()) return;
        if (addWatcher_) { addWatcher_->cancel(); addWatcher_->deleteLater(); addWatcher_ = nullptr; }
        addWatcher_ = new QFutureWatcher<bool>(this);
        auto fut = QtConcurrent::run([this, imdbId]() { return this->addMovieByImdbId(imdbId); });
        connect(addWatcher_, &QFutureWatcher<bool>::finished, this, [this, imdbId]() {
            if (!addWatcher_) return;
            if (!addWatcher_->isCanceled()) emit addMovieFinished(imdbId, addWatcher_->result());
            addWatcher_->deleteLater(); addWatcher_ = nullptr;
        });
        addWatcher_->setFuture(fut);
    }
};
