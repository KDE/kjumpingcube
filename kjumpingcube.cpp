/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 1998,1999 by Matthias Kiefer
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

#include <kapp.h>
#include <kglobal.h>
#include <klocale.h>
#include <kaccel.h>
#include <kkeydialog.h>
#include <kiconloader.h>
#include <qpopupmenu.h>
#include <kcolordlg.h>
#include <kfiledialog.h>
#include <ksimpleconfig.h>
#include <qfileinfo.h>
#include <qregexp.h>
#include <qmessagebox.h>
#include <kmenubar.h>


#define ID_GAME_QUIT 1
#define ID_GAME_NEW 2
#define ID_GAME_OPEN 3
#define ID_GAME_SAVE 4
#define ID_GAME_SAVEAS 5
#define ID_GAME_HINT 6
#define ID_GAME_STOP_HINT 7
#define ID_GAME_UNDO 8
#define ID_HELP_ABOUT 101
#define ID_HELP_CONTENTS 102
#define ID_VIEW_TOOLBAR 201
#define ID_VIEW_STATUSBAR 202
#define ID_OPT_SAVE 301
#define ID_OPT_KEYS 302
#define ID_COLOR_BASE 400
#define ID_FIELD 500
#define ID_SKILL_BASE 600
#define ID_COMPUTER_BASE 700
#define ID_STATUS_TURN 1000


#define MESSAGE_TIME 2000

