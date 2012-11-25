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

#include "ai_main.h"
#include "ai_kepler.h"
#include "ai_newton.h"
#include "ai_box.h"

#include <QApplication>

// #define DEBUG // Uncomment this to get AI_Main's debug messages
#include <assert.h>

#include <QDebug>
#include <QTime>

#include "prefs.h"

   // int nBits = sizeof(long int) * 8;
   // double maxValue = (nBits >= 32) ? - 0x7fffffff : - 0x7fff; 
   // qDebug() << "nBits" << nBits << "maxValue" << maxValue;

const int BigValue = 0x3fffffff;

AI_Main::AI_Main()
   /* IDW TODO - Use a thread and return the move via a signal.
   :
   m_thread (new ThreadedAI (this))
   */
   :
   m_box (0),
   m_side (1)
{
   m_moveStats.clear();
   m_currentMoveNo = 0;

   m_AI_Kepler = new AI_Kepler();
   m_AI_Newton = new AI_Newton();

   setSkill (Prefs::EnumSkill1::Beginner, true, false,
             Prefs::EnumSkill2::Beginner, true, false);

   m_stopped = false;
   m_active = false;
   m_currentLevel = 0;

   m_random.setSeed (0);

/*
   // Speed test for Move algorithms.
   QTime t;
   int size = 3;
   int pos  = 4;
   int sizes [8] = {3, 4,  5,  6,  7,  8,  9, 10};
   // int posns [8] = {4, 8, 12, 18, 24, 32, 40, 50};
   int posns [8] = {0, 0, 0, 0, 0, 0, 0, 0};
   int kmax = 10000;
   for (int test = 0; test < 8; test++) {
       size = sizes [test];
       pos  = posns [test];
       AI_Box xxxxx (size);	//  IDW test.
       t.start();
       for (int k = 0; k < kmax; k++) {
           xxxxx.clear();
           for (int n = 0; n < 1000; n++) {
               // printf ("MOVE %2d, Player %d at %d\n", n, One, pos);
               if (xxxxx.doMove (One, pos)) break;
               // xxxxx.printBox();
           }
       }
       // xxxxx.printBox();
       qDebug() << "TIME" << t.restart();
       for (int k = 0; k < kmax; k++) {
           xxxxx.clear();
           for (int n = 0; n < 1000; n++) {
               // printf ("MOVE %2d, Player %d at %d\n", n, One, pos);
               if (xxxxx.oldMove (One, pos)) break;
               // xxxxx.printBox();
           }
       }
       // xxxxx.printBox();
       qDebug() << "TIME" << t.restart();
   }
*/
}

AI_Main::~AI_Main()
{
   delete m_AI_Kepler;
   delete m_AI_Newton;
}

void AI_Main::setSkill (int skill1, bool kepler1, bool newton1,
                        int skill2, bool kepler2, bool newton2)
{
   m_ai[0] = 0;
   m_ai[1] = kepler1 ? m_AI_Kepler : m_AI_Newton;
   m_ai[2] = kepler2 ? m_AI_Kepler : m_AI_Newton;

   m_ai_skill[0] = 0;
   m_ai_skill[1] = skill1;
   m_ai_skill[2] = skill2;

   m_ai_maxLevel[0] = 0;
   for (int player = 1; player <= 2; player++) {
      switch (m_ai_skill[player]) {
      case Prefs::EnumSkill1::Beginner:
         m_ai_maxLevel[player] = 1;
         break;
      case Prefs::EnumSkill1::Average:
         m_ai_maxLevel[player] = 3;
         break;
      case Prefs::EnumSkill1::Expert:
         m_ai_maxLevel[player] = 5;
         break;
      default:
         m_ai_maxLevel[player] = 3;
         break;
      }
   }
}

/*
int AI_Main::skill() const
{
   return m_skill;
}
*/

void AI_Main::stop()
{
   m_stopped = true;
}

bool AI_Main::isActive() const
{
   return m_active;
}

