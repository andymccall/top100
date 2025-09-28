// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/spinner.h
// Purpose: Lightweight indeterminate spinner widget for Qt Widgets UIs.
//-------------------------------------------------------------------------------
#pragma once

#include <QWidget>
#include <QColor>

class QTimer;

/**
 * @brief Lightweight indeterminate spinner widget for busy states.
 */
class SpinnerWidget : public QWidget {
    Q_OBJECT
public:
    /** @brief Create spinner widget.
     *  @param parent Optional parent widget */
    explicit SpinnerWidget(QWidget* parent = nullptr);
    /** @brief Start spinning animation. */
    void start();
    /** @brief Stop spinning animation. */
    void stop();
    /** @return true if the spinner is currently active. */
    bool isSpinning() const { return spinning_; }
    /** @return Suggested default size for layout. */
    QSize sizeHint() const override { return QSize(32, 32); }

protected:
    /**
     * @brief Paint the spinner animation.
     * @param event Paint event data (unused)
     */
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
