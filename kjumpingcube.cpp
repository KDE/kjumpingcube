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

// Settings
#include "ui_settings.h"

#include "prefs.h"

#include <QSignalMapper>
#include <QRegExp>

#include <klocale.h>
#include <kmessagebox.h>
#include <kstandardgameaction.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <kconfigdialog.h>
#include <kicon.h>

#define MESSAGE_TIME 2000

class SettingsWidget : public QWidget, public Ui::Settings
{
public:
    SettingsWidget(QWidget *parent)
        : QWidget(parent)
        {
            setupUi(this);
        }
};

KJumpingCube::KJumpingCube()
   // :
   // m_view (new KCubeBoxWidget (Prefs::cubeDim(), this)),
   // m_game (new Game (Prefs::cubeDim(), m_view))
  // Make a KCubeBoxWidget with the user's currently preferred number of cubes.
{
   qDebug() << "KJumpingCube::KJumpingCube() CONSTRUCTOR";
   m_view = new KCubeBoxWidget (Prefs::cubeDim(), this);
   m_game = new Game (Prefs::cubeDim(), m_view);
   m_view->makeStatusPixmaps (30);

   connect(m_game,SIGNAL(playerChanged(int)),SLOT(changePlayerColor(int)));
   connect(m_game,SIGNAL(buttonChange(bool,bool,const QString&)),
                  SLOT(changeButton(bool,bool,const QString&)));

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

   connect (m_game, SIGNAL (setAction(const Action,const bool)),
                    SLOT   (setAction(const Action,const bool)));
   if (Prefs::computerPlayer1()) {
      changeButton (true, false, i18n("Start computer move"));
   }
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
/* IDW DELETE.
  action = actionCollection()->addAction (QLatin1String ("game_stop"));
  action->setIcon (KIcon( QLatin1String( "process-stop" )));
  action->setText (i18n("Stop"));
  action->setToolTip (i18n("Force the computer to stop calculating or "
                               "animating a move"));
  action->setWhatsThis
		(i18n("Stop the computer's calculation of its current move "
                      "and force it to use the best one found so far, or stop "
                      "an animation and skip to the end"));
  action->setShortcut (Qt::Key_Escape);
  gameMapper->setMapping (action, STOP);
  connect (action, SIGNAL(triggered(bool)), gameMapper, SLOT(map()));
  action->setEnabled (false);
*/
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
           "padding: 6px; } "
       "QPushButton#ActionButton:pressed { background-color: %2; "
           "border-style: inset; } "
       "QPushButton#ActionButton:disabled { color: lightGray;"
            "border-color: gray; background-color: steelblue; }";
  gameMapper->setMapping (actionButton, BUTTON);
  connect (actionButton, SIGNAL(clicked()), gameMapper, SLOT(map()));

  KAction * b = actionCollection()->addAction (QLatin1String ("action_button"));
  b->setDefaultWidget (actionButton);	// Show the button on the toolbar.
  changeButton (true, true);		// Load the button's style sheet.
  changeButton (false);			// Set the button to be inactive.

  action = KStandardAction::preferences (this, SLOT(showOptions()),
                                         actionCollection());
  qDebug() << "PREFERENCES ACTION is" << action->objectName();
  action->setIconText (i18n("Settings"));

  action = KStandardGameAction::quit (this, SLOT (close()), this);
  actionCollection()->addAction (action->objectName(), action);

  setupGUI();
}

void KJumpingCube::changeButton (bool enabled, bool stop,
                                 const QString & caption)
{
    qDebug() << "KJumpingCube::changeButton (" << enabled << stop << caption;
    if (! enabled) {			// Inactive look (computer not busy).
        actionButton->setText ("Not active");
        actionButton->setEnabled (false);
        return;
    }
    if (stop) {				// Red look (stop something).
        actionButton->setStyleSheet (buttonLook.arg("rgb(224, 0, 0)")
                                               .arg("rgb(200, 0, 0)"));
    }
    else {				// Green look (continue something).
        actionButton->setStyleSheet (buttonLook.arg("rgb(0, 200, 0)")
                                               .arg("rgb(0, 170, 0)"));
    }
    actionButton->setText (caption);
    actionButton->setEnabled (true);
}

void KJumpingCube::stop()
{

   if(m_game->isMoving())
       undoAction->setEnabled(true);

   m_game->stopActivities();

   statusBar()->showMessage(i18n("stopped activity"),MESSAGE_TIME);
}

void KJumpingCube::changePlayerColor (int newPlayer)
{
   currentPlayer->setPixmap (m_view->playerPixmap (newPlayer));
}

// IDW TODO - Do disableStop(), enableStop_Moving() and enableStop_Thinking()
//            from the Game class, using setAction and statusMessage signals.
void KJumpingCube::disableStop()
{
  stopAction->setEnabled(false);
  hintAction->setEnabled(true);
  statusBar()->clearMessage();
}

void KJumpingCube::enableStop_Moving()
{
  stopAction->setEnabled(true);
  hintAction->setEnabled(false);
  statusBar()->showMessage(i18n("Performing a move."));
}

void KJumpingCube::enableStop_Thinking(){
  stopAction->setEnabled(true);
  hintAction->setEnabled(false);
  statusBar()->showMessage(i18n("Computing a move."));
}
// End of IDW TODO.

/**
 * Show Configure dialog.
 */
void KJumpingCube::showOptions(){
  if(KConfigDialog::showDialog("settings"))
    return;

  KConfigDialog *dialog = new KConfigDialog(this, "settings", Prefs::self());
  dialog->setFaceType(KPageDialog::Plain);
  dialog->addPage(new SettingsWidget(this), i18n("General"),
                  "games-config-options");
  connect(dialog,SIGNAL(settingsChanged(QString)),m_game,SLOT(loadSettings()));
  dialog->show();
}

void KJumpingCube::setAction (Action a, const bool onOff)
{
    // These must match enum Action (see file game.h) and be in the same order.
    const char * name [] = {"game_new",  "move_hint",   "action_button",
                            "move_undo", "move_redo",
                            "game_save", "game_saveAs", "game_load"};

    ((QAction *) actionCollection()->action (name [a]))->setEnabled (onOff);
}

#include "kjumpingcube.moc"

