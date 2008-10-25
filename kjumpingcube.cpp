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
   view->setObjectName("KCubeBoxWidget");
   view->makeStatusPixmaps (30);

   connect(view,SIGNAL(playerChanged(int)),SLOT(changePlayer(int)));
   connect(view,SIGNAL(colorChanged(int)),SLOT(changePlayerPixmap(int)));
   connect(view,SIGNAL(stoppedMoving()),SLOT(disableStop()));
   connect(view,SIGNAL(stoppedThinking()),SLOT(disableStop()));
   connect(view,SIGNAL(startedMoving()),SLOT(enableStop_Moving()));
   connect(view,SIGNAL(startedThinking()),SLOT(enableStop_Thinking()));
   connect(view,SIGNAL(playerWon(int)),SLOT(showWinner(int)));
   connect(view,SIGNAL(dimensionsChanged()),SLOT(newGame()));

   // tell the KMainWindow that this is indeed the main widget
   setCentralWidget(view);

   // init statusbar
   QString s = i18n("Current player:");
   statusBar()->addPermanentWidget (new QLabel (s));

   currentPlayer = new QLabel ();
   currentPlayer->setFrameStyle (QFrame::NoFrame);
   changePlayerPixmap(1);
   statusBar()->addPermanentWidget (currentPlayer);

   initKAction();
   changePlayer(1);
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

  stopAction = actionCollection()->addAction("game_stop");
  stopAction->setIcon(KIcon("process-stop"));
  stopAction->setText(i18n("Stop"));
  stopAction->setToolTip(i18n("Force the computer to move immediately"));
  stopAction->setWhatsThis
		(i18n("Stop the computer's calculation of its current move "
			"and force it to move immediately"));
  stopAction->setShortcut(Qt::Key_Escape);
  stopAction->setEnabled(false);
  connect(stopAction, SIGNAL(triggered(bool)), SLOT(stop()));

  undoAction = KStandardGameAction::undo(this, SLOT(undo()), this);
  actionCollection()->addAction(undoAction->objectName(), undoAction);
  undoAction->setEnabled(false);
  KStandardAction::preferences(this, SLOT(showOptions()), actionCollection());

  setupGUI();
}

void KJumpingCube::newGame(){
   undoAction->setEnabled(false);
   view->reset();
   statusBar()->showMessage(i18n("New Game"),MESSAGE_TIME);
}

void KJumpingCube::saveGame(bool saveAs)
{
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
   if(view->isActive())
      return;
   view->undo();
   undoAction->setEnabled(false);
}

void KJumpingCube::changePlayer(int newPlayer)
{
   undoAction->setEnabled(true);
   changePlayerPixmap(newPlayer);
}

void KJumpingCube::changePlayerPixmap(int player)
{
   currentPlayer->setPixmap (view->playerPixmap (player));
}

void KJumpingCube::showWinner(int player) {
  QString s=i18n("Winner is Player %1!", player);
  KMessageBox::information(this,s,i18n("Winner"));
  view->reset();
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
  statusBar()->showMessage(i18n("Performing move."));
}

void KJumpingCube::enableStop_Thinking(){
  stopAction->setEnabled(true);
  hintAction->setEnabled(false);
  statusBar()->showMessage(i18n("Computing next move."));
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
  connect(dialog, SIGNAL(settingsChanged(const QString&)), view, SLOT(loadSettings()));
  dialog->show();
}

#include "kjumpingcube.moc"