KJumpingCube::KJumpingCube()
	: view(new KCubeBoxWidget(5,this))
{
   saveSettingsOnExit=false;


   connect(view,SIGNAL(playerChanged(int)),SLOT(changePlayer(int)));
   connect(view,SIGNAL(stoppedMoving()),SLOT(disableStop()));	
   connect(view,SIGNAL(stoppedThinking()),SLOT(disableStop()));	
   connect(view,SIGNAL(startedMoving()),SLOT(enableStop_Moving()));		
   connect(view,SIGNAL(startedThinking()),SLOT(enableStop_Thinking()));	
   connect(view,SIGNAL(playerWon(int)),SLOT(showWinner(int)));

   // tell the KTMainWindow that this is indeed the main widget
   setView(view);

   // init keys
   kaccel = new KAccel(this);

   kaccel->insertStdItem(KAccel::New,i18n("New Game"));
   kaccel->connectItem(KAccel::New,this,SLOT(newGame()));
   kaccel->connectItem(KAccel::Open,this,SLOT(openGame()));
   kaccel->connectItem(KAccel::Save,this,SLOT(save()));
   kaccel->connectItem(KAccel::Quit,this,SLOT(quit()));

   kaccel->insertItem(i18n("Get Hint"),"Get Hint","CTRL+H");
   kaccel->connectItem("Get Hint",this,SLOT(getHint()));
   kaccel->insertItem(i18n("Stop Thinking"),"Stop Thinking","Escape");
   kaccel->connectItem("Stop Thinking",this,SLOT(stop()));
   kaccel->insertStdItem(KAccel::Undo,i18n("Undo Move"));
   kaccel->connectItem(KAccel::Undo,this,SLOT(undo()));

   KConfig *config=kapp->config();
   kaccel->readSettings();

   // init menubar
   QPopupMenu *p = new QPopupMenu(this);
   int id;
   id=p->insertItem(i18n("&New Game"),this,SLOT(newGame()));
   kaccel->changeMenuAccel(p,id,KAccel::New);
   id=p->insertItem(i18n("&Open..."),this,SLOT(openGame()));
   kaccel->changeMenuAccel(p,id,KAccel::Open);
   p->insertSeparator();
   id=p->insertItem(i18n("&Save"),this,SLOT(save()));
   kaccel->changeMenuAccel(p,id,KAccel::Save);
   p->insertItem(i18n("Save &As..."),this,SLOT(saveAs()));
   p->insertSeparator();
   id=p->insertItem(i18n("&Quit"),this,SLOT(quit()));
   kaccel->changeMenuAccel(p,id,KAccel::Quit);

   connect(p,SIGNAL(activated(int)),SLOT(menuCallback(int)));

   // put our newly created menu into the main menu bar
   menuBar()->insertItem(i18n("&File"), p);


   game=new QPopupMenu(this);
   game->setCheckable(TRUE);
   game->insertItem(i18n("Get &Hint"),ID_GAME_HINT);
   kaccel->changeMenuAccel(game,ID_GAME_HINT,"Get Hint");
   game->insertItem(i18n("&Stop Thinking"),ID_GAME_STOP_HINT);
   kaccel->changeMenuAccel(game,ID_GAME_STOP_HINT,"Stop Thinking");
   game->setItemEnabled(ID_GAME_STOP_HINT,FALSE);
   game->insertSeparator();
   game->insertItem(i18n("&Undo Move"),ID_GAME_UNDO);
   kaccel->changeMenuAccel(game,ID_GAME_UNDO,"Undo");

   game->setItemEnabled(ID_GAME_UNDO,FALSE);

   connect(game,SIGNAL(activated(int)),SLOT(menuCallback(int)));
   menuBar()->insertItem( i18n("&Game"),game );


   playfieldMenu=new QPopupMenu(this);
   playfieldMenu->setCheckable(TRUE);
   playfieldMenu->insertItem("&5x5",ID_FIELD +5);
   playfieldMenu->insertItem("&6x6",ID_FIELD +6);
   playfieldMenu->insertItem("&7x7",ID_FIELD +7);
   playfieldMenu->insertItem("&8x8",ID_FIELD +8);
   playfieldMenu->insertItem("&9x9",ID_FIELD +9);
   playfieldMenu->insertItem("&10x10",ID_FIELD +10);

   connect(playfieldMenu,SIGNAL(activated(int)),SLOT(menuCallback(int)));


   skillMenu=new QPopupMenu(this);
   skillMenu->setCheckable(TRUE);
   skillMenu->insertItem(i18n("&Beginner"),ID_SKILL_BASE+0);
   skillMenu->insertItem(i18n("&Average"),ID_SKILL_BASE+1);
   skillMenu->insertItem(i18n("&Expert"),ID_SKILL_BASE+2);

   connect(skillMenu,SIGNAL(activated(int)),SLOT(menuCallback(int)));

   options = new QPopupMenu(this);
   options->setCheckable(TRUE);

   options->insertItem(i18n("Show &Toolbar"),ID_VIEW_TOOLBAR);
   options->insertItem(i18n("Show St&atusbar"),ID_VIEW_STATUSBAR);
   options->insertSeparator();
   options->insertItem(i18n("&Configure Key Bindings..."),ID_OPT_KEYS);
   options->insertItem(i18n("&Playfield"),playfieldMenu);
   options->insertItem(i18n("S&kill"),skillMenu);
   options->insertItem(i18n("Computer plays Player &1"),ID_COMPUTER_BASE+1);
   options->insertItem(i18n("Computer plays Player &2"),ID_COMPUTER_BASE+2);
   options->insertItem(i18n("Color Player 1..."),ID_COLOR_BASE+1);
   options->insertItem(i18n("Color Player 2..."),ID_COLOR_BASE+2);
   options->insertSeparator();
   options->insertItem(i18n("&Save Settings"),ID_OPT_SAVE);


   connect(options,SIGNAL(activated(int)),SLOT(menuCallback(int)));
   menuBar()->insertItem(i18n("&Options"), options);

   QString s;
   s =i18n("KJumpingCube Version %1 \n\n (C) 1998,1999 by Matthias Kiefer \nmatthias.kiefer@gmx.de\n\n").arg(KJC_VERSION);
   s+=i18n(
     "This program is free software; you can redistribute it\n"
     "and/or modify it under the terms of the GNU General\n"
     "Public License as published by the Free Software\n"
     "Foundation; either version 2 of the License, or\n"
     "(at your option) any later version.");
   QPopupMenu *help = helpMenu(s);

   menuBar()->insertSeparator();
   menuBar()->insertItem( i18n("&Help"), help );

   connect(menuBar(),SIGNAL(moved(menuPosition)),SLOT(barPositionChanged()));


   // init toolbar
   KIconLoader *loader = KGlobal::iconLoader();

   // newGamebutton
   toolBar()->insertButton(loader->loadIcon("filenew.xpm"),ID_GAME_NEW,
			true,i18n("New Game"));
   toolBar()->insertButton(loader->loadIcon("fileopen.xpm"),ID_GAME_OPEN,
                        true,i18n("Open Game"));
   toolBar()->insertButton(loader->loadIcon("filefloppy.xpm"),ID_GAME_SAVE,
                        true,i18n("Save Game"));

   toolBar()->insertSeparator();

   toolBar()->insertButton(loader->loadIcon("stop.xpm"),ID_GAME_STOP_HINT,
		  true,i18n("Stop Thinking"));
   toolBar()->setItemEnabled(ID_GAME_STOP_HINT,false);

   toolBar()->insertButton(loader->loadIcon("idea.xpm"),ID_GAME_HINT,
		  true,i18n("Get Hint"));

   // undo-button
   toolBar()->insertButton(loader->loadIcon("back.xpm"),ID_GAME_UNDO,true,
		         i18n("Undo Move"));
   toolBar()->setItemEnabled(ID_GAME_UNDO,false);
   toolBar()->insertSeparator();

   //helpbutton
   toolBar()->insertButton(loader->loadIcon("help.xpm"),ID_HELP_CONTENTS,true,
			i18n("Help"));


   connect(toolBar(), SIGNAL(clicked(int)), this, SLOT(menuCallback(int)));
   connect(toolBar(),SIGNAL(moved(BarPosition)),SLOT(barPositionChanged()));





   // init statusbar
   s = i18n("on turn: Player %1").arg(1);
   statusBar()->insertItem(s+i18n("(Computer)"),ID_STATUS_TURN);
   statusBar()->changeItem(s,ID_STATUS_TURN);

  

   // read config
   {
      KConfigGroupSaver cgs(config,"Window");

      int barPos = config->readNumEntry("ToolbarPos",(int)(KToolBar::Top));
      toolBar()->setBarPos((KToolBar::BarPosition)barPos);
	
      bool visible=config->readBoolEntry("Toolbar",true);
      options->setItemChecked(ID_VIEW_TOOLBAR,visible);
      if(visible)
	 enableToolBar(KToolBar::Show);
      else
	 enableToolBar(KToolBar::Hide);
	
      visible=config->readNumEntry("Statusbar",true);
      options->setItemChecked(ID_VIEW_STATUSBAR,visible);
      if(visible)
         enableStatusBar(KStatusBar::Show);
      else
         enableStatusBar(KStatusBar::Hide);

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
      options->setItemChecked(ID_COMPUTER_BASE+1,flag);
      flag=config->readBoolEntry("Computer Pl.2",false);
      view->setComputerplayer(KCubeBoxWidget::Two,flag);
      options->setItemChecked(ID_COMPUTER_BASE+2,flag);
   }

   // disable undo-function
   game->setItemEnabled(ID_GAME_UNDO,false);
   toolBar()->setItemEnabled(ID_GAME_UNDO,false);
}

