// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/rankdialog.cpp
// Purpose: Implementation of Qt Widgets ranking dialog (pairwise Elo).
//-------------------------------------------------------------------------------
#include "rankdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QKeyEvent>
#include <QPixmap>
#include <QFont>
#include <QRandomGenerator>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QCursor>

#include "../common/Top100ListModel.h"
#include "../common/constants.h"

using namespace ui_constants;

static QString joinList(const QVariantList& v, const QString& sep) {
    QStringList out; out.reserve(v.size());
    for (const auto& it : v) out << it.toString();
    return out.join(sep);
}

Top100QtRankDialog::Top100QtRankDialog(QWidget* parent, Top100ListModel* model)
    : QDialog(parent), model_(model)
{
    setWindowTitle(QString()); // no title bar text; use centered heading
    setModal(true);
    resize(900, 600);
    setFixedSize(900, 600); // fixed-size and not resizable

    auto* vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(kGroupPadding*2, kGroupPadding*2, kGroupPadding*2, kGroupPadding*2);
    vbox->setSpacing(kSpacingLarge);

    heading_ = new QLabel(tr("Rank movies"), this);
    QFont hf = heading_->font(); hf.setBold(true); heading_->setFont(hf);
    heading_->setAlignment(Qt::AlignHCenter);
    vbox->addWidget(heading_);

    prompt_ = new QLabel(tr("Which movie do you prefer?"), this);
    prompt_->setAlignment(Qt::AlignHCenter);
    vbox->addWidget(prompt_);

    auto* row = new QWidget(this);
    auto* rowLayout = new QHBoxLayout(row);
    rowLayout->setSpacing(kSpacingLarge);

    // Left pane
    leftPane_ = new QWidget(row);
    auto* leftLayout = new QVBoxLayout(leftPane_);
    leftTitle_ = new QLabel(leftPane_);
    { QFont f = leftTitle_->font(); f.setBold(true); leftTitle_->setFont(f); }
    leftDetails_ = new QLabel(leftPane_);
    leftDetails_->setWordWrap(true);
    leftPoster_ = new QLabel(leftPane_);
    leftPoster_->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    leftLayout->addWidget(leftTitle_);
    leftLayout->addWidget(leftDetails_);
    leftLayout->addWidget(leftPoster_, 1);
    leftPane_->setLayout(leftLayout);

    // Right pane
    rightPane_ = new QWidget(row);
    auto* rightLayout = new QVBoxLayout(rightPane_);
    rightTitle_ = new QLabel(rightPane_);
    { QFont f = rightTitle_->font(); f.setBold(true); rightTitle_->setFont(f); }
    rightDetails_ = new QLabel(rightPane_);
    rightDetails_->setWordWrap(true);
    rightPoster_ = new QLabel(rightPane_);
    rightPoster_->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    rightLayout->addWidget(rightTitle_);
    rightLayout->addWidget(rightDetails_);
    rightLayout->addWidget(rightPoster_, 1);
    rightPane_->setLayout(rightLayout);

    rowLayout->addWidget(leftPane_, 1);
    rowLayout->addWidget(rightPane_, 1);
    row->setLayout(rowLayout);
    vbox->addWidget(row, 1);

    // Bottom actions, right-aligned
    auto* bottom = new QWidget(this);
    auto* bottomLayout = new QHBoxLayout(bottom);
    bottomLayout->addStretch(1);
    passBtn_ = new QPushButton(tr("Pass"), bottom);
    finishBtn_ = new QPushButton(tr("Finish Ranking"), bottom);
    bottomLayout->addWidget(passBtn_);
    bottomLayout->addWidget(finishBtn_);
    bottom->setLayout(bottomLayout);
    vbox->addWidget(bottom);

    // Interactions
    leftPane_->setCursor(Qt::PointingHandCursor);
    rightPane_->setCursor(Qt::PointingHandCursor);
    leftPane_->installEventFilter(this);
    rightPane_->installEventFilter(this);

    connect(passBtn_, &QPushButton::clicked, this, [this]() { passPair(); });
    connect(finishBtn_, &QPushButton::clicked, this, [this]() { accept(); });

    // Mouse click to choose
    leftPane_->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    rightPane_->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    leftPane_->installEventFilter(this);
    rightPane_->installEventFilter(this);

    pickTwo();
}

