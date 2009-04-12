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
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kglobal.h>


static const char description[] =
	I18N_NOOP("Tactical one or two player game");

int main(int argc, char *argv[])
{
    KAboutData aboutData( "kjumpingcube", 0, ki18n("KJumpingCube"),
                          KJC_VERSION, ki18n(description), KAboutData::License_GPL,
                          ki18n("(c) 1998-2000, Matthias Kiefer"), KLocalizedString(),
                          "http://games.kde.org/kjumpingcube" );
    aboutData.addAuthor(ki18n("Matthias Kiefer"),KLocalizedString(), "matthias.kiefer@gmx.de");
    aboutData.addAuthor(ki18n("Benjamin Meyer"),ki18n("Various improvements"), "ben+kjumpingcube@meyerhome.net");
    aboutData.addCredit(ki18n("Ian Wadham"),
                      ki18n("Upgrade to KDE4 and SVG artwork support."),
                      "ianw2@optusnet.com.au");
    aboutData.addCredit(ki18n("Eugene Trounev"),
                      ki18n("Graphics for KDE 4.0 version."),
                      "irs_me@hotmail.com");
    KCmdLineArgs::init( argc, argv, &aboutData );

    KApplication application;
    KGlobal::locale()->insertCatalog("libkdegames");

    // All session management is handled in the RESTORE macro
	if (application.isSessionRestored())
        RESTORE(KJumpingCube)
    else {
		KJumpingCube *kjumpingcube = new KJumpingCube;
		kjumpingcube->show();
	}
   return application.exec();
}
