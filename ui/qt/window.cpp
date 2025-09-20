// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/qt/window.cpp
// Purpose: Qt Widgets main window layout and wiring.
//-------------------------------------------------------------------------------
#include "window.h"

#include <QApplication>
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
#include <QTextBrowser>
#include <QGridLayout>
#include <QScreen>
#include <QEvent>
#include <QVariant>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QSslSocket>

#include "../common/strings.h"
#include "../common/constants.h"
#include "../common/Top100ListModel.h"

using namespace ui_constants;
using namespace ui_strings;

// Helpers from original file
namespace {
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
                    int maxW = qMax(1, int(container_->width() * kPosterMaxWidthRatio));
                    int maxH = qMax(1, int(container_->height() * kPosterMaxHeightRatio));
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
class DetailsResizer : public QObject {
    QWidget* container_;
    QLabel* actors_;
public:
    DetailsResizer(QWidget* container, QLabel* actors)
        : QObject(container), container_(container), actors_(actors) {
        actors_->setWordWrap(false);
        actors_->setTextInteractionFlags(Qt::TextSelectableByMouse);
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
        int padding = kGroupPadding * 2;
        int labelCol = kLabelMinWidth + (kSpacingSmall + 6);
        int maxW = qMax(100, container_->width() - padding - labelCol);
        actors_->setMaximumWidth(maxW);
        QFontMetrics fm(actors_->font());
        const QString full = actors_->property("fullActors").toString();
        QString elided = fm.elidedText(full, Qt::ElideRight, maxW);
        actors_->setText(elided);
        actors_->setToolTip(full);
    }
};
}

Top100QtWindow::Top100QtWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(kQtWindowTitle);
    qInfo() << "Qt SSL supported:" << QSslSocket::supportsSsl();
    buildLayout();
    buildMenuBar();
    buildToolbar();
    connectSignals();
}

