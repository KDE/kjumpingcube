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
#include "brain.h"
#include "version.h"

#include <qfileinfo.h>
#include <qregexp.h>

#include <kapplication.h>
#include <kglobal.h>
#include <klocale.h>
#include <kkeydialog.h>
#include <kiconloader.h>
#include <kcolordialog.h>
#include <kfiledialog.h>
#include <ksimpleconfig.h>
#include <kmenubar.h>
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

   connect(toolBar(),SIGNAL(moved(BarPosition)),SLOT(barPositionChanged()));//FIXME: crashing :-(


   QString s;
   // init statusbar
   s = i18n("On turn: Player %1").arg(1);
   statusBar()->insertItem(s+i18n("(Computer)"),ID_STATUS_TURN,2);
   statusBar()->changeItem(s,ID_STATUS_TURN);
   statusBar()->setItemAlignment (ID_STATUS_TURN,AlignLeft | AlignVCenter);
   statusBar()->setFixedHeight( statusBar()->sizeHint().height());


   KConfig *config=kapp->config();
   // read config
   {
      KConfigGroupSaver cgs(config,"Window");

      int barPos = config->readNumEntry("ToolbarPos",(int)(KToolBar::Top));
      toolBar()->setBarPos((KToolBar::BarPosition)barPos);

      bool visible=config->readBoolEntry("Toolbar",true);
      if (toolBar()->isHidden() == visible)
         showToolbar->activate();

      visible=config->readNumEntry("Statusbar",true);
      if (statusBar()->isHidden() == visible)
         showStatusbar->activate();

      QSize defSize(400,400);
      QSize winSize=config->readSizeEntry("Size",&defSize);
      resize(winSize);
   }
   {
      KConfigGroupSaver cgs(config,"Game");

      QColor c1=QColor("darkred");
      QColor c2=QColor("darkblue");
      QPalette color1(config->readColorEntry("Color1",&c1));
      QPalette color2(config->readColorEntry("Color2",&c2));
      view->setColor(KCubeBoxWidget::One,color1);
      view->setColor(KCubeBoxWidget::Two,color2);

      view->setDim(config->readNumEntry("CubeDim",6));
      updatePlayfieldMenu(view->dim());
      view->setSkill((Brain::Skill)config->readNumEntry("Skill",(int)Brain::Average));
      updateSkillMenu((int)view->skill());

      bool flag=config->readBoolEntry("Computer Pl.1",false);
      view->setComputerplayer(KCubeBoxWidget::One,flag);
      changeComputer1->setChecked(flag);
      flag=config->readBoolEntry("Computer Pl.2",false);
      view->setComputerplayer(KCubeBoxWidget::Two,flag);
      changeComputer2->setChecked(flag);
   }
}


void KJumpingCube::initKAction()
{
   KStdGameAction::gameNew(this, SLOT(newGame()), actionCollection());
   KStdGameAction::load(this, SLOT(openGame()), actionCollection());
   KStdGameAction::save(this, SLOT(save()), actionCollection());
   KStdGameAction::saveAs(this, SLOT(saveAs()), actionCollection());
   KStdGameAction::quit(kapp, SLOT(quit()), actionCollection());

   hintAction = KStdGameAction::hint(this, SLOT(getHint()), actionCollection());
   stopAction = new KAction(i18n("Stop &Thinking"), "stop", Qt::Key_Escape, this, SLOT(stop()), actionCollection(), "game_stop");
   stopAction->setEnabled(FALSE);
   undoAction = KStdGameAction::undo(this, SLOT(undo()), actionCollection());
   undoAction->setEnabled(FALSE);


   showToolbar = KStdAction::showToolbar(this, SLOT(toggleToolbar()), actionCollection());
   showStatusbar = KStdAction::showStatusbar(this, SLOT(toggleStatusbar()), actionCollection());
   KStdAction::keyBindings(this, SLOT(configureKeyBindings()), actionCollection());

   QStringList plist;
   plist.append(i18n("&5x5"));
   plist.append(i18n("&6x6"));
   plist.append(i18n("&7x7"));
   plist.append(i18n("&8x8"));
   plist.append(i18n("&9x9"));
   plist.append(i18n("&10x10"));
   optionField = new KSelectAction(i18n("&Playfield"), 0, this, SLOT(fieldChange()), actionCollection(), "options_field");
   optionField->setItems(plist);

   QStringList slist;
   slist.append(i18n("&Beginner"));
   slist.append(i18n("&Average"));
   slist.append(i18n("&Expert"));
   optionSkill = KStdGameAction::chooseGameType(this, SLOT(skillChange()), actionCollection());
   optionSkill->setItems(slist);

   changeComputer1 = new KToggleAction(i18n("Computer Plays Player &1"), 0, this, SLOT(changeComputerPlayer1()), actionCollection(), "options_change_computer1");
   changeComputer2 = new KToggleAction(i18n("Computer Plays Player &2"), 0, this, SLOT(changeComputerPlayer2()), actionCollection(), "options_change_computer2");

   (void)new KAction(i18n("Color Player &1"), 0, this, SLOT(changeColor1()), actionCollection(), "options_change_color1");
   (void)new KAction(i18n("Color Player &2"), 0, this, SLOT(changeColor2()), actionCollection(), "options_change_color2");

   // finally create toolbar and menubar
   createGUI();
}

