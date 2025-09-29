// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/adddialog.cpp
// Purpose: Qt dialog to search OMDb, preview details, and select a movie to add.
//-------------------------------------------------------------------------------
#include "adddialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSplitter>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QTextBrowser>
#include <QDialogButtonBox>
#include <QScreen>
#include <QGuiApplication>
#include <QItemSelectionModel>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QBuffer>
#include <QPixmap>
#include <QShowEvent>
#include <QResizeEvent>
#include <QtConcurrent>
#include <QFutureWatcher>

#include "../common/constants.h"
#include "../common/Top100ListModel.h"

using namespace ui_constants;

Top100QtAddDialog::Top100QtAddDialog(QWidget* parent, Top100ListModel* model)
    : QDialog(parent), model_(model) {
    setWindowTitle(QStringLiteral("Add Movie (OMDb)"));
    setModal(true);
    auto *root = new QVBoxLayout(this);

    // Search row
    auto *searchRow = new QWidget(this);
    auto *searchLayout = new QHBoxLayout(searchRow);
    searchLayout->setContentsMargins(0,0,0,0);
    searchLayout->setSpacing(kSpacingSmall);
    auto *lbl = new QLabel(QStringLiteral("Search for movie"), searchRow);
    queryEdit_ = new QLineEdit(searchRow);
    searchBtn_ = new QPushButton(QStringLiteral("Search"), searchRow);
    searchBtn_->setDefault(true);
    connect(searchBtn_, &QPushButton::clicked, this, [this]() { doSearch(); });
    connect(queryEdit_, &QLineEdit::returnPressed, this, [this]() { doSearch(); });
    // Connect async model signals
    if (model_) {
        connect(model_, &Top100ListModel::omdbSearchFinished, this, [this](const QVariantList& res){
            resultsModel_->clear();
            for (const auto& v : res) {
                QVariantMap m = v.toMap();
                QString title = m.value("title").toString();
                int year = m.value("year").toInt();
                QString imdb = m.value("imdbID").toString();
                auto *it = new QStandardItem(QString("%1 (%2)").arg(title).arg(year));
                it->setData(imdb, Qt::UserRole + 1);
                it->setEditable(false);
                resultsModel_->appendRow(it);
            }
            if (resultsModel_->rowCount() > 0) resultsView_->setCurrentIndex(resultsModel_->index(0,0));
            searchBtn_->setEnabled(true);
            queryEdit_->setEnabled(true);
        });
        connect(model_, &Top100ListModel::omdbGetFinished, this, [this](const QVariantMap& m){
            QString t = m.value("title").toString();
            int y = m.value("year").toInt();
            titleLbl_->setText(y > 0 ? QString("%1 (%2)").arg(t).arg(y) : t);
            plotView_->setPlainText(!m.value("plotShort").toString().isEmpty() ? m.value("plotShort").toString() : m.value("plotFull").toString());
            QString poster = m.value("posterUrl").toString();
            loadPoster(poster);
        });
        connect(model_, &Top100ListModel::addMovieFinished, this, [this](const QString& imdb, bool ok){
            if (ok) accept(); else addBtn_->setEnabled(true);
        });
    }
    searchLayout->addWidget(lbl);
    searchLayout->addWidget(queryEdit_, 1);
    searchLayout->addWidget(searchBtn_);
    root->addWidget(searchRow);

    // Split results/preview
    auto *split = new QSplitter(Qt::Horizontal, this);
    // Results list
    resultsModel_ = new QStandardItemModel(this);
    resultsView_ = new QListView(split);
    resultsView_->setModel(resultsModel_);
    resultsView_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(resultsView_->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex&, const QModelIndex&){ onResultSelectionChanged(); });

    // Preview panel
    auto *preview = new QWidget(split);
    auto *pv = new QVBoxLayout(preview);
    pv->setContentsMargins(kGroupPadding, kGroupPadding, kGroupPadding, kGroupPadding);
    pv->setSpacing(kSpacingSmall);
    titleLbl_ = new QLabel(preview);
    QFont tf = titleLbl_->font(); tf.setBold(true); titleLbl_->setFont(tf);
    titleLbl_->setWordWrap(true);
    posterLbl_ = new QLabel(preview);
    posterLbl_->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    posterLbl_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    posterSpinner_ = new SpinnerWidget(preview);
    posterSpinner_->hide();
    plotView_ = new QTextBrowser(preview);
    plotView_->setReadOnly(true);
    pv->addWidget(titleLbl_);
    pv->addWidget(posterLbl_, 4);
    pv->addWidget(posterSpinner_, 0, Qt::AlignCenter);
    pv->addWidget(plotView_, 3);

    split->addWidget(resultsView_);
    split->addWidget(preview);
    split->setStretchFactor(0, 2);
    split->setStretchFactor(1, 3);
    root->addWidget(split, 1);

    // Buttons
    auto *btns = new QDialogButtonBox(this);
    auto *manualBtn = btns->addButton(QStringLiteral("Enter manually"), QDialogButtonBox::ActionRole);
    auto *cancelBtn = btns->addButton(QDialogButtonBox::Cancel);
    addBtn_ = btns->addButton(QStringLiteral("Add"), QDialogButtonBox::AcceptRole);
    addBtn_->setEnabled(false);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(addBtn_, &QPushButton::clicked, this, &QDialog::accept);
    connect(manualBtn, &QPushButton::clicked, this, [this]() { this->reject(); });
    root->addWidget(btns);

    nam_ = new QNetworkAccessManager(this);
}

