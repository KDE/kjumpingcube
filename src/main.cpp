/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kjumpingcube_version.h"
#include "kjumpingcube.h"

#include <KAboutData>
#include <KCrash>
#include <QApplication>
#include <KLocalizedString>
#include <KDBusService>
#include <QCommandLineParser>
#include <Kdelibs4ConfigMigrator>


int main(int argc, char *argv[])
{
    // Fixes blurry icons with fractional scaling
    QGuiApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);


    Kdelibs4ConfigMigrator migrate(QStringLiteral("kjumpingcube"));
    migrate.setConfigFiles(QStringList() << QStringLiteral("kjumpingcuberc"));
    migrate.setUiFiles(QStringList() << QStringLiteral("kjumpingcubeui.rc"));
    migrate.migrate();
    KLocalizedString::setApplicationDomain("kjumpingcube");

    KAboutData aboutData( QStringLiteral("kjumpingcube"), i18n("KJumpingCube"),
                          QStringLiteral(KJUMPINGCUBE_VERSION_STRING),
                          i18n("Tactical one or two player game"),
                          KAboutLicense::GPL,
                          i18n("(c) 1998-2000, Matthias Kiefer"));
    aboutData.setOrganizationDomain(QByteArray("kde.org"));
    aboutData.addAuthor(i18n("Matthias Kiefer"),QString(), QStringLiteral("matthias.kiefer@gmx.de"));
    aboutData.addAuthor(i18n("Benjamin Meyer"),i18n("Various improvements"), QStringLiteral("ben+kjumpingcube@meyerhome.net"));
    aboutData.addCredit(i18n("Ian Wadham"),
                      i18n("Upgrade to KDE4 and SVG artwork support."),
                      QStringLiteral("iandw.au@gmail.com"));
    aboutData.addCredit(i18n("Eugene Trounev"),
                      i18n("Graphics for KDE 4.0 version."),
                      QStringLiteral("irs_me@hotmail.com"));
    aboutData.setHomepage(QStringLiteral("https://kde.org/applications/games/org.kde.kjumpingcube"));

    QCommandLineParser parser;
    KAboutData::setApplicationData(aboutData);
    KCrash::initialize();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);
    KDBusService service;

    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kjumpingcube")));

    // All session management is handled in the RESTORE macro
    if (app.isSessionRestored()) {
        kRestoreMainWindows<KJumpingCube>();
    }
    else {
        KJumpingCube *kjumpingcube = new KJumpingCube;
        kjumpingcube->show();
    }
    return app.exec();
}
