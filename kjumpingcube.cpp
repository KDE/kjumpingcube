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
#include "version.h"

// Settings
#include "ui_settings.h"

#include "prefs.h"

#include <QRegExp>

#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktemporaryfile.h>
#include <kstandardgameaction.h>
#include <kaction.h>
#include <kactioncollection.h>
#include <kio/netaccess.h>
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
  : view(new KCubeBoxWidget(Prefs::cubeDim(), this))
  // Make a KCubeBoxWidget with the user's currently preferred number of cubes.
{
   view->setObjectName( QLatin1String("KCubeBoxWidget" ));
   view->makeStatusPixmaps (30);

   connect(view,SIGNAL(playerChanged(int)),SLOT(changePlayerColor(int)));
   connect(view,SIGNAL(colorChanged(int)),SLOT(changePlayerColor(int)));
   connect(view,SIGNAL(stoppedMoving()),SLOT(disableStop()));
   connect(view,SIGNAL(stoppedThinking()),SLOT(disableStop()));
   connect(view,SIGNAL(startedMoving()),SLOT(enableStop_Moving()));
   connect(view,SIGNAL(startedThinking()),SLOT(enableStop_Thinking()));
   connect(view,SIGNAL(playerWon(int)),SLOT(showWinner(int)));
   connect(view,SIGNAL(dimensionsChanged()),SLOT(newGame()));
   connect(view,SIGNAL(newMove()),SLOT(newMoveSeen()));
   connect(view,SIGNAL(buttonChange(bool,bool,const QString&)),
                  SLOT(changeButton(bool,bool,const QString&)));

   // tell the KMainWindow that this is indeed the main widget
   setCentralWidget(view);

   // init statusbar
   QString s = i18n("Current player:");
   statusBar()->addPermanentWidget (new QLabel (s));

   currentPlayer = new QLabel ();
   currentPlayer->setFrameStyle (QFrame::NoFrame);
   changePlayerColor(One);
   statusBar()->addPermanentWidget (currentPlayer);

   initKAction();

   if (Prefs::computerPlayer1()) {
      changeButton (true, false, i18n("Start computer move"));
   }
}

bool KJumpingCube::queryClose()
{
  // Terminate the AI or animation cleanly if either one is active.
  // If the AI is active, quitting immediately could cause a crash.
  view->shutdown();
  return true;
}

