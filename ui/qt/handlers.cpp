// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/handlers.cpp
// Purpose: Implementation of event handlers for Top100QtWindow.
//-------------------------------------------------------------------------------
#include "handlers.h"

#include "adddialog.h"

#include <QMessageBox>
#include <QListView>
#include <QComboBox>
#include <QLabel>
#include <QTextBrowser>
#include <QStatusBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include "../common/Top100ListModel.h"
#include "../../lib/config.h"
#include "../../lib/omdb.h"
#include "../common/constants.h"
#include "rankdialog.h"

using namespace ui_constants;

void Top100QtWindow::updateDetails() {
    QModelIndex idx = listView_->currentIndex();
    if (!idx.isValid()) {
        titleLabel_->clear(); posterLabel_->clear(); plotView_->clear();
        directorValue_->clear(); actorsValue_->clear(); genresValue_->clear(); runtimeValue_->clear(); imdbLink_->clear();
        return;
    }
    int row = idx.row();
    QVariantMap m = model_->get(row);
    const QString t = m.value("title").toString();
    const int y = m.value("year").toInt();
    QString head = t; if (y > 0) head += QString(" (%1)").arg(y);
    titleLabel_->setText(head);
    plotView_->setPlainText(m.value("plotFull").toString());
    directorValue_->setText(m.value("director").toString());
    {
        QStringList actors; for (const auto& v : m.value("actors").toList()) actors << v.toString();
        QString fullA = actors.join(", ");
        actorsValue_->setProperty("fullActors", fullA);
        // Update label immediately; resizer will re-elide on container resize
        QFontMetrics fm(actorsValue_->font());
        int padding = ui_constants::kGroupPadding * 2;
        int labelCol = ui_constants::kLabelMinWidth + (ui_constants::kSpacingSmall + 6);
        int maxW = qMax(100, detailsContainer_->width() - padding - labelCol);
        actorsValue_->setMaximumWidth(maxW);
        QString elided = fm.elidedText(fullA, Qt::ElideRight, maxW);
        actorsValue_->setText(elided);
        actorsValue_->setToolTip(fullA);
    }
    {
        QStringList genres; for (const auto& v : m.value("genres").toList()) genres << v.toString();
        genresValue_->setText(genres.join(", "));
    }
    int runtime = m.value("runtimeMinutes").toInt();
    runtimeValue_->setText(runtime > 0 ? QString::number(runtime) + QStringLiteral(" min") : QString());
    const QString imdb = m.value("imdbID").toString();
    if (!imdb.isEmpty()) {
        const QString url = QStringLiteral("https://www.imdb.com/title/") + imdb + QStringLiteral("/");
        imdbLink_->setText(QStringLiteral("<a href=\"") + url + QStringLiteral("\">Open</a>"));
    } else {
        imdbLink_->clear();
    }
    // Clear poster and property; schedule fetch
    posterLabel_->setPixmap(QPixmap());
    posterLabel_->setProperty("origPm", QVariant());
    const QString url = m.value("posterUrl").toString();
    if (!url.isEmpty() && url != "N/A") {
        fetchPosterByUrl(url);
    } else if (!imdb.isEmpty()) {
        fetchPosterViaOmdb(imdb);
    }
}

void Top100QtWindow::onAddMovie() {
    Top100QtAddDialog dlg(this, model_);
    if (dlg.exec() == QDialog::Accepted) {
        const QString imdb = dlg.selectedImdb();
        if (!imdb.isEmpty()) {
            if (model_->addMovieByImdbId(imdb)) {
                statusBar()->showMessage(QStringLiteral("Added movie."), 3000);
            } else {
                QMessageBox::warning(this, QStringLiteral("Add Failed"), QStringLiteral("Could not add movie (check OMDb config)."));
            }
        }
    }
}

void Top100QtWindow::onDeleteCurrent() {
    int row = listView_->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, QStringLiteral("Delete"), QStringLiteral("Select a movie first.")); return; }
    auto m = model_->get(row);
    QString imdb = m.value("imdbID").toString();
    if (imdb.isEmpty()) { QMessageBox::warning(this, QStringLiteral("Delete"), QStringLiteral("IMDb ID not available; delete not supported yet.")); return; }
    if (QMessageBox::question(this, QStringLiteral("Delete"), QStringLiteral("Delete this movie?")) != QMessageBox::Yes) return;
    if (model_->deleteByImdbId(imdb)) statusBar()->showMessage(QStringLiteral("Deleted."), 3000);
    else QMessageBox::warning(this, QStringLiteral("Delete"), QStringLiteral("Failed to delete."));
}

void Top100QtWindow::onRefresh() { model_->reload(); }

void Top100QtWindow::onPostBlueSky() {
    int row = listView_->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, QStringLiteral("Post BlueSky"), QStringLiteral("Please select a movie first.")); return; }
    statusBar()->showMessage(QStringLiteral("Posting to BlueSky..."), 3000);
    model_->postToBlueSkyAsync(row);
}

void Top100QtWindow::onPostMastodon() {
    int row = listView_->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, QStringLiteral("Post Mastodon"), QStringLiteral("Please select a movie first.")); return; }
    statusBar()->showMessage(QStringLiteral("Posting to Mastodon..."), 3000);
    model_->postToMastodonAsync(row);
}

void Top100QtWindow::onUpdateFromOmdb() {
    int row = listView_->currentIndex().row();
    if (row < 0) { QMessageBox::warning(this, QStringLiteral("Update"), QStringLiteral("Select a movie first.")); return; }
    QString imdb = model_->get(row).value("imdbID").toString();
    if (imdb.isEmpty()) { QMessageBox::warning(this, QStringLiteral("Update"), QStringLiteral("No IMDb ID for this movie.")); return; }
    statusBar()->showMessage(QStringLiteral("Updating from OMDb..."), 2000);
    if (model_->updateFromOmdbByImdbId(imdb)) {
        QMetaObject::Connection conn;
        conn = connect(model_, &Top100ListModel::reloadCompleted, this, [this, &conn]() { statusBar()->showMessage(QStringLiteral("Updated from OMDb."), 3000); disconnect(conn); });
    } else {
        QMessageBox::warning(this, QStringLiteral("Update"), QStringLiteral("Update failed."));
    }
}

void Top100QtWindow::onOpenRankDialog() {
    if (!model_ || model_->rowCount() < 2) {
        QMessageBox::information(this, QStringLiteral("Rank"), QStringLiteral("Need at least 2 movies to rank."));
        return;
    }
    Top100QtRankDialog dlg(this, model_);
    dlg.exec();
}

void Top100QtWindow::onSortChanged() {
    // Preserve current imdbID to reselect after reload
    int row = listView_->currentIndex().row();
    QString imdb; if (row >= 0) imdb = model_->get(row).value("imdbID").toString();
    int order = sortCombo_->currentData().toInt();
    QMetaObject::Connection conn;
    conn = connect(model_, &Top100ListModel::reloadCompleted, this, [this, imdb, &conn]() {
        for (int i = 0; i < model_->rowCount(); ++i) {
            if (model_->get(i).value("imdbID").toString() == imdb) { listView_->setCurrentIndex(model_->index(i, 0)); break; }
        }
        disconnect(conn);
    });
    model_->setSortOrder(order);
}