KJumpingCube::~KJumpingCube()
{
   if(view)
   {
      if(view->isActive())
         view->stopActivities();
      delete view;
   }
}




void KJumpingCube::saveProperties(KConfig *config)
{
     bool status=options->isItemChecked(ID_VIEW_TOOLBAR);
     config->writeEntry("Toolbar",status);
     status=options->isItemChecked(ID_VIEW_STATUSBAR);
     config->writeEntry("Statusbar",status);
	
     config->writeEntry("CubeDim",view->dim());
     config->writeEntry("Color1",view->color(KCubeBoxWidget::One).normal().background());
     config->writeEntry("Color2",view->color(KCubeBoxWidget::Two).normal().background());
     config->writeEntry("Skill",(int)view->skill());
     config->writeEntry("Computer Pl.1",options->isItemChecked(ID_COMPUTER_BASE+1));
     config->writeEntry("Computer Pl.2",options->isItemChecked(ID_COMPUTER_BASE+2));

     view->saveGame(config);
}

void KJumpingCube::readProperties(KConfig *config)
{
   bool visible=config->readBoolEntry("Toolbar",true);
   options->setItemChecked(ID_VIEW_TOOLBAR,visible);
   visible=config->readNumEntry("Statusbar",true);
   options->setItemChecked(ID_VIEW_STATUSBAR,visible);

   QColor c1=view->color(KCubeBoxWidget::One).normal().background();
   QColor c2=view->color(KCubeBoxWidget::Two).normal().background();
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
   options->setItemChecked(ID_COMPUTER_BASE+1,flag);
   flag=config->readBoolEntry("Computer Pl.2",false);
   view->setComputerplayer(KCubeBoxWidget::Two,flag);
   options->setItemChecked(ID_COMPUTER_BASE+2,flag);


   view->restoreGame(config);

}

