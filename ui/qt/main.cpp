// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
/*! \file ui/qt/main.cpp
    \brief Qt Widgets UI entry point and main window wiring.
    
    Provides a two-pane layout (list + details), poster loading with size
    constraints, social posting actions, and add/delete via OMDb.
*/
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
#include <QComboBox>
#include <QPixmap>
#include <QToolBar>
#include <QToolButton>
#include <QStatusBar>
#include <QSplitter>
#include <QFrame>
#include <QGroupBox>
#include <QTextBrowser>
#include <QPixmap>
#include <QGridLayout>
#include <QScreen>
#include <QEvent>
#include <QVariant>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QSslSocket>
#include <QtConcurrent>
#include <QInputDialog>
#include <QLineEdit>
#include "../common/strings.h"
#include "../common/constants.h"

// Core library headers
#include "../../lib/config.h"
#include "../../lib/top100.h"
#include "../../lib/Movie.h"
#include "../common/Top100ListModel.h"
#include "../../lib/omdb.h"

// Small adaptor: fetch titles from the top100 library and convert to QStringList.
// No longer needed: model handles loading

namespace {
// Helper to keep poster scaled on window resize without stretching
class PosterResizer : public QObject {
public:
    PosterResizer(QWidget* container, QLabel* label)
        : QObject(container), container_(container), label_(label) {}
protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (obj == container_ && ev->type() == QEvent::Resize) {
            QVariant v = label_->property("origPm");
            if (v.isValid()) {
                QPixmap pm = v.value<QPixmap>();
                if (!pm.isNull()) {
                    int maxW = qMax(1, int(container_->width() * ui_constants::kPosterMaxWidthRatio));
                    int maxH = qMax(1, int(container_->height() * ui_constants::kPosterMaxHeightRatio));
                    QPixmap scaled = pm.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    label_->setPixmap(scaled);
                }
            }
        }
        return QObject::eventFilter(obj, ev);
    }
private:
    QWidget* container_;
    QLabel* label_;
};
} // namespace

namespace {
// Helper to wrap actors based on available width of the details pane
class DetailsResizer : public QObject {
    QWidget* container_;
    QLabel* actors_;
    QString fullActors_;
public:
    DetailsResizer(QWidget* container, QLabel* actors)
        : QObject(container), container_(container), actors_(actors) {
        actors_->setWordWrap(false);
        actors_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    }
    void setFullActors(const QString& text) {
        fullActors_ = text;
        updateElide();
    }
protected:
    bool eventFilter(QObject* obj, QEvent* ev) override {
        if (obj == container_ && ev->type() == QEvent::Resize) {
            updateElide();
        }
        return QObject::eventFilter(obj, ev);
    }
private:
    void updateElide() {
        if (!actors_) return;
        int padding = ui_constants::kGroupPadding * 2;
        int labelCol = ui_constants::kLabelMinWidth + (ui_constants::kSpacingSmall + 6);
        int maxW = qMax(100, container_->width() - padding - labelCol);
        actors_->setMaximumWidth(maxW);
        QFontMetrics fm(actors_->font());
        QString elided = fm.elidedText(fullActors_, Qt::ElideRight, maxW);
        actors_->setText(elided);
        actors_->setToolTip(fullActors_);
    }
};
} // namespace

