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
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

**************************************************************************** */
#include "kjumpingcube.h"
#include "kcubeboxwidget.h"
#include "version.h"

// Settings
#include "settings.h"
#include "kautoconfig.h"
#include <kdialogbase.h>
#include <kiconloader.h>

#include <qfileinfo.h>
#include <qregexp.h>

#include <kapplication.h>
#include <kglobal.h>
#include <klocale.h>
#include <kkeydialog.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kstdaction.h>
#include <kstdgameaction.h>
#include <kaction.h>
#include <kio/netaccess.h>
#include <kstatusbar.h>

#define ID_STATUS_TURN 1000

#define MESSAGE_TIME 2000


KJumpingCube::KJumpingCube()
	: view(new KCubeBoxWidget(5,this))
{
   connect(view,SIGNAL(playerChanged(int)),SLOT(changePlayer(int)));
   connect(view,SIGNAL(stoppedMoving()),SLOT(disableStop()));
   connect(view,SIGNAL(stoppedThinking()),SLOT(disableStop()));
   connect(view,SIGNAL(startedMoving()),SLOT(enableStop_Moving()));
   connect(view,SIGNAL(startedThinking()),SLOT(enableStop_Thinking()));
   connect(view,SIGNAL(playerWon(int)),SLOT(showWinner(int)));

   // tell the KMainWindow that this is indeed the main widget
   setCentralWidget(view);

   initKAction();

   QString s;
   // init statusbar
   s = i18n("On turn: Player %1").arg(1);
   statusBar()->insertItem(s+i18n("(Computer)"),ID_STATUS_TURN,2);
   statusBar()->changeItem(s,ID_STATUS_TURN);
   statusBar()->setItemAlignment (ID_STATUS_TURN,AlignLeft | AlignVCenter);
   statusBar()->setFixedHeight( statusBar()->sizeHint().height());

  resize(400,400);
  setAutoSaveSettings();
  showStatusbar->setChecked(!statusBar()->isHidden());
  showToolbar->setChecked(!toolBar()->isHidden());
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
  stopAction->setEnabled(FALSE);
  undoAction = KStdGameAction::undo(this, SLOT(undo()), actionCollection());
  undoAction->setEnabled(FALSE);

  showToolbar = KStdAction::showToolbar(this, SLOT(toggleToolbar()),
	actionCollection());
  showStatusbar = KStdAction::showStatusbar(this, SLOT(toggleStatusbar()),
	actionCollection());
  KStdAction::keyBindings(this, SLOT(configureKeyBindings()),
	actionCollection());

  (void)new KAction(i18n("&Configure KJumpingCube..."), 0, this,
	SLOT(showOptions()), actionCollection(), "configure_kjumpingcube" );
  
  // finally create toolbar and menubar
  createGUI();
}

KJumpingCube::~KJumpingCube(){
  delete view;
}

void KJumpingCube::toggleToolbar(){
 if (toolBar()->isHidden())
   toolBar()->show();
 else
   toolBar()->hide();
}

void KJumpingCube::toggleStatusbar(){
 if (statusBar()->isHidden())
   statusBar()->show();
 else 
   statusBar()->hide();
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
         if(pattern.exactMatch(url.filename()))
         {
            url.setFileName( url.filename()+".kjc" );
         }

         if(KIO::NetAccess::exists(url))
         {
            QString mes=i18n("The file %1 exists.\n"
			     "Do you want to overwrite it?").arg(url.url());
	    result = KMessageBox::warningYesNoCancel(this, mes);
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

   if(KIO::NetAccess::upload( tempFile.name(),gameURL ))
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
      if(!KIO::NetAccess::exists(url.url()))
      {
         QString mes=i18n("The file %1 does not exist!").arg(url.url());
         KMessageBox::sorry(this,mes);
         fileOk=false;
      }
   }
   while(!fileOk);

   QString tempFile;
   if( KIO::NetAccess::download( url, tempFile ) )
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
   QString s=i18n("On turn: Player %1");
   s=s.arg(newPlayer);
   if(view->isComputer((KCubeBoxWidget::Player)newPlayer))
      s+=i18n("(Computer)");

   statusBar()->changeItem(s,ID_STATUS_TURN);

   undoAction->setEnabled(true);
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
  statusBar()->message(i18n("doing move"));
}

void KJumpingCube::enableStop_Thinking(){
  stopAction->setEnabled(true);
  hintAction->setEnabled(false);
  statusBar()->message(i18n("computing move"));
}

void KJumpingCube::configureKeyBindings(){
  KKeyDialog::configure(actionCollection(), this);
}

/**
 * Show Configure dialog.
 */
void KJumpingCube::showOptions(){
  options = new KDialogBase (this, "Configure", false, i18n("Settings"), KDialogBase::Default | KDialogBase::Ok | KDialogBase::Apply | KDialogBase::Cancel);
  KAutoConfig *kautoconfig = new KAutoConfig(options, "KAutoConfig");
  
  connect(options, SIGNAL(okClicked()), kautoconfig, SLOT(saveSettings()));
  connect(options, SIGNAL(okClicked()), this, SLOT(closeOptions()));
  connect(options, SIGNAL(applyClicked()), kautoconfig, SLOT(saveSettings()));
  connect(options, SIGNAL(defaultClicked()), kautoconfig, SLOT(resetSettings()));

  Settings *settings = new Settings(options, "General");
  options->setMainWidget(settings);
  kautoconfig->addWidget(settings, "Game");

  kautoconfig->retrieveSettings();
  options->show();
	
  connect(kautoconfig, SIGNAL(settingsChanged()), view, SLOT(readSettings()));
}

void KJumpingCube::closeOptions(){
  options->close(true);
}

#include "kjumpingcube.moc"

