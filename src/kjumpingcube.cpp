/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kjumpingcube.h"
#include "kcubeboxwidget.h"
#include "settingswidget.h"
#include "prefs.h"

#include <QSignalMapper>
#include <QStatusBar>
#include <KLocalizedString>
#include <KStandardGameAction>
#include <QAction>
#include <KActionCollection>
#include <KStandardAction>
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

   connect(m_game,&Game::playerChanged,this, &KJumpingCube::changePlayerColor);
   connect(m_game,&Game::buttonChange,
                  this, &KJumpingCube::changeButton);
   connect(m_game,&Game::statusMessage,
                  this, &KJumpingCube::statusMessage);

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

   connect (m_game, &Game::setAction,
                    this, &KJumpingCube::setAction);
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
  connect (gameMapper, &QSignalMapper::mappedInt, m_game, &Game::gameActions);

  action = KStandardGameAction::gameNew (gameMapper, QOverload<>::of(&QSignalMapper::map), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, NEW);

  action = KStandardGameAction::load    (gameMapper, QOverload<>::of(&QSignalMapper::map), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, LOAD);

  action = KStandardGameAction::save    (gameMapper, QOverload<>::of(&QSignalMapper::map), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, SAVE);

  action = KStandardGameAction::saveAs  (gameMapper, QOverload<>::of(&QSignalMapper::map), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, SAVE_AS);

  action = KStandardGameAction::hint    (gameMapper, QOverload<>::of(&QSignalMapper::map), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, HINT);

  action = KStandardGameAction::undo    (gameMapper, QOverload<>::of(&QSignalMapper::map), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, UNDO);
  action->setEnabled (false);

  action = KStandardGameAction::redo    (gameMapper, QOverload<>::of(&QSignalMapper::map), this);
  actionCollection()->addAction (action->objectName(), action);
  gameMapper->setMapping (action, REDO);
  action->setEnabled (false);

  actionButton = new QPushButton (this);
  actionButton->setObjectName (QStringLiteral("ActionButton"));
  // Action button's style sheet: parameters for red, green and clicked colors.
  buttonLook = QStringLiteral(
       "QPushButton#ActionButton { color: white; background-color: %1; "
           "border-style: outset; border-width: 2px; border-radius: 10px; "
           "border-color: beige; font: bold 14px; min-width: 10em; "
           "padding: 6px; margin: 5px; margin-left: 10px; } "
       "QPushButton#ActionButton:pressed { background-color: %2; "
           "border-style: inset; } "
       "QPushButton#ActionButton:disabled { color: white;"
            "border-color: beige; background-color: steelblue; }");
  gameMapper->setMapping (actionButton, BUTTON);
  connect (actionButton, &QAbstractButton::clicked, gameMapper, QOverload<>::of(&QSignalMapper::map));

  QWidgetAction *widgetAction = new QWidgetAction(this);
  widgetAction->setDefaultWidget(actionButton);
  actionCollection()->addAction (QStringLiteral ("action_button"), widgetAction);

  changeButton (true, true);		// Load the button's style sheet.
  changeButton (false);			// Set the button to be inactive.

  action = KStandardAction::preferences (m_game, [this]() { m_game->showSettingsDialog(true); },
                                         actionCollection());
  qCDebug(KJUMPINGCUBE_LOG) << "PREFERENCES ACTION is" << action->objectName();
  action->setIconText (i18n("Settings"));

  action = KStandardGameAction::quit (this, &KJumpingCube::close, this);
  actionCollection()->addAction (action->objectName(), action);

  setupGUI();
}

void KJumpingCube::changeButton (bool enabled, bool stop,
                                 const QString & caption)
{
    qCDebug(KJUMPINGCUBE_LOG) << "KJumpingCube::changeButton (" << enabled << stop << caption;
    if (enabled && stop) {		// Red look (stop something).
        actionButton->setStyleSheet (buttonLook.arg(QStringLiteral("rgb(210, 0, 0)"))
                                               .arg(QStringLiteral("rgb(180, 0, 0)")));
    }
    else if (enabled) {			// Green look (continue something).
        actionButton->setStyleSheet (buttonLook.arg(QStringLiteral("rgb(0, 170, 0)"))
                                               .arg(QStringLiteral("rgb(0, 150, 0)")));
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
    static const QString name [] = {
        QStringLiteral("game_new"),
        QStringLiteral("move_hint"),
        QStringLiteral("action_button"),
        QStringLiteral("move_undo"),
        QStringLiteral("move_redo"),
        QStringLiteral("game_save"),
        QStringLiteral("game_saveAs"),
        QStringLiteral("game_load")};

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


