/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 1998-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
  Copyright (C) 2012-2013 by Ian Wadham      <iandw.au@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************** */
#ifndef GAME_H
#define GAME_H

#include "ai_globals.h"

#include <QTime> // IDW

#include <KUrl>
#include <QList>

class KConfigGroup;
class KCubeBoxWidget;
class AI_Main;
class AI_Box;
class QTimer;

enum Action {NEW, HINT, BUTTON, UNDO, REDO, SAVE, SAVE_AS, LOAD};

class Game : public QObject
{
   Q_OBJECT
public:
   explicit Game (const int dim = 1, KCubeBoxWidget * parent = 0);

   virtual ~Game();

   /**
    * Make sure all animation and AI activity is over before destroying game.
    * */
   void shutdown();

   /**
   * reset cubebox for a new game
   */
   void reset();

   /**
    * Undo or redo a move.
    *
    * @param   actionType  -1 = undo, +1 = redo
    *
    * @return  -1 = Busy, 0 = No more to undo/redo, 1 = More moves to undo/redo.
    */
   int undoRedo (int actionType);

   /**
   * Set the number of cubes in a row or column.  If the number has changed,
   * delete the existing set of cubes and create a new one.
   */
   virtual void setDim (int dim);

   /**
   * sets player 'player' as computer or human
   *
   * @param player
   * @param flag: true for computer, false for human
   */
   void setComputerplayer(Player player,bool flag);

   /** returns true if player 'player' is a computerPlayer */
   bool isComputer(Player player) const;

   /** returns true if CubeBox is doing a move or getting a hint */
   bool isActive() const;
   bool isMoving() const;

   /**
   * checks if 'player' is a computerplayer an computes next move if TRUE
   */
   void checkComputerplayer(Player player);

   inline void restoreGame(const KConfigGroup&c) { readProperties(c); }

public slots:

   void gameActions (const int action);

   /** stops all activities like getting a hint or doing a move */
   void stopActivities();

   void loadSettings();

   void computerMoveDone (int index);

   void buttonClick();

signals:
   void playerChanged (int newPlayer);

   void buttonChange (bool enabled, bool stop = false,
                      const QString & caption = QString(""));
   void setAction (const Action a, const bool onOff);

protected:
   void saveProperties(KConfigGroup&);
   void readProperties(const KConfigGroup&);

private:
   QTime t; // IDW test.

   enum State        {NotReady, HumanMoving, ComputerMoving, Hinting, Waiting};
   enum Activity     {Idle, Computing, ShowingMove, AnimatingMove};

   enum WaitingState {Nil, WaitingToUndo, WaitingToLoad, WaitingToSave,
                      WaitingToStep, ComputerToMove};

   State              m_state;
   Activity           m_activity;

   WaitingState       m_waitingState;

   KCubeBoxWidget * m_view;
   AI_Box * m_box;
   int      m_side;
   Player   m_currentPlayer;
   bool     m_gameHasBeenWon;
   QList<int> * m_steps;

   AI_Main * m_ai;

   int  m_index;
   bool m_fullSpeed;

   bool   computerPlOne;
   bool   computerPlTwo;

   bool   m_pauseForComputer;
   Player m_playerWaiting;
   bool   m_pauseForStep;
   bool   m_waitingForStep;
   bool   m_ignoreComputerMove;

   KUrl   m_gameURL;

   void   newGame();
   void   saveGame (bool saveAs);
   void   loadGame();
   void   undo();
   void   redo();

   void computeMove (State moveType);

   /**
   * Increases the cube at 'index' and starts the animation loop, if required.
   */
   void   doMove (int index);
   void   doStep();

   Player changePlayer();

private slots:
   /**
   * Check if cube at [x, y] is available to the current (human) player.
   * If it is, make a move using the cube at [x, y].
   */
   void   mouseClick (int x, int y);
   void   animationDone (int index);
};

#endif // GAME_H
