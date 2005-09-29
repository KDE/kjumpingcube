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
#include "settings.h"
#include <kconfigdialog.h>

#include "prefs.h"

#include <qregexp.h>

#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kstdgameaction.h>
#include <kaction.h>
#include <kio/netaccess.h>
#include <kstatusbar.h>

#define ID_STATUS_TURN_TEXT 1000
#define ID_STATUS_TURN      2000

#define MESSAGE_TIME 2000


KJumpingCube::KJumpingCube()
  : view(new KCubeBoxWidget(5, this, "KCubeBoxWidget"))
{
   connect(view,SIGNAL(playerChanged(int)),SLOT(changePlayer(int)));
   connect(view,SIGNAL(stoppedMoving()),SLOT(disableStop()));
   connect(view,SIGNAL(stoppedThinking()),SLOT(disableStop()));
   connect(view,SIGNAL(startedMoving()),SLOT(enableStop_Moving()));
   connect(view,SIGNAL(startedThinking()),SLOT(enableStop_Thinking()));
   connect(view,SIGNAL(playerWon(int)),SLOT(showWinner(int)));

   // tell the KMainWindow that this is indeed the main widget
   setCentralWidget(view);

   // init statusbar
   QString s = i18n("Current player:");
   statusBar()->insertItem(s,ID_STATUS_TURN_TEXT, false);
   statusBar()->changeItem(s,ID_STATUS_TURN_TEXT);
   statusBar()->setItemAlignment (ID_STATUS_TURN_TEXT, AlignLeft | AlignVCenter);
   statusBar()->setFixedHeight( statusBar()->sizeHint().height() );
 
   currentPlayer = new QWidget(this, "currentPlayer");
   currentPlayer->setFixedWidth(40);
   statusBar()->addWidget(currentPlayer, ID_STATUS_TURN, false);
   statusBar()->setItemAlignment(ID_STATUS_TURN, AlignLeft | AlignVCenter);

   initKAction();
   changePlayer(1);
}

void KJumpingCube::initKAction() {
  KStdGameAction::gameNew(this, SLOT(newGame()), actionCollection());
  KStdGameAction::load(this, SLOT(openGame()), actionCollection());
  KStdGameAction::save(this, SLOT(save()), actionCollection());
  KStdGameAction::saveAs(this, SLOT(saveAs()), actionCollection());
  KStdGameAction::quit(this, SLOT(close()), actionCollection());

  hintAction = KStdGameAction::hint(view, SLOT(getHint()), actionCollection());
  stopAction = new KAction(i18n("Stop &Thinking"), "stop",
  Qt::Key_Escape, this, SLOT(stop()), actionCollection(), "game_stop");
  stopAction->setEnabled(false);
  undoAction = KStdGameAction::undo(this, SLOT(undo()), actionCollection());
  undoAction->setEnabled(false);
  KStdAction::preferences(this, SLOT(showOptions()), actionCollection());

  setupGUI();
}

void KJumpingCube::newGame(){
   undoAction->setEnabled(false);
   view->reset();
   statusBar()->message(i18n("New Game"),MESSAGE_TIME);
}

void KJumpingCube::saveGame(bool saveAs)
{
   if(saveAs || gameURL.isEmpty())
   {
      int result=0;
      KURL url;

      do
      {
         url = KFileDialog::getSaveURL(gameURL.url(),"*.kjc",this,0);

         if(url.isEmpty())
            return;

         // check filename
         QRegExp pattern("*.kjc",true,true);
         if(!pattern.exactMatch(url.filename()))
         {
            url.setFileName( url.filename()+".kjc" );
         }

         if(KIO::NetAccess::exists(url,false,this))
         {
            QString mes=i18n("The file %1 exists.\n"
               "Do you want to overwrite it?").arg(url.url());
            result = KMessageBox::warningContinueCancel(this, mes, QString::null, i18n("Overwrite"));
            if(result==KMessageBox::Cancel)
               return;
         }
      }
      while(result==KMessageBox::No);

      gameURL=url;
   }

   KTempFile tempFile;
   tempFile.setAutoDelete(true);
   KSimpleConfig config(tempFile.name());

   config.setGroup("KJumpingCube");
   config.writeEntry("Version",KJC_VERSION);
   config.setGroup("Game");
   view->saveGame(&config);
   config.sync();

   if(KIO::NetAccess::upload( tempFile.name(),gameURL,this ))
   {
      QString s=i18n("game saved as %1");
      s=s.arg(gameURL.url());
      statusBar()->message(s,MESSAGE_TIME);
   }
   else
   {
      KMessageBox::sorry(this,i18n("There was an error in saving file\n%1").arg(gameURL.url()));
   }
}