void Top100QtWindow::buildLayout() {
    central_ = new QWidget(this);
    auto *vbox = new QVBoxLayout(central_);
    auto *splitter = new QSplitter(Qt::Horizontal, central_);

    // Left
    auto *leftFrame = new QFrame(splitter);
    leftFrame->setFrameShape(QFrame::StyledPanel);
    auto *leftLayout = new QVBoxLayout(leftFrame);
    leftLayout->setSpacing(kSpacingSmall);
    leftLayout->setContentsMargins(kGroupPadding, kGroupPadding, kGroupPadding, kGroupPadding);
    auto *leftHeading = new QLabel(QString::fromUtf8(kHeadingMovies), leftFrame);
    QFont hf = leftHeading->font(); hf.setBold(true); hf.setPointSize(hf.pointSize() + 2); leftHeading->setFont(hf);
    leftLayout->addWidget(leftHeading);
    auto *sortRow = new QWidget(leftFrame);
    auto *sortRowLayout = new QHBoxLayout(sortRow);
    sortRowLayout->setSpacing(kSpacingSmall);
    sortRowLayout->setContentsMargins(0, 0, 0, 0);
    sortRowLayout->addWidget(new QLabel(QString::fromUtf8(kLabelSortOrder), sortRow));
    sortCombo_ = new QComboBox(sortRow);
    sortCombo_->addItem(QString::fromUtf8(kSortInsertion), 0);
    sortCombo_->addItem(QString::fromUtf8(kSortByYear), 1);
    sortCombo_->addItem(QString::fromUtf8(kSortAlpha), 2);
    sortCombo_->addItem(QString::fromUtf8(kSortByRank), 3);
    sortCombo_->addItem(QString::fromUtf8(kSortByScore), 4);
    sortRowLayout->addWidget(sortCombo_, 1);
    leftLayout->addWidget(sortRow);
    listView_ = new QListView(leftFrame);
    listView_->setSelectionMode(QAbstractItemView::SingleSelection);
    listView_->setSelectionBehavior(QAbstractItemView::SelectRows);
    model_ = new Top100ListModel(this);
    listView_->setModel(model_);
    leftLayout->addWidget(listView_, 1);
    leftFrame->setLayout(leftLayout);

    // Right
    auto *details = new QFrame(splitter);
    details->setFrameShape(QFrame::StyledPanel);
    auto *detailsLayout = new QVBoxLayout(details);
    detailsLayout->setSpacing(kSpacingSmall);
    detailsLayout->setContentsMargins(kGroupPadding, kGroupPadding, kGroupPadding, kGroupPadding);
    auto *detailsHeading = new QLabel(QString::fromUtf8(kHeadingDetails), details);
    QFont df = detailsHeading->font(); df.setBold(true); df.setPointSize(df.pointSize() + 2); detailsHeading->setFont(df);
    detailsLayout->addWidget(detailsHeading);
    titleLabel_ = new QLabel(details);
    titleLabel_->setWordWrap(true);
    titleLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    QFont f = titleLabel_->font(); f.setPointSize(f.pointSize() + 2); f.setBold(true); titleLabel_->setFont(f);

    auto *detailsGridWidget = new QWidget(details);
    auto *detailsGrid = new QGridLayout(detailsGridWidget);
    detailsGrid->setContentsMargins(0, 0, 0, 0);
    detailsGrid->setHorizontalSpacing(kSpacingSmall + 6);
    detailsGrid->setVerticalSpacing(4);
    detailsGrid->setColumnStretch(0, 0);
    detailsGrid->setColumnStretch(1, 0);
    detailsGridWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
    auto makeFieldLabel = [&](const char* text){
        auto *lbl = new QLabel(QString::fromUtf8(text), detailsGridWidget);
        lbl->setMinimumWidth(kLabelMinWidth);
        lbl->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        return lbl;
    };
    directorValue_ = new QLabel(detailsGridWidget); directorValue_->setWordWrap(true); directorValue_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    actorsValue_ = new QLabel(detailsGridWidget); actorsValue_->setWordWrap(false); actorsValue_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    genresValue_ = new QLabel(detailsGridWidget); genresValue_->setWordWrap(true); genresValue_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    runtimeValue_ = new QLabel(detailsGridWidget); runtimeValue_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    imdbLink_ = new QLabel(detailsGridWidget); imdbLink_->setTextFormat(Qt::RichText); imdbLink_->setOpenExternalLinks(true); imdbLink_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    int rowF = 0;
    detailsGrid->addWidget(makeFieldLabel(kFieldDirector), rowF, 0); detailsGrid->addWidget(directorValue_, rowF, 1); ++rowF;
    detailsGrid->addWidget(makeFieldLabel(kFieldActors), rowF, 0); detailsGrid->addWidget(actorsValue_, rowF, 1); ++rowF;
    detailsGrid->addWidget(makeFieldLabel(kFieldGenres), rowF, 0); detailsGrid->addWidget(genresValue_, rowF, 1); ++rowF;
    detailsGrid->addWidget(makeFieldLabel(kFieldRuntime), rowF, 0); detailsGrid->addWidget(runtimeValue_, rowF, 1); ++rowF;
    detailsGrid->addWidget(makeFieldLabel(kFieldImdbPage), rowF, 0); detailsGrid->addWidget(imdbLink_, rowF, 1); ++rowF;

    posterLabel_ = new QLabel(details);
    posterLabel_->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    posterLabel_->setScaledContents(false);
    posterLabel_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    plotView_ = new QTextBrowser(details);
    plotView_->setReadOnly(true);
    plotView_->setOpenExternalLinks(true);
    detailsLayout->addWidget(titleLabel_);
    auto *detailsTopContainer = new QWidget(details);
    detailsContainer_ = details; // for resizer usage
    auto *detailsTopVBox = new QVBoxLayout(detailsTopContainer);
    detailsTopVBox->setSpacing(0);
    detailsTopVBox->setContentsMargins(0, 0, 0, 0);
    auto *detailsGridRow = new QWidget(details);
    auto *detailsGridRowLayout = new QHBoxLayout(detailsGridRow);
    detailsGridRowLayout->setContentsMargins(0, 0, 0, 0);
    detailsGridRowLayout->setSpacing(0);
    detailsGridRowLayout->addWidget(detailsGridWidget, 0, Qt::AlignLeft | Qt::AlignTop);
    detailsGridRowLayout->addStretch(1);
    detailsTopVBox->addWidget(detailsGridRow, 0);
    detailsTopVBox->addStretch(1);
    detailsTopContainer->setLayout(detailsTopVBox);
    posterContainer_ = new QWidget(details);
    auto *posterVBox = new QVBoxLayout(posterContainer_);
    posterVBox->setSpacing(kSpacingSmall);
    posterVBox->setContentsMargins(kGroupPadding, kGroupPadding, kGroupPadding, kGroupPadding);
    posterVBox->addWidget(posterLabel_, 1);
    posterContainer_->setLayout(posterVBox);
    detailsLayout->addWidget(detailsTopContainer, 2);
    detailsLayout->addWidget(posterContainer_, 6);
    auto *plotContainer = new QWidget(details);
    auto *plotVBox = new QVBoxLayout(plotContainer);
    plotVBox->setSpacing(kSpacingSmall);
    plotVBox->setContentsMargins(kGroupPadding, kGroupPadding, kGroupPadding, kGroupPadding);
    auto *plotHeading = new QLabel(QString::fromUtf8(kGroupPlot), plotContainer);
    QFont phf = plotHeading->font(); phf.setBold(true); plotHeading->setFont(phf);
    plotHeading->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    plotVBox->addWidget(plotHeading, 0, Qt::AlignLeft);
    plotVBox->addWidget(plotView_, 1);
    plotContainer->setLayout(plotVBox);
    detailsLayout->addWidget(plotContainer, 2);
    details->setLayout(detailsLayout);

    splitter->addWidget(leftFrame);
    splitter->addWidget(details);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    vbox->addWidget(splitter);
    central_->setLayout(vbox);
    setCentralWidget(central_);

    // Initial sizing
    QScreen* scr = QGuiApplication::primaryScreen();
    QRect avail = scr ? scr->availableGeometry() : QRect(0,0,1920,1080);
    int w = int(kInitialWidth * 1.2);
    int h = int(kInitialHeight * 1.2);
    w = qMin(w, avail.width());
    h = qMin(h, avail.height());
    resize(w, h);
}

// buildMenuBar and buildToolbar moved to menu.cpp and toolbar.cpp

void Top100QtWindow::connectSignals() {
    // Model and selection behavior
    sortCombo_->setCurrentIndex(sortCombo_->findData(model_->sortOrder()));
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() { onSortChanged(); });
    connect(listView_->selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex&, const QModelIndex&) { updateDetails(); });
    connect(model_, &Top100ListModel::reloadCompleted, this, [this]() { updateDetails(); });
    connect(model_, &Top100ListModel::requestSelectRow, this, [this](int row){ if (row >= 0 && row < model_->rowCount()) listView_->setCurrentIndex(model_->index(row, 0)); });
    if (model_->rowCount() > 0) {
        listView_->setCurrentIndex(model_->index(0, 0));
        updateDetails();
    }

    // Network for poster loading and resizers
    nam_ = new QNetworkAccessManager(this);
    posterResizer_ = static_cast<QObject*>(new PosterResizer(posterContainer_, posterLabel_));
    posterContainer_->installEventFilter(posterResizer_);
    detailsResizer_ = static_cast<QObject*>(new DetailsResizer(detailsContainer_, actorsValue_));
    detailsContainer_->installEventFilter(detailsResizer_);
}