void KJumpingCube::quit()
{
   if(saveSettingsOnExit)
   {
      saveSettings();
   }
   view->stopActivities();
   kapp->quit();
}

void KJumpingCube::saveSettings()
{
  KConfig *config=kapp->config();
  {
     KConfigGroupSaver cfs(config,"Window");
     bool status=options->isItemChecked(ID_VIEW_TOOLBAR);
     config->writeEntry("Toolbar",status);
     status=options->isItemChecked(ID_VIEW_STATUSBAR);
     config->writeEntry("Statusbar",status);
     config->writeEntry("Size",size());
  }
  {
     KConfigGroupSaver cfs2(config,"Game");
     config->writeEntry("CubeDim",view->dim());
     config->writeEntry("Color1",view->color(KCubeBoxWidget::One).normal().background());
     config->writeEntry("Color2",view->color(KCubeBoxWidget::Two).normal().background());
     config->writeEntry("Skill",(int)view->skill());
     config->writeEntry("Computer Pl.1",options->isItemChecked(ID_COMPUTER_BASE+1));     
     config->writeEntry("Computer Pl.2",options->isItemChecked(ID_COMPUTER_BASE+2));
   }
   config->sync();

   statusBar()->message(i18n("settings saved"),MESSAGE_TIME);
}


void KJumpingCube::newGame()
{
   game->setItemEnabled(ID_GAME_UNDO,false);
   toolBar()->setItemEnabled(ID_GAME_UNDO,false);
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
   if(saveAs || filename.isEmpty())
   {
      int result=0;
      QString temp;
      do
      {
         temp=KFileDialog::getSaveFileName(gameDir,"*.kjc",this,0);
         if(temp.isEmpty())
            return;


         // check filename
         QRegExp pattern("*.kjc",true,true);
         if(pattern.match(temp)==-1)
         {
            temp+=".kjc";
         }
         QFileInfo file(temp);
         gameDir=file.filePath();
         if(file.exists() && file.isWritable())
         {
            QString mes=i18n("The file %1 exists.\n"
			     "Do you want to override it?").arg(temp);
	    result = QMessageBox::information(this, kapp->caption(), 
					     mes, i18n("Yes"), i18n("No"));
            if(result==2)
               return;
         }
      }
      while(result==1);

      filename=temp;
   }

   KSimpleConfig config(filename);

   config.setGroup("KJumpingCube");
   config.writeEntry("Version",KJC_VERSION);
   config.writeEntry("CubeDim",view->dim());
   config.setGroup("Game");
   view->saveGame(&config);
   config.sync();

   QString s=i18n("game saved as %1");
   s=s.arg(filename);
   statusBar()->message(s,MESSAGE_TIME);
}

