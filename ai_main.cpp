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
#include "cube.h"
#include "ai_kepler.h"
#include "ai_newton.h"

#include <QApplication>

// #define DEBUG // Uncomment this to get AI_Main's debug messages
#include <assert.h>

#ifdef DEBUG
#include <QDebug>
#endif

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
{
   m_moveStats.clear();
   m_currentMoveNo = 0;

   // IDW TODO - Make the AI selectable for each player (settings.ui and .kcfg).
   m_currentAI = new AI_Newton();
   m_ai[0] = 0;
   // m_ai[2] = m_currentAI; // IDW test, switch AIs.
   // m_ai[1] = new AI_Kepler(); // IDW test, switch AIs.
   m_ai[1] = m_currentAI;
   // m_ai[2] = new AI_Kepler();
   m_ai[2] = m_currentAI; // IDW test, Newton v. Newton.

   setSkill (Prefs::EnumSkill::Beginner);
   m_stopped = false;
   m_active = false;
   m_currentLevel = 0;

   m_random.setSeed (0);
}

void AI_Main::setSkill (int newSkill)
{
   m_skill = newSkill;

   switch (m_skill) {
   case Prefs::EnumSkill::Beginner:
      m_maxLevel = 1;
      break;
   case Prefs::EnumSkill::Average:
      // m_maxLevel = 2; // IDW test.
      m_maxLevel = 3;
      // IDW test. m_maxLevel = 1; // IDW test.
      break;
   case Prefs::EnumSkill::Expert:
      m_maxLevel = 5;
      // IDW test. m_maxLevel = 13;
      // m_maxLevel = 13;
      break;
   default:
      break;
   }
}

int AI_Main::skill() const
{
   return m_skill;
}

void AI_Main::stop()
{
   m_stopped = true;
}

bool AI_Main::isActive() const
{
   return m_active;
}

bool AI_Main::getMove (int & row, int & column,
                       CubeBox::Player player, CubeBox & box)
{
   // qDebug() << "Entering AI_Main::getMove() for player" << player;
   if (isActive())
      return false;


   m_random.setSeed (20120813);

   // IDW test. Statistics collection.
   m_currentMove = new MoveStats [1];

   m_currentMoveNo++;
   m_currentMove->player     = player;
   m_currentMove->moveNo     = m_currentMoveNo;
   m_currentMove->n_simulate = 0;
   m_currentMove->n_assess   = 0;
   m_currentMove->searchStats = new QList<SearchStats *>();

   for (int n = 0; n <= m_maxLevel; n++) {
      SearchStats * s = new SearchStats [1];
      s->n_moves = 0;
      m_currentMove->searchStats->append (s);
   }

   n_simulate = 0;
   n_assess = 0;
   m_active = true;
   m_stopped = false;

   // IDW test. Copy the current CubeBox model to a faster vector-based model.
   // IDW TODO - Change the CubeBox model.
   m_player    = player;
   m_side      = box.dim();
   m_nCubes    = m_side * m_side;
   m_owners    = new int [m_nCubes];
   m_values    = new int [m_nCubes];
   m_maxValues = new int [m_nCubes];

   for (int x = 0; x < m_side; x++)
      for (int y = 0; y < m_side; y++) {
	 int index         = x * m_side + y;
	 m_owners [index]    = box[x][y]->owner();
	 m_values [index]    = box[x][y]->value();
	 m_maxValues [index] = box[x][y]->max();
      }
   // boxPrint (m_side, m_owners, m_values); // IDW test.

   /* IDW TODO - If a thread is to be used, it will have to return the
    *            calculated move (m_move) via a signal (simulated click?).
    *
   m_thread->start(QThread::IdlePriority);
   m_thread->wait();
   */

   // Start the recursive MiniMax algorithm on a copy of the current cube box.
   Move move = tryMoves (m_player, m_side, m_owners, m_values, m_maxValues, 0);

   qDebug() << "Returned from tryMoves(), level zero, player" << m_player
            << "value" << move.val
            << "simulate" << n_simulate << "assess" << n_assess;
   qDebug() << "===============================================================";

   // Pass the best move found back to the caller.
   row    = move.row;
   column = move.col;

   qDebug() << "MOVE" << m_currentMoveNo << "for PLAYER" << m_player
            << "X" << row << "Y" << column;
   m_active = false;

   // delete [] m_owners; // IDW test. Do this in dumpStats().
   // delete [] m_values; // IDW test. Do this in dumpStats().
   // delete [] m_maxValues; // IDW test. Do this in dumpStats().

   // IDW test. Statistics collection.
   m_currentMove->x = row;
   m_currentMove->y = column;
   m_currentMove->value = move.val;
   m_currentMove->n_simulate = n_simulate;
   m_currentMove->n_assess   = n_assess;
   m_moveStats.append (m_currentMove);

   return (! m_stopped);
}