void Top100QtAddDialog::showEvent(QShowEvent* ev) {
    QDialog::showEvent(ev);
    if (parentWidget()) {
        resize(parentWidget()->width() * 0.5, parentWidget()->height() * 0.6);
        move(parentWidget()->geometry().center() - rect().center());
    }
}

void Top100QtAddDialog::resizeEvent(QResizeEvent* ev) {
    QDialog::resizeEvent(ev);
    rescalePoster();
}

void Top100QtAddDialog::doSearch() {
    resultsModel_->clear();
    addBtn_->setEnabled(false);
    selectedImdb_.clear();
    const QString q = queryEdit_->text().trimmed();
    if (q.isEmpty() || !model_) return;
    searchBtn_->setEnabled(false); queryEdit_->setEnabled(false);
    model_->searchOmdbAsync(q);
}

void Top100QtAddDialog::onResultSelectionChanged() {
    QModelIndex idx = resultsView_->currentIndex();
    if (!idx.isValid()) { addBtn_->setEnabled(false); titleLbl_->clear(); plotView_->clear(); posterLbl_->clear(); return; }
    QString imdb = resultsModel_->itemFromIndex(idx)->data(Qt::UserRole + 1).toString();
    selectedImdb_ = imdb;
    addBtn_->setEnabled(true);
    model_->fetchOmdbByIdAsync(imdb);
}

void Top100QtAddDialog::loadPoster(const QString& url) {
    origPoster_ = QPixmap();
    posterLbl_->clear();
    if (posterSpinner_) posterSpinner_->start();
    if (url.isEmpty() || url == "N/A") return;
    QNetworkRequest request{QUrl{url}};
    QNetworkReply* reply = nam_->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) { if (posterSpinner_) posterSpinner_->stop(); return; }
        QByteArray data = reply->readAll();
        QPixmap pm; pm.loadFromData(data);
        if (!pm.isNull()) {
            origPoster_ = pm;
            rescalePoster();
        }
        if (posterSpinner_) posterSpinner_->stop();
    });
}

void Top100QtAddDialog::rescalePoster() {
    if (origPoster_.isNull()) return;
    int maxW = qMax(1, int(width() * kPosterMaxWidthRatio));
    int maxH = qMax(1, int(height() * kPosterMaxHeightRatio));
    QPixmap scaled = origPoster_.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    posterLbl_->setPixmap(scaled);
}
