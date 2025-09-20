// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// File: ui/qt/toolbar.cpp
// Purpose: Toolbar construction for Qt window.
//-------------------------------------------------------------------------------
#include "toolbar.h"

#include <QToolBar>
#include <QAction>
#include <QIcon>

void Top100QtWindow::buildToolbar() {
	toolbar_ = new QToolBar(this);
	addToolBar(Qt::TopToolBarArea, toolbar_);
	toolbar_->setIconSize(QSize(18, 18));
	addAct_ = toolbar_->addAction(QIcon::fromTheme("list-add"), QStringLiteral("Add"));
	delAct_ = toolbar_->addAction(QIcon::fromTheme("edit-delete"), QStringLiteral("Delete"));
	refreshTbAct_ = toolbar_->addAction(QIcon::fromTheme("view-refresh"), QStringLiteral("Refresh"));
	postBskyAct_ = toolbar_->addAction(QIcon::fromTheme("cloud-upload"), QStringLiteral("Post BlueSky"));
	postMastoAct_ = toolbar_->addAction(QIcon::fromTheme("mail-send"), QStringLiteral("Post Mastodon"));
	updateAct_ = toolbar_->addAction(QIcon::fromTheme("view-refresh"), QStringLiteral("Update (OMDb)"));

	// Wire actions
	connect(addAct_, &QAction::triggered, this, [this]() { onAddMovie(); });
	connect(delAct_, &QAction::triggered, this, [this]() { onDeleteCurrent(); });
	connect(refreshTbAct_, &QAction::triggered, this, [this]() { onRefresh(); });
	connect(postBskyAct_, &QAction::triggered, this, [this]() { onPostBlueSky(); });
	connect(postMastoAct_, &QAction::triggered, this, [this]() { onPostMastodon(); });
	connect(updateAct_, &QAction::triggered, this, [this]() { onUpdateFromOmdb(); });
}