/* IDW TODO - Use a thread and return the move via a signal.
void AI_Main::computeMove()
{
   m_move = tryMoves (m_player, m_side, m_owners, m_values, m_maxValues, 0);
   qDebug() << "Player" << m_player << "best move" << m_move.row << m_move.col;
}
*/

Move AI_Main::tryMoves (CubeBox::Player player, int side, int * owners,
                        int * values, int * maxValues, int level)
{
   m_currentLevel = level; // IDW test. To limit qDebug() in findCubesToMove().
   double maxValue = -BigValue;

   Move bestMove = {-1, -1, -BigValue};

   m_currentAI = m_ai[player];

   // Find likely cubes to move.
   int nCubes = side * side;
   Move * cubesToMove = new Move [nCubes];
   boxPrint (side, owners, values); // IDW test.
   int moves = findCubesToMove (cubesToMove, player,
                                side, owners, values, maxValues);

   qDebug() << "Level" << level << "Player" << player
            << "number of likely moves" << moves;
   for (int n = 0; n < moves; n++) {
      qDebug() << "    " << "X" << cubesToMove[n].row
                         << "Y" << cubesToMove[n].col
                         << "val" << cubesToMove[n].val;
   }

   // IDW TODO - Sort the moves by priority in findCubesToMove() (1 first),
   //    shuffle moves that have the same value (to avoid repetitious openings).
   // IDW TODO - Apply alpha-beta pruning to the sorted moves.  Maybe we can
   //    allow low-priority moves and sacrifices ...

   int * ownersCopy = new int [nCubes];
   int * valuesCopy = new int [nCubes];

   m_currentMove->searchStats->at(level)->n_moves += moves;

   for (int n = 0; n < moves; n++) {
      qDebug() << "\n";
      if (level == 0) {
         qDebug() << "===============================================================";
      }
      qDebug() << "TRY at level" << level << "Player" << player
               << "X"<< cubesToMove[n].row << "Y" << cubesToMove[n].col
               << "val" << cubesToMove[n].val;

      // Copy the contents of the the cube box.
      for (int index = 0; index < nCubes; index++) {
         ownersCopy[index] = owners[index];
         valuesCopy[index] = values[index];
      }
      bool won = simulateMove (player, cubesToMove[n].row, cubesToMove[n].col,
                               side, ownersCopy, valuesCopy, maxValues);
      n_simulate++;

      double val;
      if (won) {
         // Accept a winning move.
         bestMove = cubesToMove[n];
         bestMove.val = BigValue - 1;
         n_assess++;
         cubesToMove[n].val = bestMove.val; // IDW test. For debug output.
         qDebug() << "Player" << player << "wins at level" << level
                  << "move" << cubesToMove[n].row << cubesToMove[n].col;
         break;
      }
      else if (level >= m_maxLevel) { // ((level >= m_maxLevel) || (moves == 1))
	 // IDW TODO - On second thoughts, it might be best to find out the best
	 //            or worst that can happen if moves == 1.
	 // Stop the recursion.
         val = m_currentAI->assessField (player, side, ownersCopy, valuesCopy);
	 n_assess++;
         cubesToMove[n].val = val; // IDW test. For debug output.
         qDebug() << "END RECURSION: Player" << player
                  << "X" << cubesToMove[n].row
                  << "Y" << cubesToMove[n].col << "assessment" << val;
         boxPrint (side, ownersCopy, valuesCopy); // IDW test.
      }
      else {
         // Switch players.
         CubeBox::Player opponent = (player == CubeBox::One) ?
                                     CubeBox::Two : CubeBox::One;

         // Do the MiniMax calculation for the next recursion level.
         qDebug() << "CALL tryMoves: Player" << opponent << "level" << level+1;
         Move move = tryMoves (opponent, side, ownersCopy, valuesCopy,
                               maxValues, level + 1);
         val = move.val;
         cubesToMove[n].val = val; // IDW test. For debug output.
         qDebug() << "RETURN to level" << level << "Player" << player
                  << "X" << move.row << "Y" << move.col << "assessment" << val;
      }

      if (val > maxValue) {
	 maxValue = val;
	 bestMove = cubesToMove[n];
	 bestMove.val = val;
         cubesToMove[n].val = val; // IDW test. For debug output.
	 qDebug() << "NEW MAXIMUM at level" << level << "Player" << player
	          << "X" << bestMove.row << "Y" << bestMove.col
                  << "assessment" << val;
      }
   }

   if (level == 0) {
      qDebug() << "VALUES OF MOVES - Player" << player
            << "number of moves" << moves;
      for (int n = 0; n < moves; n++) {
         qDebug() << "    " << "X" << cubesToMove[n].row
                            << "Y" << cubesToMove[n].col
                            << "val" << cubesToMove[n].val;
      }
   }

   delete [] cubesToMove;
   delete [] ownersCopy;
   delete [] valuesCopy;

   qDebug() << "\nBEST MOVE at level" << level << "Player" << player
            << "X" << bestMove.row << "Y" << bestMove.col
            << "assessment" << bestMove.val;

   // Apply the MiniMax rule.
   if (level > 0) {
      qDebug() << "CHANGE SIGN" << bestMove.val << "to" << -bestMove.val;
      bestMove.val = - bestMove.val;
   }

   return bestMove;
}