KJumpingCube::~KJumpingCube()
{
   saveSettings();

   if(view)
   {
      if(view->isActive())
         view->stopActivities();
      delete view;
   }
}

void KJumpingCube::saveProperties(KConfig *config)
{
     config->writeEntry("Toolbar", showToolbar->isChecked());
     config->writeEntry("Statusbar", showStatusbar->isChecked());

     config->writeEntry("CubeDim",view->dim());
     config->writeEntry("Color1",view->color(KCubeBoxWidget::One).active().background());
     config->writeEntry("Color2",view->color(KCubeBoxWidget::Two).active().background());
     config->writeEntry("Skill",(int)view->skill());
     config->writeEntry("Computer Pl.1", changeComputer1->isChecked());
     config->writeEntry("Computer Pl.2", changeComputer2->isChecked());

     view->saveGame(config);
}

void KJumpingCube::toggleToolbar()
{
 if (!toolBar()->isHidden()) {
	toolBar()->hide();
 } else {
	toolBar()->show();
 }
}

void KJumpingCube::toggleStatusbar()
{
 if (!statusBar()->isHidden()) {
	statusBar()->hide();
 } else {
	statusBar()->show();
 }
}

void KJumpingCube::readProperties(KConfig *config)
{
   bool visible=config->readBoolEntry("Toolbar",true);
   if (toolBar()->isHidden() == visible)
      showToolbar->activate();

   visible=config->readNumEntry("Statusbar",true);
   if (statusBar()->isHidden() == visible)
      showStatusbar->activate();

   QColor c1=view->color(KCubeBoxWidget::One).active().background();
   QColor c2=view->color(KCubeBoxWidget::Two).active().background();
   QPalette color1(config->readColorEntry("Color1",&c1));
   QPalette color2(config->readColorEntry("Color2",&c2));
   view->setColor(KCubeBoxWidget::One,color1);
   view->setColor(KCubeBoxWidget::Two,color2);

   view->setDim(config->readNumEntry("CubeDim",view->dim()));
   updatePlayfieldMenu(view->dim());
   view->setSkill((Brain::Skill)config->readNumEntry("Skill",(int)view->skill()));
   updateSkillMenu((int)view->skill());

   bool flag=config->readBoolEntry("Computer Pl.1",false);
   view->setComputerplayer(KCubeBoxWidget::One,flag);
   changeComputer1->setChecked(flag);
   flag=config->readBoolEntry("Computer Pl.2",false);
   view->setComputerplayer(KCubeBoxWidget::Two,flag);
   changeComputer2->setChecked(flag);


   view->restoreGame(config);
}

void KJumpingCube::saveSettings()
{
  KConfig *config=kapp->config();
  {
     KConfigGroupSaver cfs(config,"Window");
     config->writeEntry("Toolbar", showToolbar->isChecked());
     config->writeEntry("Statusbar", showStatusbar->isChecked());
     config->writeEntry("Size",size());
  }
  {
     KConfigGroupSaver cfs2(config,"Game");
     config->writeEntry("CubeDim",view->dim());
     config->writeEntry("Color1",view->color(KCubeBoxWidget::One).active().background());
     config->writeEntry("Color2",view->color(KCubeBoxWidget::Two).active().background());
     config->writeEntry("Skill",(int)view->skill());
     config->writeEntry("Computer Pl.1", changeComputer1->isChecked());
     config->writeEntry("Computer Pl.2", changeComputer2->isChecked());
   }
   config->sync();

   statusBar()->message(i18n("settings saved"),MESSAGE_TIME);
}

void KJumpingCube::newGame()
{
   undoAction->setEnabled(false);
   view->reset();
   statusBar()->message(i18n("New Game"),MESSAGE_TIME);
}

