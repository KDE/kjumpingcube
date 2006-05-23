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
#include <kconfigdialog.h>

#include "prefs.h"

#include <QRegExp>

#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kstdgameaction.h>
#include <kaction.h>
#include <kio/netaccess.h>
#include <kstatusbar.h>
#include <kstdaction.h>

#define ID_STATUS_TURN_TEXT 1000
#define ID_STATUS_TURN      2000

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
  : view(new KCubeBoxWidget(5, this))
{
   view->setObjectName("KCubeBoxWidget");
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
   statusBar()->setItemAlignment (ID_STATUS_TURN_TEXT, Qt::AlignLeft | Qt::AlignVCenter);
   statusBar()->setFixedHeight( statusBar()->sizeHint().height() );

   currentPlayer = new QWidget(this);
   currentPlayer->setObjectName("currentPlayer");
   currentPlayer->setFixedWidth(40);
   statusBar()->addWidget(currentPlayer, ID_STATUS_TURN);
   statusBar()->setItemAlignment(ID_STATUS_TURN, Qt::AlignLeft | Qt::AlignVCenter);

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
  stopAction = new KAction(KIcon("stop"), i18n("Stop &Thinking"), actionCollection(), "game_stop");
  connect(stopAction, SIGNAL(triggered(bool)), SLOT(stop()));
  stopAction->setShortcut(Qt::Key_Escape);
  stopAction->setEnabled(false);
  undoAction = KStdGameAction::undo(this, SLOT(undo()), actionCollection());
  undoAction->setEnabled(false);
  KStdAction::preferences(this, SLOT(showOptions()), actionCollection());

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
         url = KFileDialog::getSaveURL(gameURL.url(),"*.kjc",this,0);

         if(url.isEmpty())
            return;

         // check filename
         QRegExp pattern("*.kjc",Qt::CaseSensitive,QRegExp::Wildcard);
         if(!pattern.exactMatch(url.fileName()))
         {
            url.setFileName( url.fileName()+".kjc" );
         }

         if(KIO::NetAccess::exists(url,false,this))
         {
            QString mes=i18n("The file %1 exists.\n"
               "Do you want to overwrite it?", url.url());
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
      url = KFileDialog::getOpenURL( gameURL.url(), "*.kjc", this, 0 );
      if( url.isEmpty() )
         return;
      if(!KIO::NetAccess::exists(url,true,this))
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
      KSimpleConfig config(tempFile,true);
      config.setGroup("KJumpingCube");
      if(!config.hasKey("Version"))
      {
         QString mes=i18n("The file %1 isn't a KJumpingCube gamefile!",
            url.url());
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
   QPalette palette;
   palette.setColor(backgroundRole(),
		   newPlayer == 1 ? Prefs::color1() : Prefs::color2());
   currentPlayer->setPalette(palette);
   currentPlayer->repaint();
}

void KJumpingCube::showWinner(int player) {
  QString s=i18n("Winner is Player %1!", player);
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
  statusBar()->clearMessage();
}


void KJumpingCube::enableStop_Moving()
{
//   toolBar()->setItemEnabled(ID_GAME_STOP_HINT,true);
//   game->setItemEnabled(ID_GAME_STOP_HINT,true);
//   toolBar()->setItemEnabled(ID_GAME_HINT,false);
//   game->setItemEnabled(ID_GAME_HINT,false);
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

  KConfigDialog *dialog = new KConfigDialog(this, "settings", Prefs::self(), KDialogBase::Swallow);
  dialog->addPage(new SettingsWidget(this), i18n("General"), "package_settings");
  connect(dialog, SIGNAL(settingsChanged(const QString&)), view, SLOT(loadSettings()));
  dialog->show();
}

#include "kjumpingcube.moc"

