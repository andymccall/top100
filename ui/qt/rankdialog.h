// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/rankdialog.h
// Purpose: Qt Widgets dialog for pairwise ranking of movies (Elo-based).
//-------------------------------------------------------------------------------
#pragma once

#include <QDialog>
#include <QVariantMap>
class QNetworkAccessManager;

class QLabel;
class QPushButton;
class QKeyEvent;
class Top100ListModel;

/**
 * @brief Qt dialog for pairwise movie ranking (keyboard friendly: 1/2/P).
 */
class Top100QtRankDialog : public QDialog {
    Q_OBJECT
public:
    /**
     * @brief Pairwise ranking dialog (Elo-based).
     * @param parent Parent widget
     * @param model Shared Top100 model
     */
    explicit Top100QtRankDialog(QWidget* parent, Top100ListModel* model);

protected:
    /**
     * @brief Choose left/right winners via keyboard (1/2/P).
     * @param ev Key event
     */
    void keyPressEvent(QKeyEvent* ev) override;
    /**
     * @brief Intercept events for focus/keyboard handling.
     * @param obj Target object
     * @param ev Event
     * @return true if the event is handled, false to pass it on
     */
    bool eventFilter(QObject* obj, QEvent* ev) override;

private:
    Top100ListModel* model_ { nullptr };
    int leftRow_ { -1 };
    int rightRow_ { -1 };

    QLabel* heading_ { nullptr };
    QLabel* prompt_ { nullptr };
    QLabel* leftTitle_ { nullptr };
    QLabel* leftDetails_ { nullptr };
    QLabel* leftPoster_ { nullptr };
    QWidget* leftPane_ { nullptr };
    QLabel* rightTitle_ { nullptr };
    QLabel* rightDetails_ { nullptr };
    QLabel* rightPoster_ { nullptr };
    QWidget* rightPane_ { nullptr };
    QPushButton* passBtn_ { nullptr };
    QPushButton* finishBtn_ { nullptr };
    QNetworkAccessManager* nam_ { nullptr }; // for poster fetching

    void pickTwo();
    void refreshSide(bool left);
    void chooseLeft();
    void chooseRight();
    void passPair();

    void fetchPoster(QLabel* target, QWidget* container, const QString& url);
};