void KJumpingCube::openGame()
{
   bool fileOk=true;
   QString temp;
   do
   {
      temp=KFileDialog::getOpenFileName(gameDir,"*.kjc",this,0);
      if(temp.isEmpty() )
         return;

      QFileInfo file(temp);
      gameDir=file.filePath();
      if(!file.isReadable())
      {
         QString mes=i18n("The file %1 doesn't exists or isn't readable!").arg(temp);
         QMessageBox::information(this,kapp->caption(), mes, i18n("OK"));
         return;
      }
   }
   while(!fileOk);

   KSimpleConfig config(temp,true);
   config.setGroup("KJumpingCube");
   if(!config.hasKey("Version"))
   {
      QString mes=i18n("The file %1 isn't a KJumpingCube gamefile!");
      mes=mes.arg(temp);
      QMessageBox::information(this,kapp->caption(),mes,i18n("OK"));
      return;
   }

   filename=temp;
   view->setDim(config.readNumEntry("CubeDim",5));
   config.setGroup("Game");
   view->restoreGame(&config);

   game->setItemEnabled(ID_GAME_UNDO,false);
   toolBar()->setItemEnabled(ID_GAME_UNDO,false);
}

void KJumpingCube::getHint()
{
   view->getHint();
}

void KJumpingCube::stop()
{
   if(view->isMoving())
   {
      game->setItemEnabled(ID_GAME_UNDO,true);
      toolBar()->setItemEnabled(ID_GAME_UNDO,true);
   }

   view->stopActivities();

   statusBar()->message(i18n("stopped activity"),MESSAGE_TIME);
}

void KJumpingCube::undo()
{
   if(view->isActive())
      return;
   view->undo();
   game->setItemEnabled(ID_GAME_UNDO,false);
   toolBar()->setItemEnabled(ID_GAME_UNDO,false);
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
   QString s=i18n("on turn: Player %1");
   s=s.arg(newPlayer);
   if(view->isComputer((KCubeBoxWidget::Player)newPlayer))
      s+=i18n("(Computer)");

   statusBar()->changeItem(s,ID_STATUS_TURN);

   game->setItemEnabled(ID_GAME_UNDO,true);
   toolBar()->setItemEnabled(ID_GAME_UNDO,true);

}

void KJumpingCube::showWinner(int player)
{
   QString s=i18n("Winner is Player %1!").arg(player);
   QMessageBox::information(this,i18n("Winner"), s, i18n("OK"));
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
   toolBar()->setItemEnabled(ID_GAME_STOP_HINT,false);
   game->setItemEnabled(ID_GAME_STOP_HINT,false);
   toolBar()->setItemEnabled(ID_GAME_HINT,true);
   game->setItemEnabled(ID_GAME_HINT,true);

   statusBar()->clear();
}


void KJumpingCube::enableStop_Moving()
{
   toolBar()->setItemEnabled(ID_GAME_STOP_HINT,true);
   game->setItemEnabled(ID_GAME_STOP_HINT,true);
   toolBar()->setItemEnabled(ID_GAME_HINT,false);
   game->setItemEnabled(ID_GAME_HINT,false);

   statusBar()->message(i18n("doing move"));

}

void KJumpingCube::enableStop_Thinking()
{
   toolBar()->setItemEnabled(ID_GAME_STOP_HINT,true);
   game->setItemEnabled(ID_GAME_STOP_HINT,true);
   toolBar()->setItemEnabled(ID_GAME_HINT,false);
   game->setItemEnabled(ID_GAME_HINT,false);

   statusBar()->message(i18n("computing move"));

}