void KJumpingCube::initKAction() {
  QAction *action;

  action = KStandardGameAction::gameNew(this, SLOT(newGame()), this);
  actionCollection()->addAction(action->objectName(), action);
  action = KStandardGameAction::load(this, SLOT(openGame()), this);
  actionCollection()->addAction(action->objectName(), action);
  action = KStandardGameAction::save(this, SLOT(save()), this);
  actionCollection()->addAction(action->objectName(), action);
  action = KStandardGameAction::saveAs(this, SLOT(saveAs()), this);
  actionCollection()->addAction(action->objectName(), action);
  action = KStandardGameAction::quit(this, SLOT(close()), this);
  actionCollection()->addAction(action->objectName(), action);

  hintAction = KStandardGameAction::hint(view, SLOT(getHint()), this);
  actionCollection()->addAction(hintAction->objectName(), hintAction);

  stopAction = actionCollection()->addAction( QLatin1String( "game_stop" ));
  stopAction->setIcon(KIcon( QLatin1String( "process-stop" )));
  stopAction->setText(i18n("Stop"));
  stopAction->setToolTip(i18n("Force the computer to stop calculating or "
                              "animating a move"));
  stopAction->setWhatsThis
		(i18n("Stop the computer's calculation of its current move "
                      "and force it to use the best one found so far, or stop "
                      "an animation and skip to the end"));
  stopAction->setShortcut(Qt::Key_Escape);
  stopAction->setEnabled(false);
  connect(stopAction, SIGNAL(triggered(bool)), SLOT(stop()));

  undoAction = KStandardGameAction::undo(this, SLOT(undo()), this);
  actionCollection()->addAction(undoAction->objectName(), undoAction);
  undoAction->setEnabled(false);

  redoAction = KStandardGameAction::redo(this, SLOT(redo()), this);
  actionCollection()->addAction(redoAction->objectName(), redoAction);
  redoAction->setEnabled(false);

  actionButton = new QPushButton (this);
  actionButton->setObjectName ("ActionButton");
  // Style sheet for action button: parameters for red/green and clicked colors.
  buttonLook =
       "QPushButton#ActionButton { color: white; background-color: %1; "
           "border-style: outset; border-width: 2px; border-radius: 10px; "
           "border-color: beige; font: bold 14px; min-width: 10em; "
           "padding: 6px; } "
       "QPushButton#ActionButton:pressed { background-color: %2; "
           "border-style: inset; } "
       "QPushButton#ActionButton:disabled { color: lightGray;"
            "border-color: gray; background-color: steelblue; }";

  KAction * b = new KAction (this);
  actionCollection()->addAction ( QLatin1String ("action_button"), b);
  b->setDefaultWidget (actionButton);
  changeButton (true, true);		// Load the button's style sheet.
  changeButton (false);			// Set the button to be inactive.
  connect (actionButton, SIGNAL(clicked()), view, SLOT(buttonClick()));

  action = KStandardAction::preferences(this, SLOT(showOptions()), actionCollection());
  qDebug() << "PREFERENCES ACTION is" << action->objectName();
  action->setIconText (i18n("Settings"));

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

void KJumpingCube::newGame()
{
   // IDW DELETE. stop();		// Stop the current move (if any).
   view->shutdown();			// Stop the current move (if any).
   view->reset();			// Clear the cubebox.
   undoAction->setEnabled(false);
   redoAction->setEnabled(false);
   statusBar()->showMessage(i18n("New Game"),MESSAGE_TIME);
}

void KJumpingCube::saveGame(bool saveAs)
{
   // IDW TODO. if (view->waitToSave (saveAs)) return;

   if(saveAs || gameURL.isEmpty())
   {
      int result=0;
      KUrl url;

      do
      {
         url = KFileDialog::getSaveUrl(gameURL.url(),"*.kjc",this,0);

         if(url.isEmpty())
            return;

         // check filename
         QRegExp pattern("*.kjc",Qt::CaseSensitive,QRegExp::Wildcard);
         if(!pattern.exactMatch(url.fileName()))
         {
            url.setFileName( url.fileName()+".kjc" );
         }

         if(KIO::NetAccess::exists(url, KIO::NetAccess::DestinationSide, this))
         {
            QString mes=i18n("The file %1 exists.\n"
               "Do you want to overwrite it?", url.url());
            result = KMessageBox::warningContinueCancel(this, mes, QString(), KGuiItem(i18n("Overwrite")));
            if(result==KMessageBox::Cancel)
               return;
         }
      }
      while(result==KMessageBox::No);

      gameURL=url;
   }

   KTemporaryFile tempFile;
   tempFile.open();
   KConfig config(tempFile.fileName(), KConfig::SimpleConfig);
   KConfigGroup main(&config, "KJumpingCube");
   main.writeEntry("Version",KJC_VERSION);
   KConfigGroup game(&config, "Game");
   view->saveGame(game);
   config.sync();

   if(KIO::NetAccess::upload( tempFile.fileName(),gameURL,this ))
   {
      QString s=i18n("game saved as %1", gameURL.url());
      statusBar()->showMessage(s,MESSAGE_TIME);
   }
   else
   {
      KMessageBox::sorry(this,i18n("There was an error in saving file\n%1", gameURL.url()));
   }
}

void KJumpingCube::openGame()
{
   // IDW TODO. if (view->waitToLoad()) return;

   bool fileOk=true;
   KUrl url;

   do
   {
      url = KFileDialog::getOpenUrl( gameURL.url(), "*.kjc", this, 0 );
      if( url.isEmpty() )
         return;
      if(!KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, this))
      {
         QString mes=i18n("The file %1 does not exist!", url.url());
         KMessageBox::sorry(this,mes);
         fileOk=false;
      }
   }
   while(!fileOk);

   QString tempFile;
   if( KIO::NetAccess::download( url, tempFile, this ) )
   {
      KConfig config( tempFile, KConfig::SimpleConfig);
      KConfigGroup main(&config, "KJumpingCube");
      if(!main.hasKey("Version"))
      {
         QString mes=i18n("The file %1 is not a KJumpingCube gamefile!",
            url.url());
         KMessageBox::sorry(this,mes);
         return;
      }

      gameURL=url;
      KConfigGroup game(&config, "Game");
      view->restoreGame(game);

      undoAction->setEnabled(false);

      KIO::NetAccess::removeTempFile( tempFile );
   }
   else
      KMessageBox::sorry(this,i18n("There was an error loading file\n%1", url.url() ));
}

void KJumpingCube::stop()
{

   if(view->isMoving())
       undoAction->setEnabled(true);

   view->stopActivities();

   statusBar()->showMessage(i18n("stopped activity"),MESSAGE_TIME);
}

void KJumpingCube::undo()
{
   // IDW TODO. if (view->waitToUndo()) return;

   // IDW test. if(view->isActive())
      // IDW test. return;
   int moreToUndo = view->undoRedo (-1);
   if (moreToUndo >= 0) {
      undoAction->setEnabled (moreToUndo == 1);
   }
   redoAction->setEnabled (true);
}

void KJumpingCube::redo()
{
   // IDW test. if(view->isActive())
      // IDW test. return;
   int moreToRedo = view->undoRedo (+1);
   if (moreToRedo >= 0) {
      redoAction->setEnabled (moreToRedo == 1);
   }
   undoAction->setEnabled (true);
}

void KJumpingCube::changePlayerColor (int newPlayer)
{
   currentPlayer->setPixmap (view->playerPixmap (newPlayer));
}

void KJumpingCube::showWinner(int player) {
  QString s = i18n("Winner is Player %1!", player);
  // Comment this out for IDW high-speed test.
  KMessageBox::information (this, s, i18n("Winner"));
}

void KJumpingCube::newMoveSeen()
{
    undoAction->setEnabled(true);
    redoAction->setEnabled(false);
}

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

/**
 * Show Configure dialog.
 */
void KJumpingCube::showOptions(){
  if(KConfigDialog::showDialog("settings"))
    return;

  KConfigDialog *dialog = new KConfigDialog(this, "settings", Prefs::self());
  dialog->setFaceType(KPageDialog::Plain);
  dialog->addPage(new SettingsWidget(this), i18n("General"), "games-config-options");
  connect(dialog, SIGNAL(settingsChanged(QString)), view, SLOT(loadSettings()));
  dialog->show();
}

#include "kjumpingcube.moc"