/**
 * @brief Qt Widgets application entry point.
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code
 */
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    QMainWindow win;
    win.setWindowTitle(ui_strings::kQtWindowTitle);

    // Log SSL support status (helps diagnose HTTPS image loading)
    qInfo() << "Qt SSL supported:" << QSslSocket::supportsSsl();

    auto *central = new QWidget(&win);
    auto *vbox = new QVBoxLayout(central);

    // Splitter: left list, right details
    auto *splitter = new QSplitter(Qt::Horizontal, central);

    // Left: Frame with heading, sort order, and list view
    auto *leftFrame = new QFrame(splitter);
    leftFrame->setFrameShape(QFrame::StyledPanel);
    auto *leftLayout = new QVBoxLayout(leftFrame);
    leftLayout->setSpacing(ui_constants::kSpacingSmall);
    leftLayout->setContentsMargins(ui_constants::kGroupPadding, ui_constants::kGroupPadding, ui_constants::kGroupPadding, ui_constants::kGroupPadding);
    auto *leftHeading = new QLabel(QString::fromUtf8(ui_strings::kHeadingMovies), leftFrame);
    QFont hf = leftHeading->font(); hf.setBold(true); hf.setPointSize(hf.pointSize() + 2); leftHeading->setFont(hf);
    leftLayout->addWidget(leftHeading);
    auto *sortRow = new QWidget(leftFrame);
    auto *sortRowLayout = new QHBoxLayout(sortRow);
    sortRowLayout->setSpacing(ui_constants::kSpacingSmall);
    sortRowLayout->setContentsMargins(0, 0, 0, 0);
    sortRowLayout->addWidget(new QLabel(QString::fromUtf8(ui_strings::kLabelSortOrder), sortRow));
    auto *sortCombo = new QComboBox(sortRow);
    sortCombo->addItem(QString::fromUtf8(ui_strings::kSortInsertion), static_cast<int>(SortOrder::DEFAULT));
    sortCombo->addItem(QString::fromUtf8(ui_strings::kSortByYear), static_cast<int>(SortOrder::BY_YEAR));
    sortCombo->addItem(QString::fromUtf8(ui_strings::kSortAlpha), static_cast<int>(SortOrder::ALPHABETICAL));
    sortCombo->addItem(QString::fromUtf8(ui_strings::kSortByRank), static_cast<int>(SortOrder::BY_USER_RANK));
    sortCombo->addItem(QString::fromUtf8(ui_strings::kSortByScore), static_cast<int>(SortOrder::BY_USER_SCORE));
    sortRowLayout->addWidget(sortCombo, 1);
    leftLayout->addWidget(sortRow);
    auto *listView = new QListView(leftFrame);
    listView->setSelectionMode(QAbstractItemView::SingleSelection);
    listView->setSelectionBehavior(QAbstractItemView::SelectRows);
    auto *model = new Top100ListModel(&win);
    listView->setModel(model);
    leftLayout->addWidget(listView, 1);
    leftFrame->setLayout(leftLayout);

    // Right: Details frame
    auto *details = new QFrame(splitter);
    details->setFrameShape(QFrame::StyledPanel);
    auto *detailsLayout = new QVBoxLayout(details);
    detailsLayout->setSpacing(ui_constants::kSpacingSmall);
    detailsLayout->setContentsMargins(ui_constants::kGroupPadding, ui_constants::kGroupPadding, ui_constants::kGroupPadding, ui_constants::kGroupPadding);
    auto *detailsHeading = new QLabel(QString::fromUtf8(ui_strings::kHeadingDetails), details);
    QFont df = detailsHeading->font(); df.setBold(true); df.setPointSize(df.pointSize() + 2); detailsHeading->setFont(df);
    detailsLayout->addWidget(detailsHeading);
    auto *titleLabel = new QLabel(details);
    titleLabel->setWordWrap(true);
    titleLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont f = titleLabel->font();
    f.setPointSize(f.pointSize() + 2);
    f.setBold(true);
    titleLabel->setFont(f);
    // Details grid under the title
    auto *detailsGridWidget = new QWidget(details);
    auto *detailsGrid = new QGridLayout(detailsGridWidget);
    detailsGrid->setContentsMargins(0, 0, 0, 0);
    detailsGrid->setHorizontalSpacing(ui_constants::kSpacingSmall + 6);
    detailsGrid->setVerticalSpacing(4);
    // Keep two snug columns; don't let columns stretch across the full width
    detailsGrid->setColumnStretch(0, 0);
    detailsGrid->setColumnStretch(1, 0);
    // Prevent vertical stretching of the rows when window expands
    detailsGridWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    auto makeFieldLabel = [&](const char* text){
        auto *lbl = new QLabel(QString::fromUtf8(text), detailsGridWidget);
        lbl->setMinimumWidth(ui_constants::kLabelMinWidth);
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        return lbl;
    };
    auto *directorValue = new QLabel(detailsGridWidget);
    directorValue->setWordWrap(true);
    directorValue->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    auto *actorsValue = new QLabel(detailsGridWidget);
    actorsValue->setWordWrap(false);
    actorsValue->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    auto *genresValue = new QLabel(detailsGridWidget);
    genresValue->setWordWrap(true);
    genresValue->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    auto *runtimeValue = new QLabel(detailsGridWidget);
    runtimeValue->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    auto *imdbLink = new QLabel(detailsGridWidget);
    imdbLink->setTextFormat(Qt::RichText);
    imdbLink->setOpenExternalLinks(true);
    imdbLink->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    int rowF = 0;
    detailsGrid->addWidget(makeFieldLabel(ui_strings::kFieldDirector), rowF, 0);
    detailsGrid->addWidget(directorValue, rowF, 1); ++rowF;
    detailsGrid->addWidget(makeFieldLabel(ui_strings::kFieldActors), rowF, 0);
    detailsGrid->addWidget(actorsValue, rowF, 1); ++rowF;
    detailsGrid->addWidget(makeFieldLabel(ui_strings::kFieldGenres), rowF, 0);
    detailsGrid->addWidget(genresValue, rowF, 1); ++rowF;
    detailsGrid->addWidget(makeFieldLabel(ui_strings::kFieldRuntime), rowF, 0);
    detailsGrid->addWidget(runtimeValue, rowF, 1); ++rowF;
    detailsGrid->addWidget(makeFieldLabel(ui_strings::kFieldImdbPage), rowF, 0);
    detailsGrid->addWidget(imdbLink, rowF, 1); ++rowF;

    auto *posterLabel = new QLabel(details);
    posterLabel->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    // Prevent image from driving window resize; do not stretch: we'll scale pixmap manually with aspect ratio
    posterLabel->setScaledContents(false);
    posterLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    auto *plotView = new QTextBrowser(details);
    plotView->setReadOnly(true);
    plotView->setOpenExternalLinks(true);
    detailsLayout->addWidget(titleLabel);
    // Keep details compact within a 20% area by wrapping in a container with a stretch spacer
    auto *detailsTopContainer = new QWidget(details);
    auto *detailsTopVBox = new QVBoxLayout(detailsTopContainer);
    detailsTopVBox->setSpacing(0);
    detailsTopVBox->setContentsMargins(0, 0, 0, 0);
    // Left-pack the grid in a row with a right-side stretch so it doesn't span full width
    auto *detailsGridRow = new QWidget(details);
    auto *detailsGridRowLayout = new QHBoxLayout(detailsGridRow);
    detailsGridRowLayout->setContentsMargins(0, 0, 0, 0);
    detailsGridRowLayout->setSpacing(0);
    detailsGridRowLayout->addWidget(detailsGridWidget, 0, Qt::AlignLeft | Qt::AlignTop);
    detailsGridRowLayout->addStretch(1);
    detailsTopVBox->addWidget(detailsGridRow, 0);
    detailsTopVBox->addStretch(1);
    detailsTopContainer->setLayout(detailsTopVBox);
    // Poster container (no title), ~60%
    auto *posterContainer = new QWidget(details);
    auto *posterVBox = new QVBoxLayout(posterContainer);
    posterVBox->setSpacing(ui_constants::kSpacingSmall);
    posterVBox->setContentsMargins(ui_constants::kGroupPadding, ui_constants::kGroupPadding, ui_constants::kGroupPadding, ui_constants::kGroupPadding);
    posterVBox->addWidget(posterLabel, 1);
    posterContainer->setLayout(posterVBox);
    detailsLayout->addWidget(detailsTopContainer, /*stretch*/ 2);
    detailsLayout->addWidget(posterContainer, /*stretch*/ 6);
    // Plot container (~20%) with a left-aligned heading
    auto *plotContainer = new QWidget(details);
    auto *plotVBox = new QVBoxLayout(plotContainer);
    plotVBox->setSpacing(ui_constants::kSpacingSmall);
    plotVBox->setContentsMargins(ui_constants::kGroupPadding, ui_constants::kGroupPadding, ui_constants::kGroupPadding, ui_constants::kGroupPadding);
    auto *plotHeading = new QLabel(QString::fromUtf8(ui_strings::kGroupPlot), plotContainer);
    QFont phf = plotHeading->font(); phf.setBold(true); plotHeading->setFont(phf);
    plotHeading->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    plotVBox->addWidget(plotHeading, 0, Qt::AlignLeft);
    plotVBox->addWidget(plotView, 1);
    plotContainer->setLayout(plotVBox);
    detailsLayout->addWidget(plotContainer, /*stretch*/ 2);
    details->setLayout(detailsLayout);

    splitter->addWidget(leftFrame);
    splitter->addWidget(details);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    vbox->addWidget(splitter);
    central->setLayout(vbox);
    win.setCentralWidget(central);

    // Network for poster loading
    auto *nam = new QNetworkAccessManager(&win);

    // detailsSizer will be set up later; capture by pointer via outer variable
    static DetailsResizer* gDetailsSizer = nullptr;
    auto updateDetails = [listView, model, titleLabel, posterLabel, plotView, nam, details, directorValue, actorsValue, genresValue, runtimeValue, imdbLink, posterContainer]() {
        QModelIndex idx = listView->currentIndex();
        if (!idx.isValid()) {
            titleLabel->clear();
            posterLabel->clear();
            plotView->clear();
            directorValue->clear();
            actorsValue->clear();
            genresValue->clear();
            runtimeValue->clear();
            imdbLink->clear();
            return;
        }
        int row = idx.row();
        QVariantMap m = model->get(row);
        const QString t = m.value("title").toString();
        const int y = m.value("year").toInt();
        QString head = t;
        if (y > 0) head += QString(" (%1)").arg(y);
        titleLabel->setText(head);
        plotView->setPlainText(m.value("plotFull").toString());
        // Details values
        directorValue->setText(m.value("director").toString());
        {
            QStringList actors;
            for (const auto& v : m.value("actors").toList()) actors << v.toString();
            QString fullA = actors.join(", ");
            actorsValue->setProperty("fullActors", fullA);
            if (gDetailsSizer) gDetailsSizer->setFullActors(fullA);
        }
        {
            QStringList genres;
            for (const auto& v : m.value("genres").toList()) genres << v.toString();
            genresValue->setText(genres.join(", "));
        }
        int runtime = m.value("runtimeMinutes").toInt();
        runtimeValue->setText(runtime > 0 ? QString::number(runtime) + QStringLiteral(" min") : QString());
        const QString imdb = m.value("imdbID").toString();
        if (!imdb.isEmpty()) {
            const QString url = QStringLiteral("https://www.imdb.com/title/") + imdb + QStringLiteral("/");
            imdbLink->setText(QStringLiteral("<a href=\"") + url + QStringLiteral("\">Open</a>"));
        } else {
            imdbLink->clear();
        }
        // Clear poster and stored original
        posterLabel->setPixmap(QPixmap());
        posterLabel->setProperty("origPm", QVariant());
        const QString url = m.value("posterUrl").toString();
        auto fetchAndShow = [nam, posterLabel, details, posterContainer](const QString& posterUrl) {
            if (posterUrl.isEmpty() || posterUrl == "N/A") return;
            QNetworkRequest req{QUrl(posterUrl)};
            req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Top100/1.0 (Qt)"));
            req.setRawHeader("Accept", "image/*, */*;q=0.8");
            QNetworkReply *reply = nam->get(req);
            QObject::connect(reply, &QNetworkReply::finished, posterLabel, [reply, posterLabel, details, posterContainer]() {
                reply->deleteLater();
                if (reply->error() == QNetworkReply::NoError) {
                    QByteArray data = reply->readAll();
                    QPixmap pm;
                    if (pm.loadFromData(data)) {
                        // Store original for future resizes and scale to current container
                        posterLabel->setProperty("origPm", pm);
                        int maxW = posterContainer->width() > 0 ? int(posterContainer->width() * ui_constants::kPosterMaxWidthRatio) : 400;
                        int maxH = posterContainer->height() > 0 ? int(posterContainer->height() * ui_constants::kPosterMaxHeightRatio) : 600;
                        QPixmap scaled = pm.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        posterLabel->setPixmap(scaled);
                    } else {
                        qWarning() << "Poster image decode failed (bytes):" << data.size() << "from" << reply->url();
                    }
                } else {
                    qWarning() << "Poster download error:" << reply->errorString() << "code" << reply->error() << "url" << reply->url();
                }
            });
        };

        if (!url.isEmpty() && url != "N/A") {
            fetchAndShow(url);
        } else {
            // Fallback: try to fetch poster URL via OMDb using imdbID
            const QString imdb = m.value("imdbID").toString();
            if (!imdb.isEmpty()) {
                auto *watcher = new QFutureWatcher<QString>(posterLabel);
                QObject::connect(watcher, &QFutureWatcher<QString>::finished, posterLabel, [watcher, fetchAndShow]() mutable {
                    QString poster = watcher->result();
                    watcher->deleteLater();
                    fetchAndShow(poster);
                });
                QFuture<QString> fut = QtConcurrent::run([imdb]() -> QString {
                    try {
                        AppConfig cfg = loadConfig();
                        if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) return QString();
                        auto maybe = omdbGetById(cfg.omdbApiKey, imdb.toStdString());
                        if (!maybe) return QString();
                        return QString::fromStdString(maybe->posterUrl);
                    } catch (...) {
                        return QString();
                    }
                });
                watcher->setFuture(fut);
            }
        }
    };
    QObject::connect(listView->selectionModel(), &QItemSelectionModel::currentChanged, &win, [=](const QModelIndex&, const QModelIndex&) { updateDetails(); });
    // Also refresh details after a reload, in case selection index remains the same
    QObject::connect(model, &Top100ListModel::reloadCompleted, &win, [updateDetails]() { updateDetails(); });
    QObject::connect(model, &Top100ListModel::requestSelectRow, &win, [model, listView](int row){
        if (row >= 0 && row < model->rowCount()) listView->setCurrentIndex(model->index(row, 0));
    });
    // Select first item initially
    if (model->rowCount() > 0) {
        listView->setCurrentIndex(model->index(0, 0));
        updateDetails();
    }

    // Toolbar with actions: Add, Delete, Refresh, Post to BlueSky, Post to Mastodon
    QToolBar *toolbar = new QToolBar(&win);
    win.addToolBar(Qt::TopToolBarArea, toolbar);
    toolbar->setIconSize(QSize(18, 18));
    QAction *addAct = toolbar->addAction(QIcon::fromTheme("list-add"), QStringLiteral("Add"));
    QAction *delAct = toolbar->addAction(QIcon::fromTheme("edit-delete"), QStringLiteral("Delete"));
    QAction *refreshTbAct = toolbar->addAction(QIcon::fromTheme("view-refresh"), QStringLiteral("Refresh"));
    QAction *postBskyAct = toolbar->addAction(QIcon::fromTheme("cloud-upload"), QStringLiteral("Post BlueSky"));
    QAction *postMastoAct = toolbar->addAction(QIcon::fromTheme("mail-send"), QStringLiteral("Post Mastodon"));
    QAction *updateAct = toolbar->addAction(QIcon::fromTheme("view-refresh"), QStringLiteral("Update (OMDb)"));

    // Bind sort combobox to model and preserve selection on reload
    // Initialize combo selection from model
    sortCombo->setCurrentIndex(sortCombo->findData(model->sortOrder()));
    QObject::connect(sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), &win, [model, sortCombo, listView]() {
        // Capture current imdbID to re-select after reload
        int row = listView->currentIndex().row();
        QString imdb;
        if (row >= 0) imdb = model->get(row).value("imdbID").toString();
        int order = sortCombo->currentData().toInt();
        QMetaObject::Connection conn;
        conn = QObject::connect(model, &Top100ListModel::reloadCompleted, listView, [model, listView, imdb, &conn]() {
            // Find row with same imdbID
            for (int i = 0; i < model->rowCount(); ++i) {
                if (model->get(i).value("imdbID").toString() == imdb) {
                    listView->setCurrentIndex(model->index(i, 0));
                    break;
                }
            }
            QObject::disconnect(conn);
        });
        model->setSortOrder(order);
    });

    QObject::connect(addAct, &QAction::triggered, &win, [&win, model]() {
        // Prompt for OMDb search or manual
        QMessageBox::StandardButton choice = QMessageBox::question(
            &win,
            QStringLiteral("Add Movie"),
            QStringLiteral("Search OMDb for a movie?\nChoose No to enter manually (not wired yet)."),
            QMessageBox::Yes|QMessageBox::No,
            QMessageBox::Yes);
        if (choice == QMessageBox::No) {
            QMessageBox::information(&win, QStringLiteral("Manual Entry"), QStringLiteral("Manual entry is not wired yet."));
            return;
        }
        bool ok = false;
        QString query = QInputDialog::getText(&win, QStringLiteral("OMDb Search"), QStringLiteral("Title keyword:"), QLineEdit::Normal, QString(), &ok);
        if (!ok || query.trimmed().isEmpty()) return;
        // Fetch results using simple blocking call via OMDb search API from lib
        try {
            AppConfig cfg = loadConfig();
            if (!cfg.omdbEnabled || cfg.omdbApiKey.empty()) {
                QMessageBox::warning(&win, QStringLiteral("OMDb"), QStringLiteral("OMDb is not enabled or API key missing in config."));
                return;
            }
            auto results = omdbSearch(cfg.omdbApiKey, query.toStdString());
            if (results.empty()) {
                QMessageBox::information(&win, QStringLiteral("OMDb"), QStringLiteral("No results."));
                return;
            }
            // Build a selection list
            QStringList items;
            for (const auto& r : results) {
                items << QString::fromStdString(r.title + " (" + std::to_string(r.year) + ") [" + r.imdbID + "]");
            }
            bool okSel = false;
            QString sel = QInputDialog::getItem(&win, QStringLiteral("Select Movie"), QStringLiteral("Results:"), items, 0, false, &okSel);
            if (!okSel) return;
            // Extract imdbID between [..]
            int l = sel.lastIndexOf('[');
            int r = sel.lastIndexOf(']');
            if (l >= 0 && r > l) {
                QString imdb = sel.mid(l+1, r-l-1);
                if (model->addMovieByImdbId(imdb)) {
                    QMessageBox::information(&win, QStringLiteral("Added"), QStringLiteral("Movie added."));
                    // Select the last row
                    int last = model->rowCount() - 1;
                    if (last >= 0) {
                        win.statusBar()->showMessage(QStringLiteral("Added movie."), 3000);
                    }
                } else {
                    QMessageBox::warning(&win, QStringLiteral("Add Failed"), QStringLiteral("Could not add movie."));
                }
            }
        } catch (...) {
            QMessageBox::warning(&win, QStringLiteral("Error"), QStringLiteral("Unexpected error during OMDb search."));
        }
    });
    QObject::connect(delAct, &QAction::triggered, &win, [&win, model, listView]() {
        int row = listView->currentIndex().row();
        if (row < 0) { QMessageBox::warning(&win, QStringLiteral("Delete"), QStringLiteral("Select a movie first.")); return; }
        auto m = model->get(row);
        QString imdb = m.value("imdbID").toString();
        if (imdb.isEmpty()) {
            // If imdbID not exposed, delete by title as fallback
            QString title = m.value("title").toString();
            QMessageBox::warning(&win, QStringLiteral("Delete"), QStringLiteral("IMDb ID not available; delete not supported yet."));
            return;
        }
        if (QMessageBox::question(&win, QStringLiteral("Delete"), QStringLiteral("Delete this movie?")) != QMessageBox::Yes) return;
        if (model->deleteByImdbId(imdb)) {
            win.statusBar()->showMessage(QStringLiteral("Deleted."), 3000);
        } else {
            QMessageBox::warning(&win, QStringLiteral("Delete"), QStringLiteral("Failed to delete."));
        }
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

    QObject::connect(updateAct, &QAction::triggered, &win, [model, listView, &win]() {
        int row = listView->currentIndex().row();
        if (row < 0) { QMessageBox::warning(&win, QStringLiteral("Update"), QStringLiteral("Select a movie first.")); return; }
        QString imdb = model->get(row).value("imdbID").toString();
        if (imdb.isEmpty()) { QMessageBox::warning(&win, QStringLiteral("Update"), QStringLiteral("No IMDb ID for this movie.")); return; }
        win.statusBar()->showMessage(QStringLiteral("Updating from OMDb..."), 2000);
        if (model->updateFromOmdbByImdbId(imdb)) {
            QMetaObject::Connection conn;
            conn = QObject::connect(model, &Top100ListModel::reloadCompleted, &win, [&win, &conn]() {
                win.statusBar()->showMessage(QStringLiteral("Updated from OMDb."), 3000);
                QObject::disconnect(conn);
            });
        } else {
            QMessageBox::warning(&win, QStringLiteral("Update"), QStringLiteral("Update failed."));
        }
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

    // Top menu: Sort
    QMenu *sortTopMenu = menuBar->addMenu(QStringLiteral("Sort"));
    sortTopMenu->addAction(QString::fromUtf8(ui_strings::kSortInsertion), &win, [model]() { model->setSortOrder(static_cast<int>(SortOrder::DEFAULT)); });
    sortTopMenu->addAction(QString::fromUtf8(ui_strings::kSortByYear), &win, [model]() { model->setSortOrder(static_cast<int>(SortOrder::BY_YEAR)); });
    sortTopMenu->addAction(QString::fromUtf8(ui_strings::kSortAlpha), &win, [model]() { model->setSortOrder(static_cast<int>(SortOrder::ALPHABETICAL)); });
    sortTopMenu->addAction(QString::fromUtf8(ui_strings::kSortByRank), &win, [model]() { model->setSortOrder(static_cast<int>(SortOrder::BY_USER_RANK)); });
    sortTopMenu->addAction(QString::fromUtf8(ui_strings::kSortByScore), &win, [model]() { model->setSortOrder(static_cast<int>(SortOrder::BY_USER_SCORE)); });

    QMenu *helpMenu = menuBar->addMenu(QString::fromUtf8(ui_strings::kMenuHelp));
    QAction *aboutAct = helpMenu->addAction(QString::fromUtf8(ui_strings::kActionAbout));
    QObject::connect(aboutAct, &QAction::triggered, &win, [&win]() {
        QMessageBox::about(&win,
                           QString::fromUtf8(ui_strings::kActionAbout),
                           QString::fromUtf8(ui_strings::kAboutDialogText));
    });

    // Resize to 120% of initial size, clamped to screen's available geometry
    QScreen* scr = QGuiApplication::primaryScreen();
    QRect avail = scr ? scr->availableGeometry() : QRect(0,0,1920,1080);
    int w = int(ui_constants::kInitialWidth * 1.2);
    int h = int(ui_constants::kInitialHeight * 1.2);
    w = qMin(w, avail.width());
    h = qMin(h, avail.height());
    win.resize(w, h);
    // Install poster resize helper after widgets are constructed
    auto *resizer = new PosterResizer(posterContainer, posterLabel);
    posterContainer->installEventFilter(resizer);
    // Install details resizer to keep actors wrapping relative to width
    auto *detailsSizer = new DetailsResizer(details, actorsValue);
    gDetailsSizer = detailsSizer;
    details->installEventFilter(detailsSizer);
    // Apply initial elide if text already set from first selection
    {
        QString full = actorsValue->property("fullActors").toString();
        if (!full.isEmpty()) detailsSizer->setFullActors(full);
    }
    QObject::connect(model, &Top100ListModel::reloadCompleted, &win, [detailsSizer, actorsValue]() {
        QString full = actorsValue->property("fullActors").toString();
        if (!full.isEmpty()) detailsSizer->setFullActors(full);
    });
    win.show();

    return app.exec();
}
