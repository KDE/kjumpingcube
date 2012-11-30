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
#include <QMutex>

#include <krandomsequence.h>

#include "ai_base.h"

class ThreadedAI;
class AI_Box;

struct Move
{
   int row;
   int col;
   int val;
};

// IDW TODO - Re-write the API documentation.

/**
* Class AI_Main computes a (good) possible move from a given position.
*
* It puts a value on every cube by looking at its neighbours and picks a
* short list of likely cubes to move, because it takes too long to check
* every possible move.  It then simulates what would happen if you click
* on these cubes.  This is all done recursively to a certain depth and the
* position is valued.  The move that leads to the best position is chosen.
*
* @short The "brain" of KJumpingCube.
*/
class AI_Main : public QObject
{
   Q_OBJECT
public:
   // QMutex endMutex;		// Use when stopping the threaded calculation?
   int computeMove();		// The threaded part of the move calculation.

   // Statistics-gathering procedures.
   // -------------------------------
   void startStats();
   void postMove (Player player, int index);
   void dumpStats();

   /**
   * The constructor for the KJumpingCube AI (or "brain").
   */
   explicit AI_Main();
   virtual ~AI_Main();

   // IDW TODO - It would be good to use const for more parameters if possible.

   /**
   * Compute a good move for a player from a given position.
   *
   * @param player  Player for whom a move is to be computed.
   * @param box     The player's position before the move.
   */
   void getMove (const Player player, AI_Box * box);

   /**
    *  Stop the AI, but not till the end of the current cycle.  The AI will
    *  then return the best move found so far, via signal done(int index).
    */
   void stop();

   /**
    * Return true if the AI is running at the moment.
    */
   bool isActive() const;

   /**
    * Set skill and AI players according to current preferences or settings.
    */
   void setSkill (int skill1, bool kepler1, bool newton1,
                  int skill2, bool kepler2, bool newton2);

signals:
   /**
    * Signal the best move found after the AI search finishes or is stopped.
    *
    * @param index    The index, within the cube box, of the cube to move.
    */
   void done (int index);

private:
   ThreadedAI * m_thread;	// A thread to run computeMove() and tryMoves().

   Player     m_player;		// The player whose best move is to be found.
   AI_Box *   m_box;		// The successive positions being evaluated.

   int        m_side;		// The number of rows/columns in the cube box.
   int        m_nCubes;		// The number of cubes in the cube box.
   int *      m_owners;		// The m_nCubes owners of the cubes.
   int *      m_values;		// The m_nCubes values of the cubes.
   int *      m_maxValues;	// The m_nCubes maximum values of the cubes.

   /**
   * Recursively check and evaluate moves available at a given position, using
   * the MiniMax algorithm.  The AI_Box instance (m_box) is used to store
   * positions and make moves that follow the KJumpingCube rules.  Instances of
   * AI_Base (such as m_AI_Kepler or m_AI_Newton) are used to assess possible
   * moves and the resulting positions.
   *
   * @param player  Player for whom moves are being evaluated.
   * @param level   Current level of recursion (i.e. level in move tree).
   *
   * @return        The best move found and the value of the position reached.
   */
   Move tryMoves (Player player, int level);

   /**
   * Checks a position to find moves that are likely to give good results.
   *
   * @param c2m     Array in which likely moves will be stored.
   * @param player  The player who is to move.
   * @param side    The number of rows/columns in the cube box.
   * @param owners  The owner of each cube in the box.
   * @param values  The value of each cube in the box.
   * @param maxValues The maximum value of each cube in the box.
   *
   * @return        The likely moves found.
   */
   int findCubesToMove (Move * c2m, Player player, int side,
                        Player * owners, int * values, int * maxValues);

   AI_Base * m_AI_Kepler;	// Pointer to a Kepler-style player.
   AI_Base * m_AI_Newton;	// Pointer to a Newton-style player.

   AI_Base * m_ai [3];		// Pointers to AI players 1 and 2 (but not 0).
   int m_ai_skill [3];		// Skills of AI players 1 and 2 (but not 0).
   int m_ai_maxLevel [3];	// Lookahead of AI players 1 and 2 (but not 0).

   AI_Base * m_currentAI;	// Pointer to AI for current player.
   int  m_skill;		// Skill of the current player's AI.
   int  m_maxLevel;		// Maximum search depth for current player.

   int  m_currentLevel;		// Current search depth (or lookahead).

   bool m_stopped;		// True if the AI has to be stopped.
   bool m_active;		// True if the AI is running.

   KRandomSequence m_random;	// Random number generator.

   int n_simulate;		// Number of moves simulated in the search.
   int n_assess;		// Number of positions assessed in the search.

   // Statistical data and methods.
   // -----------------------------
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

   void boxPrint (int side, int * owners, int * values);

   QString tag (int level);
   void initStats (int player);
   void saveStats (Move & move);
};

#endif //AI_MAIN_H
