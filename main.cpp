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


static const char description[] =
	I18N_NOOP("Tactical one or two player game");

// A hack to circumvent tricky i18n issue, not used later on in the code.
// Both context and contents must be exactly the same as for the entry in
// kdelibs/kdeui/ui_standards.rc
static const char dummy[] = I18N_NOOP2("Menu title", "&Move");

int main(int argc, char *argv[])
{
    KAboutData aboutData( "kjumpingcube", I18N_NOOP("KJumpingCube"),
                          KJC_VERSION, description, KAboutData::License_GPL,
                          "(c) 1998-2000, Matthias Kiefer");
    aboutData.addAuthor("Matthias Kiefer",0, "matthias.kiefer@gmx.de");
    aboutData.addAuthor("Benjamin Meyer",I18N_NOOP("Various improvements"), "ben+kjumpingcube@meyerhome.net");
    KCmdLineArgs::init( argc, argv, &aboutData );

    KApplication app;
    KGlobal::locale()->insertCatalogue("libkdegames");

    // All session management is handled in the RESTORE macro
	if (app.isRestored())
        RESTORE(KJumpingCube)
    else {
		KJumpingCube *kjumpingcube = new KJumpingCube;
		app.setMainWidget(kjumpingcube);
		kjumpingcube->show();
	}
   return app.exec();
}
