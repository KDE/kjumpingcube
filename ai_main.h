/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 2012 by Ian Wadham <iandw.au@gmail.com>

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
#ifndef AI_MAIN_H
#define AI_MAIN_H

#include <QList>
#include <QThread>

#include <krandomsequence.h>

#include "ai_base.h"

/* IDW TODO - Use a thread and return the move via a signal.
class ThreadedAI;
*/

class AI_Box;

struct Move
{
   int row;
   int col;
   int val;
};

// IDW TODO - Re-write the API documentation.

/**
* Class AI_Main computes a (good) possibility to move
* for a given playingfield.
*
* It puts a value on every cube by looking at its neighbours
* and searches the best cubes to move. It then simulates what would
* happen, if you would click on these cubes. This is done recursively
* to a certain depth and the playingfield will be valued.
*
* @short The games brain
*/
class AI_Main
{
public:

   void startStats();
   void postMove (Player player, int index);
   void dumpStats();

   /* IDW TODO - Use a thread and return the move via a signal.
   void computeMove();
   */

   /**
   * @param initValue value to initialize the random number generator with
   *        if no value is given a truly random value is used
   */
   explicit AI_Main();
   virtual ~AI_Main();

   // IDW TODO - It would be good to use const for CubeBox parameters where possible,
   //            but something back in cubeboxbase.h prevents that.

   /**
   * Computes a good possible move at the given field.
   * The index of this Cube is stored in given 'row' and 'column'
   *
   * @return false if computing was stopped
   * @see AI_Main#stop;
   */
   bool getMove (int & index, Player player, AI_Box * box);

   /**
    *  Stops the AI, but not till the end of the current cycle.
    */
   void stop();

   /**
    * @return true if the AI is running at the moment.
    */
   bool isActive() const;

   /**
    * Skill according to Prefs::EnumSkill
    */
   void setSkill (int skill1, bool kepler1, bool newton1,
                  int skill2, bool kepler2, bool newton2);

private:
   /* IDW TODO - Use a thread and return the move via a signal.
   ThreadedAI * m_thread;

   Move       m_move;
   */

   Player     m_player;

   AI_Box *   m_box;

   int        m_side;
   int        m_nCubes;
   int *      m_owners;
   int *      m_values;
   int *      m_maxValues;

   /**
   * Recursively checks and evaluates moves available at a given position, using
   * the MiniMax algorithm.
   *
   * @param player  Player for whom moves are being evaluated.
   * @param box     Position of the cubes in the cube box (a reference, not a copy).
   * @param level   Current level of recursion (i.e. level in move tree).
   *
   * @return        The best move found and the value of the position reached.
   */
   // IDW test. Move tryMoves (Player player, int side, int * owners, int * values,
                                                    // IDW test. int * maxValues, int level);
   Move tryMoves (Player player, int level);

   /**
   * Checks the given playingfield, which cubes are favorable to do a move
   * by checking every cubes neighbours. And looking for the difference to overflow.
   *
   * @param c2m Array in which the coordinates of the best cubes to move will be stored
   * @param player for which player to check
   * @param box playingfield to check
   * @param debug if debugmessages should be printed
   * @return number of found cubes to move
   */
   int findCubesToMove (Move * c2m, Player player, int side,
                        int * owners, int * values, int * maxValues);

   void boxPrint (int side, int * owners, int * values);

   AI_Base * m_AI_Kepler;
   AI_Base * m_AI_Newton;

   AI_Base * m_ai [3];
   AI_Base * m_currentAI;

   int m_ai_skill [3];
   int m_ai_maxLevel [3];

   /** Current depth of recursive searching for moves. */
   int  m_currentLevel;

   /** maximum depth of recursive thinking */
   int  m_maxLevel;

   /** the player for which to check the moves */
   Player currentPlayer;

   /** Flag, if the AI has to be stopped. */
   bool m_stopped;

   /** Flag, if the AI is running. */
   bool m_active;

   /** Skill of the AI, see Prefs::EnumSkill. */
   int  m_skill;

   /** Sequence generator */
   KRandomSequence m_random;

   int n_simulate;
   int n_assess;

   struct SearchStats {
      int n_moves;
   };

   struct MoveStats {
      Player player;
      int moveNo;
      int x;
      int y;
      int value;
      int n_simulate;
      int n_assess;
      QList<SearchStats *> * searchStats;
   };

   int m_currentMoveNo;
   MoveStats * m_currentMove;
   QList<MoveStats *> m_moveStats;

   QString tag (int level);
   void initStats (int player);
   void saveStats (Move & move);
};

/* IDW TODO - Use a thread and return the move via a signal.
class ThreadedAI : public QThread
{
public:
   ThreadedAI (AI_Main * ai)
      : m_ai (ai)
   { }

   virtual void run()
   {
      m_ai->computeMove();
   }

   void stop()
   {
   }
private:
   AI_Main * m_ai;
};
*/

#endif //AI_MAIN_H
