// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
/*! \file ui/kde/main.cpp
    \brief KDE Kirigami/QML app bootstrap that wires the shared model and loads Main.qml.
*/
// Top100 — Your Personal Movie List
//
// File: ui/kde/main.cpp
// Purpose: KDE Kirigami/QML app bootstrap and model wiring.
// Language: C++17 (CMake build)
//
// Author: Andy McCall, mailme@andymccall.co.uk
// Date: September 18, 2025
//-------------------------------------------------------------------------------
// Minimal KDE Kirigami/QML app for Top100
//
// Parity with Qt Widgets UI:
// - Left list (Top100ListModel)
// - Right details pane with poster + plot
// - Header toolbar actions (Add/Delete placeholders, Refresh, Post BlueSky/Mastodon)
// - Async posting using model signals; QML shows passive notifications

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <QStringList>
#include "../common/strings.h"
#include "../common/constants.h"

#include "../common/Top100ListModel.h"
// Core library headers
#include "../../lib/config.h"
#include "../../lib/top100.h"
#include "../../lib/Movie.h"

// TODO: include your top100 library header here
// #include "path/to/your/top100/api.h"

// Small adaptor: fetch titles from the top100 library and convert to QStringList.
static QStringList fetchTop100Titles() {
    QStringList titles;
    try {
        AppConfig cfg = loadConfig();
        Top100 list(cfg.dataFile);
        // Prefer ranked order; fall back gracefully to alphabetical by using the API
        auto movies = list.getMovies(SortOrder::BY_USER_RANK);
        if (movies.empty()) {
            movies = list.getMovies(SortOrder::ALPHABETICAL);
        }
        titles.reserve(static_cast<int>(movies.size()));
        for (const auto& m : movies) {
            titles << QString::fromStdString(m.title);
        }
    } catch (const std::exception& e) {
        // Surface minimal error info in the list for visibility
        titles << QStringLiteral("(error loading Top100: %1)").arg(QString::fromUtf8(e.what()));
    }
    return titles;
}

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(ui_strings::kAppName);
    app.setOrganizationName("andymccall");
    app.setApplicationDisplayName(ui_strings::kKdeWindowTitle);

    // Optional: use Breeze icon name if available
    app.setWindowIcon(QIcon::fromTheme("applications-multimedia"));

    QQmlApplicationEngine engine;
    // Expose strings to QML
    engine.rootContext()->setContextProperty("UiStrings_MenuFile", QString::fromUtf8(ui_strings::kMenuFile));
    engine.rootContext()->setContextProperty("UiStrings_MenuHelp", QString::fromUtf8(ui_strings::kMenuHelp));
    engine.rootContext()->setContextProperty("UiStrings_ActionQuit", QString::fromUtf8(ui_strings::kActionQuit));
    engine.rootContext()->setContextProperty("UiStrings_ActionAbout", QString::fromUtf8(ui_strings::kActionAbout));
    engine.rootContext()->setContextProperty("UiStrings_AboutText", QString::fromUtf8(ui_strings::kAboutDialogText));
    // Labels/headings
    engine.rootContext()->setContextProperty("UiStrings_HeadingMovies", QString::fromUtf8(ui_strings::kHeadingMovies));
    engine.rootContext()->setContextProperty("UiStrings_HeadingDetails", QString::fromUtf8(ui_strings::kHeadingDetails));
    engine.rootContext()->setContextProperty("UiStrings_LabelSortOrder", QString::fromUtf8(ui_strings::kLabelSortOrder));
    engine.rootContext()->setContextProperty("UiStrings_GroupPlot", QString::fromUtf8(ui_strings::kGroupPlot));
    engine.rootContext()->setContextProperty("UiStrings_FieldDirector", QString::fromUtf8(ui_strings::kFieldDirector));
    engine.rootContext()->setContextProperty("UiStrings_FieldActors", QString::fromUtf8(ui_strings::kFieldActors));
    engine.rootContext()->setContextProperty("UiStrings_FieldImdbId", QString::fromUtf8(ui_strings::kFieldImdbId));
    engine.rootContext()->setContextProperty("UiStrings_FieldImdbPage", QString::fromUtf8(ui_strings::kFieldImdbPage));
    engine.rootContext()->setContextProperty("UiStrings_FieldGenres", QString::fromUtf8(ui_strings::kFieldGenres));
    engine.rootContext()->setContextProperty("UiStrings_FieldRuntime", QString::fromUtf8(ui_strings::kFieldRuntime));
    // Sort options
    engine.rootContext()->setContextProperty("UiStrings_SortInsertion", QString::fromUtf8(ui_strings::kSortInsertion));
    engine.rootContext()->setContextProperty("UiStrings_SortByYear", QString::fromUtf8(ui_strings::kSortByYear));
    engine.rootContext()->setContextProperty("UiStrings_SortAlpha", QString::fromUtf8(ui_strings::kSortAlpha));
    engine.rootContext()->setContextProperty("UiStrings_SortByRank", QString::fromUtf8(ui_strings::kSortByRank));
    engine.rootContext()->setContextProperty("UiStrings_SortByScore", QString::fromUtf8(ui_strings::kSortByScore));

    // Expose shared UI constants to QML for parity with Qt Widgets
    engine.rootContext()->setContextProperty("Ui_InitWidth", ui_constants::kInitialWidth);
    engine.rootContext()->setContextProperty("Ui_InitHeight", ui_constants::kInitialHeight);
    engine.rootContext()->setContextProperty("Ui_PosterMaxWidthRatio", ui_constants::kPosterMaxWidthRatio);
    engine.rootContext()->setContextProperty("Ui_PosterMaxHeightRatio", ui_constants::kPosterMaxHeightRatio);
    engine.rootContext()->setContextProperty("Ui_LabelMinWidth", ui_constants::kLabelMinWidth);

    // Shared list model exposed to QML
    Top100ListModel model;
    model.reload();
    engine.rootContext()->setContextProperty("top100Model", &model);

    const QUrl url(QStringLiteral("qrc:/qml/Main.qml"));

    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl) {
            QCoreApplication::exit(-1);
        }
    }, Qt::QueuedConnection);

    engine.load(url);

    return app.exec();
}
