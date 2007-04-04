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
#ifndef KJUMPINGCUBE_H
#define KJUMPINGCUBE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kmainwindow.h>
#include <kurl.h>

class KAction;
class KCubeBoxWidget;

/**
 * This class serves as the main window for KJumpingCube.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Matthias Kiefer <matthias.kiefer@gmx.de>
 * @version 0.7.2
 */
class KJumpingCube : public KMainWindow {
  Q_OBJECT

public:
  /** Default Constructor */
  KJumpingCube();

private:
  KCubeBoxWidget *view;
	QWidget *currentPlayer;
	KAction *undoAction, *stopAction, *hintAction;

  KURL gameURL;
  void initKAction();

private slots:
  void newGame();
  void saveGame(bool saveAs=false);
  inline void saveAs() { saveGame(true); }
  inline void save() { saveGame(false); }
  void openGame();
  void stop();
  void undo();
  void changePlayer(int newPlayer);
  void showWinner(int);
  void disableStop();
  void enableStop_Moving();
  void enableStop_Thinking();

  void showOptions();
};

#endif // KJUMPINGCUBE_H

