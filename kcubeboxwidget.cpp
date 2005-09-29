/* ****************************************************************************
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
#include "kcubeboxwidget.h"

#include <kapplication.h>
#include <kconfig.h>
#include <qlayout.h>
#include <qtimer.h>
#include <assert.h>
#include <kcursor.h>

#include "prefs.h"

KCubeBoxWidget::KCubeBoxWidget(const int d,QWidget *parent,const char *name)
        : QWidget(parent,name),
          CubeBoxBase<KCubeWidget>(d)
{
   init();
}



KCubeBoxWidget::KCubeBoxWidget(CubeBox& box,QWidget *parent,const char *name)
      :QWidget(parent,name),
       CubeBoxBase<KCubeWidget>(box.dim())
{  
   init();
   
   int i,j;
   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
         *cubes[i][j]=*box[i][j];
      }
      
   currentPlayer=(KCubeBoxWidget::Player)box.player();
}



KCubeBoxWidget::KCubeBoxWidget(const KCubeBoxWidget& box,QWidget *parent,const char *name)
      :QWidget(parent,name),
       CubeBoxBase<KCubeWidget>(box.dim())
{  
   init();
   
   int i,j;
   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
         *cubes[i][j]=*box.cubes[i][j];
      }
   
      
   currentPlayer=box.currentPlayer;
}



KCubeBoxWidget::~KCubeBoxWidget()
{
   if(isActive())
    stopActivities();
   if(cubes)
      deleteCubes();
   if(undoBox)
      delete undoBox;
}

void KCubeBoxWidget::loadSettings(){
  setColor(KCubeBoxWidget::One, Prefs::color1());
  setColor(KCubeBoxWidget::Two, Prefs::color2());

  setDim(Prefs::cubeDim());
  brain.setSkill( Prefs::skill() );

  setComputerplayer(KCubeBoxWidget::One, Prefs::computerPlayer1());
  setComputerplayer(KCubeBoxWidget::Two, Prefs::computerPlayer2());
  checkComputerplayer(currentPlayer);
}

KCubeBoxWidget& KCubeBoxWidget::operator=(const KCubeBoxWidget& box)
{
   if(this!=&box)
   {
      if(dim()!=box.dim())
      {
         setDim(box.dim());
      }
      
      
      for(int i=0;i<dim();i++)
         for(int j=0;j<dim();j++)
         {
            *cubes[i][j]=*box.cubes[i][j];
         }
         
      currentPlayer=box.currentPlayer;
   }
   return *this;
}

KCubeBoxWidget& KCubeBoxWidget::operator=(CubeBox& box)
{
   if(dim()!=box.dim())
   {
      setDim(box.dim());
   }
   
   for(int i=0;i<dim();i++)
      for(int j=0;j<dim();j++)
      {
         *cubes[i][j]=*box[i][j];
      }
      
   currentPlayer=(KCubeBoxWidget::Player)box.player();
      
   return *this;
}

void KCubeBoxWidget::reset()
{
   stopActivities();
   
   int i,j;
   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
         cubes[i][j]->reset();
      }
      
   KCubeWidget::enableClicks(true);

   currentPlayer=One;
   
   emit playerChanged(One);
   checkComputerplayer(One);
}

void KCubeBoxWidget::undo()
{
   if(isActive())
      return;
   
   Player oldPlayer=currentPlayer;
   
   *this=*undoBox;
   
   if(oldPlayer!=currentPlayer)
      emit playerChanged(currentPlayer);

   checkComputerplayer(currentPlayer);
  
}
      
void KCubeBoxWidget::getHint()
{
   if(isActive())
      return;

   int d=dim();
   for(int i=0;i<d;i++)
      for(int j=0;j<d;j++)
      {
         cubes[i][j]->stopHint();
      }

   int row=0,column=0;
   CubeBox field=*this;

   emit startedThinking();
   bool canceled=!brain.getHint(row,column,(CubeBox::Player)currentPlayer,field);
   emit stoppedThinking();

   if(canceled)
   {
      return;  // return if thinking was stopped
   }
   cubes[row][column]->showHint();
}

void KCubeBoxWidget::setColor(Player player,QPalette color)
{
   KCubeWidget::setColor((Cube::Owner)player,color);

   for(int row=0;row<dim();row++)
      for(int col=0;col<dim();col++)
      {
         cubes[row][col]->updateColors();
      }
}

void KCubeBoxWidget::setDim(int d)
{
   if(d != dim())
   {
      undoBox->setDim(d);
      CubeBoxBase<KCubeWidget>::setDim(d);
   }
}

void KCubeBoxWidget::setComputerplayer(Player player,bool flag)
{
   if(player==One)
      computerPlOne=flag;
   else if(player==Two)
      computerPlTwo=flag;
}

  
void KCubeBoxWidget::stopActivities()
{
   if(moveTimer->isActive())
   {
      stopLoop();
      emit stoppedMoving();
   }
   if(brain.isActive())
   {
      brain.stop();
      emit stoppedThinking();
   }

}

void KCubeBoxWidget::saveProperties(KConfigBase* config)
{
   if(isMoving())
   {
      stopActivities();
      undo();
   }
   else if(brain.isActive())
      stopActivities();

   // save current player
   config->writeEntry("onTurn",(int)currentPlayer);

   QStrList list;
   list.setAutoDelete(true);
   QString owner, value, key;
   int cubeDim=dim();

   for(int row=0; row < cubeDim ; row++)
     for(int column=0; column < cubeDim ; column++)
     {
	key.sprintf("%u,%u",row,column);
	owner.sprintf("%u",cubes[row][column]->owner());
	value.sprintf("%u",cubes[row][column]->value());
	list.append(owner.ascii());
	list.append(value.ascii());
	config->writeEntry(key , list);

	list.clear();
      }
  config->writeEntry("CubeDim",dim());
}

void KCubeBoxWidget::readProperties(KConfigBase* config)
{
  QStrList list;
  list.setAutoDelete(true);
  QString owner, value, key;
  setDim(config->readNumEntry("CubeDim",5));
  int cubeDim=dim();

  for(int row=0; row < cubeDim ; row++)
    for(int column=0; column < cubeDim ; column++)
      {
	key.sprintf("%u,%u",row,column);
	config->readListEntry(key, list);
	owner=list.first();
	value=list.next();
	cubes[row][column]->setOwner((KCubeWidget::Owner)owner.toInt());
	cubes[row][column]->setValue(value.toInt());
	
	list.clear();
      }


   // set current player
   int onTurn=config->readNumEntry("onTurn",1);
   currentPlayer=(Player)onTurn;
   emit playerChanged(onTurn);
   checkComputerplayer((Player)onTurn);
}

/* ***************************************************************** **
**                               slots                               **
** ***************************************************************** */ 
void KCubeBoxWidget::setWaitCursor()
{
   setCursor(KCursor::waitCursor());
}


  
void KCubeBoxWidget::setNormalCursor()
{
   setCursor(KCursor::handCursor());
}

