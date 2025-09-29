// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/spinner.cpp
// Purpose: Implementation of SpinnerWidget (indeterminate progress indicator).
//-------------------------------------------------------------------------------
#include "spinner.h"

#include <QPainter>
#include <QTimer>

SpinnerWidget::SpinnerWidget(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_TranslucentBackground, true);
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this]() {
        angle_ = (angle_ + 30) % 360; // 12 steps
        update();
    });
    hide();
}

void SpinnerWidget::start() {
    if (spinning_) return;
    spinning_ = true;
    show();
    timer_->start(80);
}

void SpinnerWidget::stop() {
    if (!spinning_) return;
    spinning_ = false;
    timer_->stop();
    hide();
}

void SpinnerWidget::paintEvent(QPaintEvent*) {
    if (!spinning_) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);
    const int w = width();
    const int h = height();
    const int r = qMin(w, h) / 2 - lineLength_ - 1;
    p.translate(w / 2, h / 2);
    p.rotate(angle_);
    for (int i = 0; i < lines_; ++i) {
        QColor c = color_;
        c.setAlphaF(1.0 - (i / static_cast<double>(lines_)));
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(innerRadius_, -lineWidth_ / 2, lineLength_, lineWidth_, 1, 1);
        p.rotate(360.0 / lines_);
    }
}