void KJumpingCube::menuCallback(int item)
{
   switch(item)
   {
      case ID_GAME_NEW:
         newGame();
         break;
      case ID_GAME_OPEN:
         openGame();
         break;
      case ID_GAME_SAVE:
         saveGame();
         break;
      case ID_GAME_HINT:
         getHint();
         break;
      case ID_GAME_STOP_HINT:
         stop();
         break;
      case ID_GAME_UNDO:
        undo();
        break;
      case ID_COMPUTER_BASE+1:
      case ID_COMPUTER_BASE+2:
      {
	 bool flag=options->isItemChecked(item);
	 flag= flag ? false : true;
	 options->setItemChecked(item,flag);
	 KCubeBoxWidget::Player player;
	 player=(KCubeBoxWidget::Player)(item-ID_COMPUTER_BASE);
	
	 QString s;
	 if(!flag)
	 {
	    s=i18n("Player %1 is now played by you.").arg(item-ID_COMPUTER_BASE);
	 }
	 else
	 {
	    s=i18n("Player %1 is now played by the computer.").arg(item-ID_COMPUTER_BASE);
	 }
         statusBar()->message(s,MESSAGE_TIME);
	
	 view->setComputerplayer(player,flag);
	
	 break;
      }
      case ID_SKILL_BASE+0:
      case ID_SKILL_BASE+1:
      case ID_SKILL_BASE+2:
      {
         int newSkill=item-ID_SKILL_BASE;
         view->setSkill((Brain::Skill)(newSkill));

         updateSkillMenu(item-ID_SKILL_BASE);
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
         QString s=i18n("Skill of computerplayer is now: %1").arg(skillStr);
	 statusBar()->message(s,MESSAGE_TIME);

         break;
      }          	
      case ID_VIEW_TOOLBAR:
      {
	enableToolBar(); // toggles toolbar
	bool status=options->isItemChecked(ID_VIEW_TOOLBAR);
	options->setItemChecked(ID_VIEW_TOOLBAR, status?false:true);
	break;
      }
      case ID_VIEW_STATUSBAR:
      {
	 enableStatusBar(); // toggles statusbar
	 bool status=options->isItemChecked(ID_VIEW_STATUSBAR);
	 options->setItemChecked(ID_VIEW_STATUSBAR, status?false:true);
	 break;
      }
      case ID_OPT_SAVE:
         saveSettings();
         break;

      case ID_COLOR_BASE+1:
      case ID_COLOR_BASE+2:
         changeColor(item-ID_COLOR_BASE);
         break;

       // set new cubedim
       case (ID_FIELD+5):
       case (ID_FIELD+6):
       case (ID_FIELD+7):
       case (ID_FIELD+8):
       case (ID_FIELD+9):
       case (ID_FIELD+10):
       {
	 if(view->isActive())
		 break;
	
	 if(view->dim() != item-ID_FIELD)
	 {
	     view->setDim(item-ID_FIELD);
	     updatePlayfieldMenu(item-ID_FIELD);
	     game->setItemEnabled(ID_GAME_UNDO,false);
	     toolBar()->setItemEnabled(ID_GAME_UNDO,false);
	
	     QString s=i18n("playfield changed to %1x%2");
             s=s.arg(item-ID_FIELD).arg(item-ID_FIELD);
	     statusBar()->message(s,MESSAGE_TIME);
	  }
	 break;
      }
    case ID_OPT_KEYS:
    {
       if(KKeyDialog::configureKeys(kaccel))
       {
	  kaccel->changeMenuAccel(game,ID_GAME_HINT,"Get Hint");
   	  kaccel->changeMenuAccel(game,ID_GAME_STOP_HINT,"Stop Thinking");
   	  kaccel->changeMenuAccel(game,ID_GAME_UNDO,"Undo");
       }
       break;
    }
    case ID_HELP_CONTENTS:
       kapp->invokeHTMLHelp("","");
    } // end switch
}

void KJumpingCube::updatePlayfieldMenu(int dim)
{
  for(int i=5;i<=10;i++)
    playfieldMenu->setItemChecked(ID_FIELD +i ,false);

  playfieldMenu->setItemChecked(ID_FIELD +dim,true);
}

void KJumpingCube::updateSkillMenu(int id)
{
  for(int i=0;i<=3;i++)
    skillMenu->setItemChecked(ID_SKILL_BASE+i ,false);

  skillMenu->setItemChecked(ID_SKILL_BASE+id,true);

}

bool KJumpingCube::queryClose()
{
   if(view->isActive())
      view->stopActivities();

   return true;
}