bool AI_Main::getMove (int & index, const Player player, AI_Box * box)
{
   qDebug() << "\nEntering AI_Main::getMove() for player" << player;
   if (isActive())
      return false;

   m_currentAI = m_ai[player];
   m_skill     = m_ai_skill[player];
   m_maxLevel  = m_ai_maxLevel[player];

   initStats (player);	// IDW test. Statistics collection.

   m_active  = true;
   m_stopped = false;
   m_player  = player;

   // IDW TODO - Change the CubeBox model.
   if (m_box == 0) {
       qDebug() << "NEW AI_Box REQUIRED: side =" << box->side();
       m_box = new AI_Box (box->side());
   }
   else if (m_side != box->side()) {
       qDebug() << "NEW AI_Box SIZE NEEDED: was" << m_side << "now" << box->side();
       delete m_box;
       m_box = new AI_Box (box->side());
   }
   m_side = m_box->side();
   m_box->printBox();
   m_box->initPosition (box, player, true);
   qDebug() << "INITIAL POSITION";
   m_box->printBox();

   /* IDW TODO - If a thread is to be used, it will have to return the
    *            calculated move (m_move) via a signal (simulated click?).
    *
   m_thread->start(QThread::IdlePriority);
   m_thread->wait();
   */

   // Start the recursive MiniMax algorithm on a copy of the current cube box.
   // IDW test. Move move = tryMoves (m_player, m_side, m_owners, m_values, m_maxValues, 0);
   qDebug() << tag(0) << "Calling tryMoves(), level zero, player" << m_player;
   Move move = tryMoves (m_player, 0);

   qDebug() << tag(0) << "Returned from tryMoves(), level zero, player" << m_player
            << "value" << move.val
            << "simulate" << n_simulate << "assess" << n_assess;
   qDebug() << "==============================================================";

   // delete [] m_owners;
   // delete [] m_values;
   // delete [] m_maxValues;	// IDW test. Needed in dumpStats().

   saveStats (move);		// IDW test. Statistics collection.

   // Pass the best move found back to the caller.
   index = move.row * m_side + move.col;

   qDebug() << tag(0) << "MOVE" << m_currentMoveNo << "for PLAYER" << m_player
            << "X" << move.row << "Y" << move.col;
   m_active = false;

   return (! m_stopped);
}

/* IDW TODO - Use a thread and return the move via a signal.
void AI_Main::computeMove()
{
   m_move = tryMoves (m_player, m_side, m_owners, m_values, m_maxValues, 0);
   qDebug() << "Player" << m_player << "best move" << m_move.row << m_move.col;
}
*/

// IDW test. Move AI_Main::tryMoves (Player player, int side, int * owners,
                        // IDW test. int * values, int * maxValues, int level)