void KCubeBoxWidget::stopHint()
{

   int d=dim();
   for(int i=0;i<d;i++)
      for(int j=0;j<d;j++)
      {
         cubes[i][j]->stopHint();
      }

}

bool KCubeBoxWidget::checkClick(int row,int column, bool isClick)
{
   if(isActive())
      return false;

   // make the game start when computer player is player one and user clicks
   if(isClick && currentPlayer == One && computerPlOne)
   {
      checkComputerplayer(currentPlayer);
      return false;
   }
   else if((Cube::Owner)currentPlayer==cubes[row][column]->owner() ||
		   cubes[row][column]->owner()==Cube::Nobody)
   {
      doMove(row,column);
      return true;
   }
   else
      return false;
}

void KCubeBoxWidget::checkComputerplayer(Player player)
{
   // checking if a process is running or the Widget isn't shown yet
   if(isActive() || !isVisibleToTLW())  
      return;
   if((player==One && computerPlOne && currentPlayer==One)
         || (player==Two && computerPlTwo && currentPlayer==Two))
   {
      KCubeWidget::enableClicks(false);

      CubeBox field(*this);
      int row=0,column=0;
      emit startedThinking();
      bool canceled=!brain.getHint(row,column,(CubeBoxBase<Cube>::Player)player,field);
      emit stoppedThinking();

      if(!canceled)
      {
         cubes[row][column]->showHint(500,2);

         bool result=checkClick(row,column,false);
		 assert(result);
      }
   }
      
}

