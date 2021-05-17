/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KJUMPINGCUBE_H
#define KJUMPINGCUBE_H

#include <QLabel>

#include <KXmlGuiWindow>
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

public Q_SLOTS:
   void setAction (const Action a, const bool onOff);

protected:
  /// To make sure all activity ceases before closing.
  bool queryClose() override;

private:
  Game * m_game;
  KCubeBoxWidget * m_view;
  QLabel *currentPlayer;
  QAction *undoAction, *redoAction, *stopAction, *hintAction;

  void initKAction();

  QPushButton * actionButton;
  QString       buttonLook;

private Q_SLOTS:
  void changePlayerColor (int newPlayer);
  void changeButton (bool enabled, bool stop = false,
                     const QString & caption = QString());
  void statusMessage (const QString & message, bool timed);
};

#endif // KJUMPINGCUBE_H