Move AI_Main::tryMoves (Player player, int level)
{
   m_currentLevel = level; // IDW test. To limit qDebug() in findCubesToMove().
   double maxValue = -BigValue;
   bool isAI = true;

   Move bestMove = {-1, -1, -BigValue};

   // Find likely cubes to move.
   int nCubes = m_side * m_side;
   Move * cubesToMove = new Move [nCubes];
   boxPrint (m_side, (int *) m_box->m_owners, m_box->m_values); // IDW test.
   int moves = findCubesToMove (cubesToMove, player, m_side,
                       m_box->m_owners, m_box->m_values, m_box->m_maxValues);

   qDebug() << tag(level) << "Level" << level << "Player" << player
            << "number of likely moves" << moves;
   for (int n = 0; n < moves; n++) {
      qDebug() << tag(level) << "    " << "X" << cubesToMove[n].row
                         << "Y" << cubesToMove[n].col
                         << "val" << cubesToMove[n].val;
   }

   // IDW TODO - Sort the moves by priority in findCubesToMove() (1 first),
   //    shuffle moves that have the same value (to avoid repetitious openings).
   // IDW TODO - Apply alpha-beta pruning to the sorted moves.  Maybe we can
   //    allow low-priority moves and sacrifices ...

   /* IDW test.
   int * ownersCopy = new int [nCubes];
   int * valuesCopy = new int [nCubes];
   */

   m_currentMove->searchStats->at(level)->n_moves += moves;

   for (int n = 0; n < moves; n++) {
      qDebug() << "\n";
      if (level == 0) {
         qDebug() << "==============================================================";
      }
      qDebug() << tag(level) << "TRY" << n << "at level" << level
               << "Player" << player
               << "X"<< cubesToMove[n].row << "Y" << cubesToMove[n].col
               << "val" << cubesToMove[n].val;

      /* IDW test.
      // Copy the contents of the the cube box.
      for (int index = 0; index < nCubes; index++) {
         ownersCopy[index] = owners[index];
         valuesCopy[index] = values[index];
      }
      bool won = simulateMove (player, cubesToMove[n].row, cubesToMove[n].col,
                               side, ownersCopy, valuesCopy, maxValues);
      */
      m_box->copyPosition (player, true);
      bool won = m_box->doMove (player,
                                cubesToMove[n].row*m_side + cubesToMove[n].col);
      n_simulate++;

      double val;
      if (won) {
         // Accept a winning move.
         bestMove = cubesToMove[n];
         bestMove.val = BigValue - 1;
         n_assess++;
         cubesToMove[n].val = bestMove.val; // IDW test. For debug output.
         qDebug() << tag(level) << "Player" << player << "wins at level" << level
                  << "move" << cubesToMove[n].row << cubesToMove[n].col;
         m_box->undoPosition (player, isAI);
         break;
      }
      else if (level >= m_maxLevel) { // ((level >= m_maxLevel) || (moves == 1))
	 // IDW TODO - On second thoughts, it might be best to find out the best
	 //            or worst that can happen if moves == 1.
	 // Stop the recursion.
	 // IDW TODO - Should assessField param 3 be type (Player *)?
         val = m_currentAI->assessField (player, m_side,
                                         m_box->m_owners, m_box->m_values);
	 n_assess++;
         cubesToMove[n].val = val; // IDW test. For debug output.
         qDebug() << tag(level) << "END RECURSION: Player" << player
                  << "X" << cubesToMove[n].row
                  << "Y" << cubesToMove[n].col << "assessment" << val;
         boxPrint (m_side, (int *)(m_box->m_owners), m_box->m_values);// IDW test.
      }
      else {
         // Switch players.
         Player opponent = (player == One) ?  Two : One;

         // Do the MiniMax calculation for the next recursion level.
         qDebug() << tag(level) << "CALL tryMoves: Player" << opponent << "level" << level+1;
	 // IDW TODO - Should tryMoves param be reorganised?
	 // IDW TODO - Do we need to be carrying maxValues around everywhere?
         // IDW test. Move move = tryMoves (opponent, side, (int *)(m_box->m_owners), m_box->m_values,
                               // IDW test. maxValues, level + 1);
         Move move = tryMoves (opponent, level + 1);
         val = move.val;
         cubesToMove[n].val = val; // IDW test. For debug output.
         qDebug() << tag(level) << "RETURN to level" << level << "Player" << player
                  << "X" << move.row << "Y" << move.col << "assessment" << val;
      }

      if (val > maxValue) {
	 maxValue = val;
	 bestMove = cubesToMove[n];
	 bestMove.val = val;
         cubesToMove[n].val = val; // IDW test. For debug output.
	 qDebug() << tag(level) << "NEW MAXIMUM at level" << level << "Player" << player
	          << "X" << bestMove.row << "Y" << bestMove.col
                  << "assessment" << val;
      }
      m_box->undoPosition (player, isAI);
   }

   if (level == 0) {
      qDebug() << tag(level) << "VALUES OF MOVES - Player" << player
            << "number of moves" << moves;
      for (int n = 0; n < moves; n++) {
         qDebug() << tag(level) << "    " << "X" << cubesToMove[n].row
                            << "Y" << cubesToMove[n].col
                            << "val" << cubesToMove[n].val;
      }
   }

   delete [] cubesToMove;
   // IDW TODO - Need to do m_box->undoPosition (player, isAI) somewhere here.

   /* IDW test.
   delete [] ownersCopy;
   delete [] valuesCopy;
   */

   qDebug();
   qDebug() << tag(level) << "BEST MOVE at level" << level << "Player" << player
            << "X" << bestMove.row << "Y" << bestMove.col
            << "assessment" << bestMove.val;

   // Apply the MiniMax rule.
   if (level > 0) {
      qDebug() << tag(level) << "CHANGE SIGN" << bestMove.val << "to" << -bestMove.val;
      bestMove.val = - bestMove.val;
   }

   return bestMove;
}