/* ***************************************************************** **
**                         status functions                          **
** ***************************************************************** */
 
bool KCubeBoxWidget::isActive() const
{
   bool flag=false;
   if(moveTimer->isActive())
      flag=true;
   else if(brain.isActive())
      flag=true;
   
   return flag;
}  

bool KCubeBoxWidget::isMoving() const
{
   return moveTimer->isActive();
}

bool KCubeBoxWidget::isComputer(Player player) const
{
   if(player==One)
      return computerPlOne;
   else 
      return computerPlTwo;
}
  

int KCubeBoxWidget::skill() const
{
   return brain.skill();
}
   
QPalette KCubeBoxWidget::color(Player forWhom)
{
   return KCubeWidget::color((KCubeWidget::Owner)forWhom); 
}

/* ***************************************************************** **
**                   initializing functions                          **
** ***************************************************************** */
void KCubeBoxWidget::init()
{
   initCubes();

   undoBox=new CubeBox(dim());
   
   currentPlayer=One;
   moveDelay=100;
   moveTimer=new QTimer(this);
   computerPlOne=false;
   computerPlTwo=false;
   KCubeWidget::enableClicks(true);
   loadSettings();
   
   connect(moveTimer,SIGNAL(timeout()),SLOT(nextLoopStep()));
   connect(this,SIGNAL(startedThinking()),SLOT(setWaitCursor()));
   connect(this,SIGNAL(stoppedThinking()),SLOT(setNormalCursor()));
   connect(this,SIGNAL(startedMoving()),SLOT(setWaitCursor()));
   connect(this,SIGNAL(stoppedMoving()),SLOT(setNormalCursor()));
   connect(this,SIGNAL(playerWon(int)),SLOT(stopActivities()));

   setNormalCursor();

   emit playerChanged(One);
}
   
void KCubeBoxWidget::initCubes()
{
   const int s=dim();
   int i,j;
   
   // create Layout
   layout=new QGridLayout(this,s,s);
   

   for(i=0;i<s;i++)
   {
      layout->setRowStretch(i,1);
      layout->setColStretch(i,1);
   }         
   
   
   // create new cubes
   cubes = new KCubeWidget**[s];
   for(i=0;i<s;i++)
   {
      cubes[i]=new KCubeWidget*[s];
   }
   for(i=0;i<s;i++)
      for(j=0;j<s;j++)
      {
         cubes[i][j]=new KCubeWidget(this);
         cubes[i][j]->setCoordinates(i,j);
         layout->addWidget(cubes[i][j],i,j);
         cubes[i][j]->show();
         connect(cubes[i][j],SIGNAL(clicked(int,int,bool)),SLOT(stopHint()));
         connect(cubes[i][j],SIGNAL(clicked(int,int,bool)),SLOT(checkClick(int,int,bool)));
      }
   
   // initialize cubes  
   int max=dim()-1;
      
   cubes[0][0]->setMax(2);
   cubes[0][max]->setMax(2);
   cubes[max][0]->setMax(2);
   cubes[max][max]->setMax(2);
   
   for(i=1;i<max;i++)
   {
      cubes[i][0]->setMax(3);
      cubes[i][max]->setMax(3);
      cubes[0][i]->setMax(3);
      cubes[max][i]->setMax(3);
   }
   
   for(i=1;i<max;i++)
     for(j=1;j<max;j++)
      {
         cubes[i][j]->setMax(4);
      }
      
}

QSize  KCubeBoxWidget::sizeHint() const
{
   return QSize(400,400);
}

void  KCubeBoxWidget::deleteCubes()
{
   if(layout)
      delete layout;
     
   CubeBoxBase<KCubeWidget>::deleteCubes();
}
 

/* ***************************************************************** **
**                   other private functions                         **
** ***************************************************************** */

void KCubeBoxWidget::doMove(int row,int column)
{
   // if a move hasn't finished yet don't do another move
   if(isActive())
      return;

   // for undo-function copy field
   *undoBox=*this;

   cubes[row][column]->increase((Cube::Owner)currentPlayer);

   if(cubes[row][column]->overMax())
   {
      KCubeWidget::enableClicks(false);
      startLoop();
   }
   else
      changePlayer();      
}