void KJumpingCube::saveAs()
{
   saveGame(true);
}

void KJumpingCube::save()
{
   saveGame(false);
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
   config.writeEntry("CubeDim",view->dim());
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
         QString mes=i18n("The file %1 isn't a KJumpingCube gamefile!");
         mes=mes.arg(url.url());
         KMessageBox::sorry(this,mes);
         return;
      }

      gameURL=url;
      view->setDim(config.readNumEntry("CubeDim",5));
      config.setGroup("Game");
      view->restoreGame(&config);

      undoAction->setEnabled(false);

      KIO::NetAccess::removeTempFile( tempFile );
   }
   else
   {
      KMessageBox::sorry(this,i18n("There was an error loading file\n%1").arg( url.url() ));
   }
}

void KJumpingCube::getHint()
{
   view->getHint();
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

void KJumpingCube::changeColor(int player)
{
   QColor newColor;
   int result=KColorDialog::getColor(newColor);
   if(result)
   {
       QPalette color(newColor);
       view->setColor((KCubeBoxWidget::Player)player,color);

       QString s=i18n("color changed for player %1");
       s=s.arg(player);
       statusBar()->message(s,MESSAGE_TIME);
   }

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

void KJumpingCube::showWinner(int player)
{
   QString s=i18n("Winner is Player %1!").arg(player);
   KMessageBox::information(this,s,i18n("Winner"));
   view->reset();
}



void KJumpingCube::barPositionChanged()
{
  KConfig *config= kapp->config();
  KConfigGroupSaver cfs(config,"Window");
  config->writeEntry("ToolbarPos",(int)(toolBar()->barPos()));
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

void KJumpingCube::enableStop_Thinking()
{
   stopAction->setEnabled(true);
   hintAction->setEnabled(false);

   statusBar()->message(i18n("computing move"));
}


void KJumpingCube::skillChange()
{
 int index = optionSkill->currentItem();

 int newSkill=index;
 view->setSkill((Brain::Skill)(newSkill));

 updateSkillMenu(index);
 QString skillStr;
 switch(newSkill)
 {
   case 0:
      skillStr=i18n("Beginner");
      break;
   case 1:
      skillStr=i18n("Average");
      break;
   case 2:
      skillStr=i18n("Expert");
      break;
 }
 QString s=i18n("The computer player's skill level is now: %1").arg(skillStr);
 statusBar()->message(s,MESSAGE_TIME);
}

void KJumpingCube::fieldChange()
{
 int index = optionField->currentItem();
   // set new cubedim
 if(view->isActive())
	 return;

 if(view->dim() != 5+index)
 {
     view->setDim(5+index);
     updatePlayfieldMenu(5+index);
     undoAction->setEnabled(false);

     QString s=i18n("playfield changed to %1x%2");
            s=s.arg(5+index).arg(5+index);
     statusBar()->message(s,MESSAGE_TIME);
  }
}

void KJumpingCube::changeComputerPlayer1()
{
 bool flag = changeComputer1->isChecked();
 KCubeBoxWidget::Player player = (KCubeBoxWidget::Player)(1);

 QString s;
 if (flag) {
    s=i18n("Player %1 is now played by the computer.").arg(1);
 } else {
    s=i18n("Player %1 is now played by you.").arg(1);
 }
 statusBar()->message(s,MESSAGE_TIME);

 view->setComputerplayer(player, flag);

}
void KJumpingCube::changeComputerPlayer2()
{
 bool flag = changeComputer2->isChecked();
 KCubeBoxWidget::Player player = (KCubeBoxWidget::Player)(2);

 QString s;
 if (flag) {
    s=i18n("Player %1 is now played by the computer.").arg(1);
 } else {
    s=i18n("Player %1 is now played by you.").arg(1);
 }
 statusBar()->message(s,MESSAGE_TIME);

 view->setComputerplayer(player, flag);
}

void KJumpingCube::changeColor1()
{ changeColor(1); }
void KJumpingCube::changeColor2()
{ changeColor(2); }

void KJumpingCube::configureKeyBindings()
{
  KKeyDialog::configure(actionCollection(), this);
}

void KJumpingCube::updatePlayfieldMenu(int dim)
{
  optionField->setCurrentItem(dim-5);
}

void KJumpingCube::updateSkillMenu(int id)
{
  optionSkill->setCurrentItem(id);
}

bool KJumpingCube::queryClose()
{
   if(view->isActive())
      view->stopActivities();

   return true;
}

#include "kjumpingcube.moc"