int AI_Main::findCubesToMove (Move * c2m, Player player, int side,
                              Player * owners, int * values, int * maxValues)
{
   // IDW TODO - Streamline and tidy up this method.
   int x, y, index, n;
   int opponent = (player == One) ? Two : One;
   int moves    = 0;
   int min      = 9999;
   int nCubes   = side * side;
   int * seq    = new int [nCubes];

   // Set up a random sequence of integers 0 to (nCubes - 1).
   for (n = 0; n < nCubes; n++) {
      seq[n] = n;
   }

   int last = nCubes;
   int z, temp;
   for (n = 0; n < nCubes; n++) {
      z = m_random.getLong (last);
      last--;
      temp = seq[z];
      seq[z] = seq[last];
      seq[last] = temp;
   }

   if (m_skill == Prefs::EnumSkill1::Beginner) {
      // Select the cubes with the most pips on them.
      int max = 0;
        for (n = 0; n < nCubes; n++) {
	   index = seq[n];
           if (owners[index] != opponent) {
              c2m[moves].row = index / side;
              c2m[moves].col = index % side;
              c2m[moves].val = values[index];

              if (c2m[moves].val > max)
                 max = c2m[moves].val;

              // qDebug() << "cube" << x << y << "val" << c2m[moves].val
              //          << "max" << max;
              moves++;
           }
        }

      // Find all moves with maximum value.
      int counter=0;
      for (n = 0; n < moves; n++) {
         if (c2m[n].val == max) {
            c2m[counter].row = c2m[n].row;
            c2m[counter].col = c2m[n].col;
            c2m[counter].val = c2m[n].val;

            counter++;
         }
      }

      if (counter != 0) {
         moves = counter;
      }
   }
   else {	// If skill is not Beginner.
      // Put values on the cubes.
      int secondMin = min;
      for (n = 0; n < nCubes; n++) {
	   index = seq[n];
           // Use only cubes that do not belong to the opponent.
           // qDebug() << "\nCUBE" << x << y << "OWNER" << owners[index]
           //          << "OPPONENT" << opponent;
           if (owners[index] != opponent) {
              int val;

              // Check the neighbours of each cube.
              x = index / side;
              y = index % side;
              val = m_currentAI->assessCube (x, y, player,
                                             side, owners, values, maxValues);

#ifdef DEBUG
              if (m_currentLevel == 0)
                 qDebug() << x << "," << y << " : " << val;
#endif
              // Only if val > 0 is it a likely move.
              if ( val > 0 ) {
                 if (val < min) {
                    secondMin = min;
                    min = val;
                 }
                 else if ((val > min) && (val < secondMin)) {
                    secondMin = val;
		 }

                 // Store the move.
                 c2m[moves].row = x;
                 c2m[moves].col = y;
                 c2m[moves].val = val;
                 moves++;
              }
           }
        }
        if (m_currentLevel == 0)
        qDebug() << "\nMinimum is" << min << ", second minimum is" << secondMin
	         << "available moves" << moves;

        // If all cubes are bad, check all cubes for the next move.
        if (moves == 0) {
           min = 5;
           for (x = 0; x < side; x++)
              for (y = 0; y < side; y++) {
                 index = x * side + y;
                 if (owners[index] != opponent) {
                    c2m[moves].row = x;
                    c2m[moves].col = y;
                    c2m[moves].val = (maxValues[index] - values[index]) + 1;
                    if (c2m[moves].val < min)
                       min = c2m[moves].val;
                    moves++;
                 }
              }
	   qDebug() << "NO LIKELY MOVES AVAILABLE: selecting" << moves << "min" << min;
        }

        int counter = 0;
        // Find all moves with minimum assessment
        for (n = 0; n < moves; n++) {
           if (c2m[n].val == min) {
              c2m[counter].row = c2m[n].row;
              c2m[counter].col = c2m[n].col;
              c2m[counter].val = c2m[n].val;

              counter++;
           }
        }

        // IDW TODO - Finalise the logic for limiting the number of moves tried.
        //            The 1/3 gizmo is to limit searches on an empty cube box.
        //            Should not use secondMin on Expert level?
        //            Should not use secondMin on deeper recursion levels?
        //            Should it all depend on how many moves are at "min"?
        //            Should AI_Newton have overlapping values of move types?

        // if (true || m_skill == Prefs::EnumSkill1::Average) // IDW test.
        // if ((counter <= 2) || (m_skill == Prefs::EnumSkill1::Average)) {
        // if (m_currentMoveNo > (nCubes / 3)) {

        if (/* (m_skill == Prefs::EnumSkill1::Average) && */
            (m_currentMoveNo > (nCubes / 3))) { // IDW test. Board > 1/3 full
           for (n = 0; n < moves; n++) {
              if (c2m[n].val == secondMin) {
                 c2m[counter].row = c2m[n].row;
                 c2m[counter].col = c2m[n].col;
                 c2m[counter].val = c2m[n].val;

                 counter++;
              }
           }
        }

        if (counter != 0) {
           moves = counter;
        }
   }

   delete [] seq;

   // IDW TODO - Can we find a more subtle rule?

   // If more than maxMoves moves are favorable, take maxMoves random
   // moves because it will take too long to check more.
   // int maxMoves = (m_currentMoveNo > (nCubes / 3)) ? 10 : 4;
   int maxMoves = 4;
   return qMin (moves, maxMoves);
}