void KJumpingCube::openGame()
{
   bool fileOk=true;
   KURL url;

   do
   {
      url = KFileDialog::getOpenURL( gameURL.url(), "*.kjc", this, 0 );
      if( url.isEmpty() )
         return;
      if(!KIO::NetAccess::exists(url,true,this))
      {
         QString mes=i18n("The file %1 does not exist!").arg(url.url());
         KMessageBox::sorry(this,mes);
         fileOk=false;
      }
   }
   while(!fileOk);

   QString tempFile;
   if( KIO::NetAccess::download( url, tempFile, this ) )
   {
      KSimpleConfig config(tempFile,true);
      config.setGroup("KJumpingCube");
      if(!config.hasKey("Version"))
      {
         QString mes=i18n("The file %1 isn't a KJumpingCube gamefile!")
           .arg(url.url());
         KMessageBox::sorry(this,mes);
         return;
      }

      gameURL=url;
      config.setGroup("Game");
      view->restoreGame(&config);

      undoAction->setEnabled(false);

      KIO::NetAccess::removeTempFile( tempFile );
   }
   else
      KMessageBox::sorry(this,i18n("There was an error loading file\n%1").arg( url.url() ));
}

void KJumpingCube::stop()
{

   if(view->isMoving())
       undoAction->setEnabled(true);

   view->stopActivities();

   statusBar()->message(i18n("stopped activity"),MESSAGE_TIME);
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
   currentPlayer->setBackgroundColor(newPlayer == 1 ? Prefs::color1() : Prefs::color2());
   currentPlayer->repaint();
}

void KJumpingCube::showWinner(int player) {
  QString s=i18n("Winner is Player %1!").arg(player);
  KMessageBox::information(this,s,i18n("Winner"));
  view->reset();
}

void KJumpingCube::disableStop()
{
//   toolBar()->setItemEnabled(ID_GAME_STOP_HINT,false);
//   game->setItemEnabled(ID_GAME_STOP_HINT,false);
//   toolBar()->setItemEnabled(ID_GAME_HINT,true);
//   game->setItemEnabled(ID_GAME_HINT,true);
  stopAction->setEnabled(false);
  hintAction->setEnabled(true);
  statusBar()->clear();
}


void KJumpingCube::enableStop_Moving()
{
//   toolBar()->setItemEnabled(ID_GAME_STOP_HINT,true);
//   game->setItemEnabled(ID_GAME_STOP_HINT,true);
//   toolBar()->setItemEnabled(ID_GAME_HINT,false);
//   game->setItemEnabled(ID_GAME_HINT,false);
  stopAction->setEnabled(true);
  hintAction->setEnabled(false);
  statusBar()->message(i18n("Performing move."));
}

void KJumpingCube::enableStop_Thinking(){
  stopAction->setEnabled(true);
  hintAction->setEnabled(false);
  statusBar()->message(i18n("Computing next move."));
}

/**
 * Show Configure dialog.
 */
void KJumpingCube::showOptions(){
  if(KConfigDialog::showDialog("settings"))
    return;

  KConfigDialog *dialog = new KConfigDialog(this, "settings", Prefs::self(), KDialogBase::Swallow);
  dialog->addPage(new Settings(0, "General"), i18n("General"), "package_settings");
  connect(dialog, SIGNAL(settingsChanged()), view, SLOT(loadSettings()));
  dialog->show();
}

#include "kjumpingcube.moc"