void Top100QtRankDialog::pickTwo() {
    int n = model_ ? model_->rowCount() : 0;
    if (n < 2) { leftRow_ = rightRow_ = -1; return; }
    int a = static_cast<int>(QRandomGenerator::global()->bounded(n));
    int b = static_cast<int>(QRandomGenerator::global()->bounded(n));
    if (b == a) b = (a + 1) % n;
    leftRow_ = a; rightRow_ = b;
    refreshSide(true);
    refreshSide(false);
}

void Top100QtRankDialog::refreshSide(bool left) {
    int row = left ? leftRow_ : rightRow_;
    if (row < 0) return;
    QVariantMap m = model_->get(row);
    const QString title = m.value("title").toString();
    const int year = m.value("year").toInt();
    const QString head = title + (year > 0 ? QString(" (%1)").arg(year) : QString());
    const QString director = m.value("director").toString();
    const QString actors = joinList(m.value("actors").toList(), ", ");
    const QString genres = joinList(m.value("genres").toList(), ", ");
    const QString runtime = m.value("runtimeMinutes").toInt() > 0 ? QString::number(m.value("runtimeMinutes").toInt()) + " min" : QString();
    QString det = QString("<b>Director:</b> %1<br><b>Actors:</b> %2<br><b>Genres:</b> %3<br><b>Runtime:</b> %4")
                    .arg(director.toHtmlEscaped())
                    .arg(actors.toHtmlEscaped())
                    .arg(genres.toHtmlEscaped())
                    .arg(runtime.toHtmlEscaped());
    QLabel* t = left ? leftTitle_ : rightTitle_;
    QLabel* d = left ? leftDetails_ : rightDetails_;
    QLabel* p = left ? leftPoster_ : rightPoster_;
    t->setText(head);
    d->setText(det);
    p->clear();
    const QString posterUrl = m.value("posterUrl").toString();
    if (!posterUrl.isEmpty()) {
        fetchPoster(p, left ? leftPane_ : rightPane_, posterUrl);
    }
}

void Top100QtRankDialog::chooseLeft() {
    if (!model_ || leftRow_ < 0 || rightRow_ < 0) return;
    model_->recordPairwiseResult(leftRow_, rightRow_, 1);
    pickTwo();
}

void Top100QtRankDialog::chooseRight() {
    if (!model_ || leftRow_ < 0 || rightRow_ < 0) return;
    model_->recordPairwiseResult(leftRow_, rightRow_, 0);
    pickTwo();
}

void Top100QtRankDialog::passPair() { pickTwo(); }

void Top100QtRankDialog::keyPressEvent(QKeyEvent* ev) {
    switch (ev->key()) {
        case Qt::Key_Left: chooseLeft(); return;
        case Qt::Key_Right: chooseRight(); return;
        case Qt::Key_Down:
        case Qt::Key_Enter:
        case Qt::Key_Return: passPair(); return;
        default: break;
    }
    QDialog::keyPressEvent(ev);
}

bool Top100QtRankDialog::eventFilter(QObject* obj, QEvent* ev) {
    if (ev->type() == QEvent::MouseButtonRelease) {
        if (obj == leftPane_) { chooseLeft(); return true; }
        if (obj == rightPane_) { chooseRight(); return true; }
    }
    if (ev->type() == QEvent::Resize) {
        QWidget* container = qobject_cast<QWidget*>(obj);
        if (container == leftPane_ || container == rightPane_) {
            QLabel* target = (container == leftPane_) ? leftPoster_ : rightPoster_;
            QVariant v = target->property("origPm");
            if (v.isValid()) {
                QPixmap pm = v.value<QPixmap>();
                int maxW = qMax(1, int(container->width() * kPosterMaxWidthRatio));
                int maxH = qMax(1, int(container->height() * kPosterMaxHeightRatio));
                target->setPixmap(pm.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }
    }
    return QDialog::eventFilter(obj, ev);
}

void Top100QtRankDialog::fetchPoster(QLabel* target, QWidget* container, const QString& url) {
    if (!nam_) nam_ = new QNetworkAccessManager(this);
    QNetworkRequest req{QUrl(url)};
    QNetworkReply* reply = nam_->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, target, container]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) return;
        const QByteArray data = reply->readAll();
        QPixmap pm; if (!pm.loadFromData(data)) return;
        target->setProperty("origPm", pm);
        int maxW = qMax(1, int(container->width() * kPosterMaxWidthRatio));
        int maxH = qMax(1, int(container->height() * kPosterMaxHeightRatio));
        QPixmap scaled = pm.scaled(maxW, maxH, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        target->setPixmap(scaled);
    });
    // Resizing handled in eventFilter for left/right panes
}