int AI_Main::findCubesToMove (Move * c2m, CubeBox::Player player, int side,
                              int * owners, int * values, int * maxValues)
{
   // IDW TODO - Streamline and tidy up this method.
   int x, y, index, n;
   int opponent = (player == CubeBox::One) ? CubeBox::Two : CubeBox::One;
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

   if (m_skill == Prefs::EnumSkill::Beginner) {
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

        // if (true || m_skill == Prefs::EnumSkill::Average) // IDW test.
        // if ((counter <= 2) || (m_skill == Prefs::EnumSkill::Average)) {
        // if (m_currentMoveNo > (nCubes / 3)) {

        if ((m_skill == Prefs::EnumSkill::Average) &&
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
   int maxMoves = 10;
   return qMin (moves, maxMoves);
}

bool AI_Main::simulateMove (CubeBox::Player player, int row, int col, int side,
                            int * owners, int * values, int * maxValues)
{
   bool finished;
   bool playerWon = false;

   int index = row * side + col;
   int x, y;

   if (owners[index] != (Cube::Owner)player && owners[index] != Cube::Nobody)
      return false;			// The move is not valid.

   values[index]++;			// Increase the cube to be moved.
   owners[index] = player;		// If neutral, take ownership.
   // cubes[row][column]->increase((Cube::Owner)player); // IDW DELETE this.

   do {
      finished  = true;
      playerWon = true;

      // Check all cubes for overflow. Keep on checking till all moves are done.
      for (x = 0; x < side; x++) {
         for (y = 0; y < side; y++) {
	    index = x * side + y;
            if (values[index] > maxValues[index]) {
	       // Increase the neighboring cubes and take them over.
	       if (x > 0) {
		   values[index-side]++;	// West.
		   owners[index-side] = player;
	       }
	       if (x < side-1) {
		   values[index+side]++;	// East.
		   owners[index+side] = player;
	       }
	       if (y > 0) {
		   values[index-1]++;		// North.
		   owners[index-1] = player;
	       }
	       if (y < side-1) {
		   values[index+1]++;		// South.
		   owners[index+1] = player;
	       }
               values[index] = values[index] - maxValues[index];
               finished = false;
            }

            if (owners[index] != (Cube::Owner)player)
               playerWon = false;
         }
      }

      if (playerWon)
         return true;
   } while (! finished);

   return false;
}

void AI_Main::boxPrint (int side, int * owners, int * values)
{
   // IDW test. For debugging.
   for (int y = 0; y < side; y++) {
      fprintf (stderr, "   ");
      for (int x = 0; x < side; x++) {
	 int index = x * side + y;
	 if (owners[index] == Cube::Nobody) fprintf (stderr, "  .");
	 else fprintf (stderr, " %2d", (owners[index] == Cube::One) ?
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

void AI_Main::dumpStats()
{
   // IDW test. For debugging.
   for (int index = 0; index < m_nCubes; index++) {
      m_owners[index] = Cube::Nobody;
      m_values[index] = 1;
   }
   boxPrint (m_side, m_owners, m_values);
   foreach (MoveStats * m, m_moveStats) {
      QList<int> l;
      for (int n = 0; n <= m_maxLevel; n++) {
         l.append (m->searchStats->at(n)->n_moves);
      }
      qDebug() << m->player << m->moveNo << "X" << m->x << "Y" << m->y
               << "value" << m->value << m->n_simulate << m->n_assess << l;
      bool won = simulateMove ((CubeBox::Player) m->player, m->x, m->y,
                               m_side, m_owners, m_values, m_maxValues);
      boxPrint (m_side, m_owners, m_values);
      qDeleteAll (*(m->searchStats));
      delete m;
   }
   m_moveStats.clear();

   delete [] m_owners; // IDW test. Should be in getMove().
   delete [] m_values; // IDW test. Should be in getMove().
   delete [] m_maxValues; // IDW test. Should be in getMove().
}
