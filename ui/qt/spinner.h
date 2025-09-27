// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/spinner.h
// Purpose: Lightweight indeterminate spinner widget for Qt Widgets UIs.
//-------------------------------------------------------------------------------
#pragma once

#include <QWidget>
#include <QColor>

class QTimer;

class SpinnerWidget : public QWidget {
    Q_OBJECT
public:
    explicit SpinnerWidget(QWidget* parent = nullptr);

    void start();
    void stop();
    bool isSpinning() const { return spinning_; }

    QSize sizeHint() const override { return QSize(32, 32); }

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QTimer* timer_ = nullptr;
    int angle_ = 0; // 0..359
    bool spinning_ = false;
    int lines_ = 12;
    int lineLength_ = 6;
    int lineWidth_ = 3;
    int innerRadius_ = 6;
    QColor color_ { Qt::gray };
};