void KCubeBoxWidget::startLoop()
{
   emit startedMoving();
   
   KCubeWidget::enableClicks(false);
   
   loop.row=0;
   loop.column=0;
   loop.finished=true;
   
   moveTimer->start(moveDelay);
}

void KCubeBoxWidget::stopLoop()
{
   moveTimer->stop();
   emit stoppedMoving();
   KCubeWidget::enableClicks(true);
}

void KCubeBoxWidget::nextLoopStep()
{
   // search cube with to many points
   while(!cubes[loop.row][loop.column]->overMax())
   {
      loop.column++;
      if(loop.column==dim())
      {
         if(loop.row==dim()-1)
	 {
	    if(!loop.finished)
	    {
               loop.row=0;
               loop.column=0;
               loop.finished=true;
               return;
	    }
	    else   // loop finished
	    {
	       stopLoop();
	       changePlayer();
		         
	       return;
            }
         }
         else
         {
            loop.row++;
            loop.column=0;
         }
      }
   }
    
 
   increaseNeighbours(currentPlayer,loop.row,loop.column);
   cubes[loop.row][loop.column]->decrease();
   loop.finished=false;
    
   if(hasPlayerWon(currentPlayer))
   {
      emit playerWon((int)currentPlayer);
      stopLoop();
      return;
   }
}

bool KCubeBoxWidget::hasPlayerWon(Player player)
{
   for(int i=0;i<dim();i++)
      for(int j=0;j<dim();j++)
      {
         if(cubes[i][j]->owner()!=(Cube::Owner)player)
         {
            return false;
         }
      }
   return true;
}

KCubeBoxWidget::Player KCubeBoxWidget::changePlayer()
{
   currentPlayer=(currentPlayer==One)? Two : One;

   emit playerChanged(currentPlayer);
   checkComputerplayer(currentPlayer);
   KCubeWidget::enableClicks(true);
   return currentPlayer;
}


void KCubeBoxWidget::increaseNeighbours(KCubeBoxWidget::Player forWhom,int row,int column)
{
   KCubeWidget::Owner _player = (KCubeWidget::Owner)(forWhom);

   if(row==0)
   {
    	if(column==0)  // top left corner
	{
	   cubes[0][1]->increase(_player);
	   cubes[1][0]->increase(_player);
	   return;
	}
	else if(column==dim()-1)  // top right corner
	{
	   cubes[0][dim()-2]->increase(_player);
	   cubes[1][dim()-1]->increase(_player);
	   return;
	}
	else  // top edge
	{
	   cubes[0][column-1]->increase(_player);
	   cubes[0][column+1]->increase(_player);
	   cubes[1][column]->increase(_player);
	   return;
	}
   }
   else if(row==dim()-1)
   {
      if(column==0)  // left bottom corner
      {
         cubes[dim()-2][0]->increase(_player);
         cubes[dim()-1][1]->increase(_player);
         return;
      }

      else if(column==dim()-1) // right bottom corner
      {
         cubes[dim()-2][dim()-1]->increase(_player);
         cubes[dim()-1][dim()-2]->increase(_player);
	      return;
      }
      else  // bottom edge
      {
 	      cubes[dim()-1][column-1]->increase(_player);
	      cubes[dim()-1][column+1]->increase(_player);
	      cubes[dim()-2][column]->increase(_player);
	      return;
      }
   }
   else if(column==0) // left edge
   {
      cubes[row-1][0]->increase(_player);
      cubes[row+1][0]->increase(_player);
      cubes[row][1]->increase(_player);
      return;
   }
   else if(column==dim()-1)  // right edge
   {
      cubes[row-1][dim()-1]->increase(_player);
      cubes[row+1][dim()-1]->increase(_player);
      cubes[row][dim()-2]->increase(_player);
      return;
   }
   else
   {
      cubes[row][column-1]->increase(_player);
      cubes[row][column+1]->increase(_player);
      cubes[row-1][column]->increase(_player);
      cubes[row+1][column]->increase(_player);
      return;
   }


}

#include "kcubeboxwidget.moc"

