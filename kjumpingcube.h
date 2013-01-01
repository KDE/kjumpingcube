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

#include <QLabel>

#include <kxmlguiwindow.h>
#include <game.h>

class QAction;
class KCubeBoxWidget;
class QPushButton;

/**
 * This class serves as the main window for KJumpingCube.  It handles the
 * menus, toolbars, and status bars.
 *
 * @short Main window class
 * @author Matthias Kiefer <matthias.kiefer@gmx.de>
 * @version 0.7.2
 */
class KJumpingCube : public KXmlGuiWindow {
  Q_OBJECT

public:
  /** Default Constructor */
  KJumpingCube();

public slots:
   void setAction (Action a, const bool onOff);

protected:
  /// To make sure all activity ceases before closing.
  bool queryClose();

private:
  Game * m_game;
  KCubeBoxWidget * m_view;
	QLabel *currentPlayer;
	QAction *undoAction, *redoAction, *stopAction, *hintAction;

  void initKAction();

  QPushButton * actionButton;
  QString       buttonLook;

private slots:
  void stop();
  void changePlayerColor (int newPlayer);
  void disableStop();
  void enableStop_Moving();
  void enableStop_Thinking();
  void changeButton (bool enabled, bool stop = false,
                     const QString & caption = QString());

  void showOptions();
};

#endif // KJUMPINGCUBE_H

