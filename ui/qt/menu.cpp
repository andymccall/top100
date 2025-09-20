// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/menu.cpp
// Purpose: Menu construction for Qt window.
//-------------------------------------------------------------------------------
#include "menu.h"

#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QMessageBox>
#include <QComboBox>

#include "../common/strings.h"
#include "../common/constants.h"

using namespace ui_strings;
using namespace ui_constants;

void Top100QtWindow::buildMenuBar() {
	QMenuBar *mb = this->menuBar();
	// File
	QMenu *fileMenu = mb->addMenu(QString::fromUtf8(kMenuFile));
	QAction *refreshAct = fileMenu->addAction(QStringLiteral("Refresh"));
	connect(refreshAct, &QAction::triggered, this, [this]() { onRefresh(); });
	QAction *quitAct = fileMenu->addAction(QString::fromUtf8(kActionQuit));
	connect(quitAct, &QAction::triggered, qApp, &QCoreApplication::quit);

	// Sort
	QMenu *sortTopMenu = mb->addMenu(QStringLiteral("Sort"));
	sortTopMenu->addAction(QString::fromUtf8(kSortInsertion), this, [this]() { sortCombo_->setCurrentIndex(0); onSortChanged(); });
	sortTopMenu->addAction(QString::fromUtf8(kSortByYear), this, [this]() { sortCombo_->setCurrentIndex(1); onSortChanged(); });
	sortTopMenu->addAction(QString::fromUtf8(kSortAlpha), this, [this]() { sortCombo_->setCurrentIndex(2); onSortChanged(); });
	sortTopMenu->addAction(QString::fromUtf8(kSortByRank), this, [this]() { sortCombo_->setCurrentIndex(3); onSortChanged(); });
	sortTopMenu->addAction(QString::fromUtf8(kSortByScore), this, [this]() { sortCombo_->setCurrentIndex(4); onSortChanged(); });

	// Help
	QMenu *helpMenu = mb->addMenu(QString::fromUtf8(kMenuHelp));
	QAction *aboutAct = helpMenu->addAction(QString::fromUtf8(kActionAbout));
	connect(aboutAct, &QAction::triggered, this, [this]() {
		QMessageBox::about(this, QString::fromUtf8(kActionAbout), QString::fromUtf8(kAboutDialogText));
	});
}
