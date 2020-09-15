/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 1998-2000 by Matthias Kiefer
                            <matthias.kiefer@gmx.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**************************************************************************** */
#include "version.h"
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
                          QStringLiteral(KJC_VERSION),
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
