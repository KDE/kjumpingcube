/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 1998,1999 by Matthias Kiefer
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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

**************************************************************************** */
#include "version.h"
#include "kjumpingcube.h"
#include <kapp.h>
#include <klocale.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>


static const char *description = 
	I18N_NOOP("Tactical one- or two-player game");


int main(int argc, char *argv[])
{
	KAboutData aboutData( "kjumpingcube", I18N_NOOP("KJumpingCube"), 
		KJC_VERSION, description, KAboutData::License_GPL, 
		"(c) 1999-2000, DEVELOPERS");
	aboutData.addAuthor("Matthias Kiefer",0, "matthias.kiefer@gmx.de");
	KCmdLineArgs::init( argc, argv, &aboutData );

	KApplication app;

	// All session management is handled in the RESTORE macro
	if (app.isRestored())
	{
		RESTORE(KJumpingCube)
	}
	else
	{
		KJumpingCube *widget = new KJumpingCube;
		widget->show();
	}

	return app.exec();
}
