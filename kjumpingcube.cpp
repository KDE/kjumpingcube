/*****************************************************************************
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

#include "kjumpingcube.h"
#include "kcubeboxwidget.h"
#include "settingswidget.h"
#include "prefs.h"

#include <QSignalMapper>
#include <QStatusBar>
#include <KLocalizedString>
#include <KMessageBox>
#include <KStandardGameAction>
#include <QAction>
#include <KActionCollection>
#include <KStandardAction>
#include <KConfigDialog>
#include <QWidgetAction>

#include "kjumpingcube_debug.h"

#define MESSAGE_TIME 2000

KJumpingCube::KJumpingCube()
{
  // Make a KCubeBoxWidget with the user's currently preferred number of cubes.
   qCDebug(KJUMPINGCUBE_LOG) << "KJumpingCube::KJumpingCube() CONSTRUCTOR";
   m_view = new KCubeBoxWidget (Prefs::cubeDim(), this);
   m_game = new Game (Prefs::cubeDim(), m_view, this);
   m_view->makeStatusPixmaps (30);

   connect(m_game,SIGNAL(playerChanged(int)),SLOT(changePlayerColor(int)));
   connect(m_game,SIGNAL(buttonChange(bool,bool,QString)),
                  SLOT(changeButton(bool,bool,QString)));
   connect(m_game,SIGNAL(statusMessage(QString,bool)),
                  SLOT(statusMessage(QString,bool)));

   // Tell the KMainWindow that this is indeed the main widget.
   setCentralWidget (m_view);

   // init statusbar
   QString s = i18n("Current player:");
   statusBar()->addPermanentWidget (new QLabel (s));

   currentPlayer = new QLabel ();
   currentPlayer->setFrameStyle (QFrame::NoFrame);
   changePlayerColor(One);
   statusBar()->addPermanentWidget (currentPlayer);

   initKAction();

   connect (m_game, SIGNAL (setAction(Action,bool)),
                    SLOT   (setAction(Action,bool)));
   m_game->gameActions (NEW);		// Start a new game.
}

bool KJumpingCube::queryClose()
{
  // Terminate the AI or animation cleanly if either one is active.
  // If the AI is active, quitting immediately could cause a crash.
  m_game->shutdown();
  return true;
}

void KJumpingCube::initKAction() {
  QAction * action;

  QSignalMapper * gameMapper = new QSignalMapper (this);
  connect (gameMapper, SIGNAL (mapped(int)), m_game, SLOT (gameActions(int)));

  action = KStandardGameAction::gameNew (gameMapper, SLOT (map()), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, NEW);

  action = KStandardGameAction::load    (gameMapper, SLOT (map()), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, LOAD);

  action = KStandardGameAction::save    (gameMapper, SLOT (map()), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, SAVE);

  action = KStandardGameAction::saveAs  (gameMapper, SLOT (map()), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, SAVE_AS);

  action = KStandardGameAction::hint    (gameMapper, SLOT(map()), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, HINT);

  action = KStandardGameAction::undo    (gameMapper, SLOT (map()), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, UNDO);
  action->setEnabled (false);

  action = KStandardGameAction::redo    (gameMapper, SLOT (map()), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, REDO);
  action->setEnabled (false);

  actionButton = new QPushButton (this);
  actionButton->setObjectName ("ActionButton");
  // Action button's style sheet: parameters for red, green and clicked colors.
  buttonLook =
       "QPushButton#ActionButton { color: white; background-color: %1; "
           "border-style: outset; border-width: 2px; border-radius: 10px; "
           "border-color: beige; font: bold 14px; min-width: 10em; "
           "padding: 6px; margin: 5px; margin-left: 10px; } "
       "QPushButton#ActionButton:pressed { background-color: %2; "
           "border-style: inset; } "
       "QPushButton#ActionButton:disabled { color: white;"
            "border-color: beige; background-color: steelblue; }";
  gameMapper->setMapping (actionButton, BUTTON);
  connect (actionButton, SIGNAL(clicked()), gameMapper, SLOT(map()));

  QWidgetAction *widgetAction = new QWidgetAction(this);
  widgetAction->setDefaultWidget(actionButton);
  actionCollection()->addAction (QLatin1String ("action_button"), widgetAction);

  changeButton (true, true);		// Load the button's style sheet.
  changeButton (false);			// Set the button to be inactive.

  action = KStandardAction::preferences (m_game, SLOT(showSettingsDialog()),
                                         actionCollection());
  qCDebug(KJUMPINGCUBE_LOG) << "PREFERENCES ACTION is" << action->objectName();
  action->setIconText (i18n("Settings"));

  action = KStandardGameAction::quit (this, SLOT (close()), this);
  actionCollection()->addAction (action->objectName(), action);

  setupGUI();
}

void KJumpingCube::changeButton (bool enabled, bool stop,
                                 const QString & caption)
{
    qCDebug(KJUMPINGCUBE_LOG) << "KJumpingCube::changeButton (" << enabled << stop << caption;
    if (enabled && stop) {		// Red look (stop something).
        actionButton->setStyleSheet (buttonLook.arg("rgb(210, 0, 0)")
                                               .arg("rgb(180, 0, 0)"));
    }
    else if (enabled) {			// Green look (continue something).
        actionButton->setStyleSheet (buttonLook.arg("rgb(0, 170, 0)")
                                               .arg("rgb(0, 150, 0)"));
    }
    actionButton->setText (caption);
    actionButton->setEnabled (enabled);
}

void KJumpingCube::changePlayerColor (int newPlayer)
{
   currentPlayer->setPixmap (m_view->playerPixmap (newPlayer));
}

void KJumpingCube::setAction (const Action a, const bool onOff)
{
    // These must match enum Action (see file game.h) and be in the same order.
    const char * name [] = {"game_new",  "move_hint",   "action_button",
                            "move_undo", "move_redo",
                            "game_save", "game_saveAs", "game_load"};

    ((QAction *) actionCollection()->action (name [a]))->setEnabled (onOff);
}

void KJumpingCube::statusMessage (const QString & message, bool timed)
{
  if (timed) {
    statusBar()->showMessage (message, MESSAGE_TIME);
  }
  else {
    statusBar()->showMessage (message);
  }
}


