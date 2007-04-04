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
#ifndef KCUBEBOXWIDGET_H
#define KCUBEBOXWIDGET_H

#include "cubeboxbase.h"
#include "kcubewidget.h"
#include "brain.h"
#include <qwidget.h>

class QGridLayout;
class CubeBox;
class QPalette;
class QTimer;
class KConfigBase;

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/**
*@internal
*/
struct Loop
{
   int row;
   int column;
   bool finished;
};


class KCubeBoxWidget : public QWidget , public CubeBoxBase<KCubeWidget>
{
   Q_OBJECT
public:
   KCubeBoxWidget(const int dim=1,QWidget *parent=0,const char *name=0);
      
   KCubeBoxWidget(CubeBox& box, QWidget *parent=0,const char *name=0);
   KCubeBoxWidget(const KCubeBoxWidget& box,QWidget *parent=0,const char *name=0);
   virtual ~KCubeBoxWidget();
   
   KCubeBoxWidget& operator= (CubeBox& box);
   KCubeBoxWidget& operator= ( const KCubeBoxWidget& box);
   
   /**
   * reset cubebox for a new game
   */
   void reset();
  
   /** undo last move */
   void undo();
   
   /**
   * set colors that are used to show owners of the cubes
   *
   * @param forWhom for which player the color should be set
   * @param color  color for player one
   */
   void setColor(Player forWhom,QPalette color);
   /**
   * sets number of Cubes in a row/column to 'size'.
   */
   virtual void setDim(int dim);
  
   /**
   * sets player 'player' as computer or human
   *
   * @param player 
   * @param flag: true for computer, false for human
   */
   void setComputerplayer(Player player,bool flag);
    
   /** returns current skill, according to Prefs::EnumSkill */
   int skill() const;

   /** returns true if player 'player' is a computerPlayer */
   bool isComputer(Player player) const;
  
   /** returns true if CubeBox is doing a move or getting a hint */
   bool isActive() const;
   bool isMoving() const;

   /** returns current Color for Player ´forWhom´ */
   QPalette color(Player forWhom);

   /**
   * checks if 'player' is a computerplayer an computes next move if TRUE
   */
   void checkComputerplayer(Player player);
 
   inline void saveGame(KConfigBase *c) { saveProperties(c); }
   inline void restoreGame(KConfigBase *c) { readProperties(c); } 
  
public slots:
   /** stops all activities like getting a hint or doing a move */
   void stopActivities();
   /**
    * computes a possibility to move and shows it by highlightning
    * this cube
    */
   void getHint();
  
  void loadSettings();
  
signals:
   void playerChanged(int newPlayer);
   void playerWon(int player);
   void startedMoving();
   void startedThinking();
   void stoppedMoving();
   void stoppedThinking();

protected:
   virtual QSize sizeHint() const;
   virtual void deleteCubes();
   virtual void initCubes(); 

   void saveProperties(KConfigBase *);
   void readProperties(KConfigBase *);

protected slots:
   /** sets the cursor to an waitcursor */
   void setWaitCursor();
   /** restores the original cursor */
   void setNormalCursor();
  
private:
   void init();
 
   QGridLayout *layout;
   CubeBox *undoBox;
   Brain brain;
   
   QTimer *moveTimer;
   int moveDelay;
   Loop loop;
   /** */
   void startLoop();
   /** */
   void stopLoop();
  
   Player changePlayer();
   bool hasPlayerWon(Player player); 
   bool computerPlOne;
   bool computerPlTwo;
  
   /**
   * increases the cube at row 'row' and column 'column' ,
   * and starts the Loop for checking the playingfield 
   */
   void doMove(int row,int column);

   void increaseNeighbours(KCubeBoxWidget::Player forWhom,int row,int column);

private slots:   
   void nextLoopStep();
   /**
   * checks if cube at ['row','column'] is clickable by the current player.
   * if true, it increases this cube and checks the playingfield  
   */
   bool checkClick(int row,int column,bool isClick);

   /** turns off blinking, if an other cube is clicked */
   void stopHint();

};

#endif // KCUBEBOXWIDGET_H