void AI_Main::boxPrint (int side, int * owners, int * values)
{
   // IDW test. For debugging.
   fprintf (stderr, "AI_Main::boxPrint (%d, %lu, %lu)\n",
            side, (long) owners, (long) values);
   for (int y = 0; y < side; y++) {
      fprintf (stderr, "   ");
      for (int x = 0; x < side; x++) {
	 int index = x * side + y;
	 if (owners[index] == Nobody) fprintf (stderr, "  .");
	 else fprintf (stderr, " %2d", (owners[index] == One) ?
		 values[index] : -values[index]);
      }
      fprintf (stderr, "\n");
   }
}

void AI_Main::startStats()
{
   // IDW test. For debugging.
   m_currentMoveNo = 0;
   m_moveStats.clear();
}

void AI_Main::postMove (Player player, int index)
{
   // IDW test. Statistics collection.
   // Used to record a move by a human player or other AI (e.g. Brain class).
   int x = index / m_side;
   int y = index % m_side;
   m_maxLevel    = m_ai_maxLevel[player];
   m_currentMove = new MoveStats [1];

   m_currentMoveNo++;
   m_currentMove->player     = player;
   m_currentMove->moveNo     = m_currentMoveNo;
   m_currentMove->n_simulate = 0;
   m_currentMove->n_assess   = 0;
   m_currentMove->searchStats = new QList<SearchStats *>();

   m_currentMove->x     = x;
   m_currentMove->y     = y;
   m_currentMove->value = 0;
   m_moveStats.append (m_currentMove);
   qDebug() << "==============================================================";
   qDebug() << tag(0) << "MOVE" << m_currentMoveNo << "for PLAYER" << player
            << "X" << x << "Y" << y;
   qDebug() << "==============================================================";
}

void AI_Main::initStats (int player)
{
   // IDW test. For debugging.
   m_currentMove = new MoveStats [1];

   m_currentMoveNo++;
   m_currentMove->player     = (Player) player;
   m_currentMove->moveNo     = m_currentMoveNo;
   m_currentMove->n_simulate = 0;
   m_currentMove->n_assess   = 0;
   m_currentMove->searchStats = new QList<SearchStats *>();

   qDebug() << tag(0) << "PLAYER" << player << m_currentAI->whoami() << "skill" << m_skill << "max level" << m_maxLevel;

   for (int n = 0; n <= m_maxLevel; n++) {
      SearchStats * s = new SearchStats [1];
      s->n_moves = 0;
      m_currentMove->searchStats->append (s);
   }

   n_simulate = 0;
   n_assess = 0;
}

void AI_Main::saveStats (Move & move)
{
   // IDW test. For debugging.
   m_currentMove->x = move.row;
   m_currentMove->y = move.col;
   m_currentMove->value = move.val;
   m_currentMove->n_simulate = n_simulate;
   m_currentMove->n_assess   = n_assess;
   m_moveStats.append (m_currentMove);
}

void AI_Main::dumpStats()
{
   // IDW test. For debugging. Replay all the moves, with statistics for each.
   AI_Box * statsBox = new AI_Box (m_side);
   statsBox->printBox();
   foreach (MoveStats * m, m_moveStats) {
      QList<int> l;
      int nMax = m->searchStats->count();
      for (int n = 0; n < nMax; n++) {
         l.append (m->searchStats->at(n)->n_moves);
      }
      qDebug() << m->player << m->moveNo << "X" << m->x << "Y" << m->y
               << "value" << m->value << m->n_simulate << m->n_assess << l;

      bool won = statsBox->doMove (m->player, m->x * m_side + m->y);
      statsBox->printBox();
      qDeleteAll (*(m->searchStats));
      delete m;
   }
   m_moveStats.clear();
   delete statsBox;
}

QString AI_Main::tag (int level)
{
    QString indent ("");
    indent.fill ('-', 2 * level);
    indent = indent.prepend (QString::number (level));
    indent = indent.leftJustified (2 * m_maxLevel + 1);
    QString mv = QString::number(m_currentMoveNo).rightJustified(3, '0');
    return (QString ("%1 %2 %3")).arg(m_currentMove->player).arg(mv).arg(indent);
}
