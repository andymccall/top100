// SPDX-License-Identifier: Apache-2.0
//-------------------------------------------------------------------------------
// Top100 — Your Personal Movie List
//
// File: ui/android/main.cpp
// Purpose: Android (Qt Quick) UI bootstrap bridging core model to QML.
// Language: C++17
//-------------------------------------------------------------------------------
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include "../common/Top100ListModel.h"
#include "../common/strings.h"
#include "../common/constants.h"
#include "../../lib/config.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName(ui_strings::kAppName);
    app.setOrganizationName("andymccall");
    app.setApplicationDisplayName(ui_strings::kAppName);

    QQmlApplicationEngine engine;

    // Expose common UI text constants
    engine.rootContext()->setContextProperty("UiStrings_MenuFile", QString::fromUtf8(ui_strings::kMenuFile));
    engine.rootContext()->setContextProperty("UiStrings_MenuHelp", QString::fromUtf8(ui_strings::kMenuHelp));
    engine.rootContext()->setContextProperty("UiStrings_ActionQuit", QString::fromUtf8(ui_strings::kActionQuit));
    engine.rootContext()->setContextProperty("UiStrings_ActionAbout", QString::fromUtf8(ui_strings::kActionAbout));
    engine.rootContext()->setContextProperty("UiStrings_AboutText", QString::fromUtf8(ui_strings::kAboutDialogText));
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
    engine.rootContext()->setContextProperty("UiStrings_SortInsertion", QString::fromUtf8(ui_strings::kSortInsertion));
    engine.rootContext()->setContextProperty("UiStrings_SortByYear", QString::fromUtf8(ui_strings::kSortByYear));
    engine.rootContext()->setContextProperty("UiStrings_SortAlpha", QString::fromUtf8(ui_strings::kSortAlpha));
    engine.rootContext()->setContextProperty("UiStrings_SortByRank", QString::fromUtf8(ui_strings::kSortByRank));
    engine.rootContext()->setContextProperty("UiStrings_SortByScore", QString::fromUtf8(ui_strings::kSortByScore));

    // Layout constants
    engine.rootContext()->setContextProperty("Ui_InitWidth", ui_constants::kInitialWidth);
    engine.rootContext()->setContextProperty("Ui_InitHeight", ui_constants::kInitialHeight);
    engine.rootContext()->setContextProperty("Ui_PosterMaxWidthRatio", ui_constants::kPosterMaxWidthRatio);
    engine.rootContext()->setContextProperty("Ui_PosterMaxHeightRatio", ui_constants::kPosterMaxHeightRatio);
    engine.rootContext()->setContextProperty("Ui_LabelMinWidth", ui_constants::kLabelMinWidth);

    // Shared list model
    Top100ListModel model;
    model.reload();
    engine.rootContext()->setContextProperty("top100Model", &model);

    const QUrl url(QStringLiteral("qrc:/android/qml/AndroidMain.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated, &app,
                     [url](QObject* obj, const QUrl& objUrl){ if (!obj && objUrl == url) QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
