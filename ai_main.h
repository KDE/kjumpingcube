/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef AI_MAIN_H
#define AI_MAIN_H

#include <QList>
#include <QThread>
#include <QMutex>
#include <QRandomGenerator>

#include "ai_base.h"
#include "ai_box.h"

class ThreadedAI;

struct Move
{
   int index;
   int val;
};

/**
 * Class AI_Main computes a (good) possible move from a given position.
 *
 * It puts a value on every cube by looking at its neighbours and picks a
 * short list of likely cubes to move (it takes too long to check every
 * possible move).  It then simulates what would happen if you click on
 * these cubes.  This is all done recursively to a certain depth and the
 * position is valued.  The move that leads to the best position is chosen.
 *
 * @short The "computer player" of KJumpingCube.
 */

/**
 * Number of available skill-levels for computer players.  Must be consistent
 * with the Qt Designer form and the file "settings.ui" that it generates.
 */
const int nSkillLevels = 5;

/**
 * Extent of lookahead for each computer skill-level.  0 = perform moves and
 * evaluate the resulting positions, selecting the move with the best value
 * of those being considered.  n > 0 = perform n moves recursively, using
 * the MiniMax algorithm, then perform one more move and evaluate it, e.g.
 * depth 1 causes the computer to evaluate your responses to its moves.
 */
const int depths [nSkillLevels] = {0, 0, 1, 3, 5};

/**
 * The maximum number of moves considered at each level of lookahead.  This
 * limits the size of the move-tree and the amount of computation involved.
 * The most likely moves are chosen using values supplied by assessCube()
 * (see the AI_Kepler and AI_Newton classes for examples).
 */
const int maxBreadth = 4;

class AI_Main : public AI_Box
{
   // NOTE: Inheriting AI_Box gives AI_Main direct access to AI_Box's protected
   //       data arrays.  Speed of computation is essential here.
   Q_OBJECT
public:
   /**
   * The constructor for the KJumpingCube AI (or "computer player").
   */
   explicit AI_Main (QObject * parent, int side);
   virtual ~AI_Main();

   /**
   * Compute a good move for a player from a given position, providing either
   * a computer-player move or a hint for a human player.  The main calculation
   * is threaded and returns its result via signal "done (int index)".
   *
   * @param player  Player for whom a move is to be computed.
   * @param box     The player's position before the move.
   */
   void getMove (const Player player, const AI_Box * box);

   int computeMove();		// The threaded part of the move calculation.

   // QMutex endMutex;		// Use when stopping the threaded calculation?

   /**
    *  Stop the AI, but not till the end of the current cycle.  The AI will
    *  then return the best move found so far, via signal "done (int index)".
    */
   void stop();

   /**
    * Set skill and AI players according to current preferences or settings.
    */
   void setSkill (int skill1, bool kepler1, bool newton1,
                  int skill2, bool kepler2, bool newton2);

Q_SIGNALS:
   /**
    * Signal the best move found after the AI search finishes or is stopped.
    *
    * @param index    The index, within the cube box, of the cube to move.
    */
   void done (int index);

private:
   ThreadedAI * m_thread;	// A thread to run computeMove() and tryMoves().

   Player     m_player;		// The player whose best move is to be found.

   int *      m_randomSeq;	// A random order for trying out moves.

   /**
   * Recursively check and evaluate moves available at a given position, using
   * the MiniMax algorithm.  The AI_Box instance (m_box) is used to store
   * positions and make moves that follow the KJumpingCube rules.  Instances of
   * AI_Base (such as m_AI_Kepler or m_AI_Newton) are used to assess possible
   * moves and the resulting positions.
   *
   * @param player  Player for whom moves are being evaluated.
   * @param level   Current level of recursion (i.e. level in move-tree).
   *
   * @return        The best move found and the value of the position reached.
   */
   Move tryMoves (Player player, int level);

   /**
   * Check a position to find moves that are likely to give good results.
   * Return at most maxBreadth moves, to reduce computation time in tryMoves().
   *
   * @param c2m       The array in which likely moves will be stored.
   * @param player    The player who is to move.
   * @param owners    The owner of each cube in the box.
   * @param values    The value of each cube in the box.
   * @param maxValues The maximum value of each cube in the box.
   *
   * @return          The number of likely moves found.
   */
   int findCubesToMove (Move * c2m, const Player player, const Player * owners,
                                    const int * values, const int * maxValues);

   /**
    * Make sure a cube box of the correct size is available as a workspace.
    *
    * @param side    The number of rows/columns in the cube box.
    */
   void checkWorkspace (const int side);

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

   QRandomGenerator m_random;	// Random number generator.

#if AILog > 0
public:
   // Statistics-gathering procedures.
   // -------------------------------
   void startStats();
   void postMove (Player player, int index, int side);
   void dumpStats();
#endif

private:
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
      int nLikelyMoves;
      Move * likelyMoves;
      QList<SearchStats *> * searchStats;
   };

   int m_currentMoveNo;
#if AILog > 0
   MoveStats * m_currentMove;
   QList<MoveStats *> m_moveStats;

   int n_simulate;		// Number of moves simulated in the search.
   int n_assess;		// Number of positions assessed in the search.

   void boxPrint (int side, int * owners, int * values);

   QString tag (int level);
   void initStats (int player);
   void saveStats (Move & move);
   void saveLikelyMoves (int nMoves, Move * moves);
#endif
};

#endif //AI_MAIN_H
