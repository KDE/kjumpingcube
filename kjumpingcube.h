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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

**************************************************************************** */
#ifndef KJUMPINGCUBE_H
#define KJUMPINGCUBE_H 

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <kapp.h>
#include <kmainwindow.h>
#include <kurl.h>

#include "kcubeboxwidget.h"

class KAccel;
class QPopupMenu;
class QString;

/**
 * This class serves as the main window for KJumpingCube.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Matthias Kiefer <matthias.kiefer@gmx.de>
 * @version 0.7.2
 */
class KJumpingCube : public KMainWindow
{
   Q_OBJECT
public:
   /** Default Constructor */
   KJumpingCube();

   /** Default Destructor */
   virtual ~KJumpingCube();

 
protected:
   /**
   * This function is called when it is time for the app to save its
   * properties for session management purposes.
   */
   void saveProperties(KConfig *);

   /**
   * This function is called when this app is restored.  The KConfig
   * object points to the session management config file that was saved
   * with @ref saveProperties
   */
   void readProperties(KConfig *);
	
   /** Just to check if the brain is still working*/
   virtual bool queryClose();

private:
   KCubeBoxWidget *view;
   KAccel *kaccel;

   KURL gameURL;

   void initKAction();
   void updatePlayfieldMenu(int dim);
   void updateSkillMenu(int id);
   void changeColor(int player);

private slots:
   void quit();
   void saveSettings();
   void newGame();
   void saveGame(bool saveAs=false);
   void saveAs();
   void save();
   void openGame();
   void getHint();
   void stop();
   void undo();
   void changePlayer(int newPlayer);
   void showWinner(int);
   void barPositionChanged();
   void disableStop();
   void enableStop_Moving();
   void enableStop_Thinking();
   void toggleToolbar();
   void toggleStatusbar();
   void configureKeyBindings();
   void fieldChange();
   void skillChange();
   void changeComputerPlayer1();
   void changeComputerPlayer2();
   void changeColor1();
   void changeColor2();

};
#endif // KJUMPINGCUBE_H
