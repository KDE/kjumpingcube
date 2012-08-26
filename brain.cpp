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

#include "brain.h"
#include "cube.h"
#include "ai_kepler.h"
#include "ai_newton.h"

#include <QApplication>
#include <math.h>


// #define DEBUG // Uncomment this to get Brain's debug messages
#include <assert.h>

#ifdef DEBUG
#include <QDebug>
#endif

#include "prefs.h"

Brain::Brain(int initValue)
{
   // m_currentAI = new AI_Newton();
   m_currentAI = new AI_Kepler();
   m_ai[0] = 0;
   m_ai[1] = m_currentAI;
   m_ai[2] = m_currentAI;
   // m_ai[2] = new AI_Kepler();

   setSkill (Prefs::EnumSkill1::Beginner);
   stopped =false;
   active = false;
   currentLevel = 0;

   // initialize the random number generator
   random.setSeed (initValue);
}

void Brain::setSkill (int newSkill)
{
   _skill = newSkill;

   switch (_skill) {
   case Prefs::EnumSkill1::Beginner:
      maxLevel = 1;
      break;
   case Prefs::EnumSkill1::Average:
      maxLevel = 3;
      // IDW test. maxLevel = 1; // IDW test.
      break;
   case Prefs::EnumSkill1::Expert:
      maxLevel = 5;
      break;
   default:
      break;
   }
}

int Brain::skill() const
{
   return _skill;
}

void Brain::stop()
{
   stopped = true;
}

bool Brain::isActive() const
{
   return active;
}

bool Brain::getMove (int& row, int& column,CubeBox::Player player ,CubeBox box)
{
   if (isActive())
      return false;

   qDebug() << "Entered bool Brain::getMove(): player" << player;
   m_currentAI = m_ai[player];
   active = true;
   stopped = false;
   currentPlayer = player;

   int i = 0, j = 0;
   int moves = 0; // how many moves are the favorable ones
   CubeBox::Player opponent = (player == CubeBox::One) ?
                               CubeBox::Two : CubeBox::One;

   // if more than one cube has the same rating this array is used to select
   // one
   coordinate * c2m = new coordinate [box.dim() * box.dim()];

   // Array, which holds the assessment of the separate moves
   double ** worth = new double * [box.dim()];
   for (i = 0; i < box.dim(); i++)
      worth[i] = new double [box.dim()];

   double min = -pow (2.0, sizeof(long int) * 8. - 1);

   for (i = 0; i < box.dim(); i++)
      for (j = 0; j < box.dim(); j++) {
         worth[i][j] = min;
      }

   // find the favorable cubes to increase
   moves = findCubes2Move (c2m, player, box);
   qDebug() << "Player" << player << "likely moves" << moves << ".";

   // if only one cube is found, then don't check recursively the move
   if (moves == 1) {
#ifdef DEBUG
      qDebug() << "found only one favorable cube";
#endif
      qDebug() << "Move X" << c2m[0].row << "Y" << c2m[0].column
               << "value" << c2m[0].val;
      row = c2m[0].row;
      column = c2m[0].column;
   }
   else {
#ifdef DEBUG
      qDebug() << "found more than one favorable cube: " << moves;
#endif
      for (i = 0; i < moves; i++) {
         // if Thinking process stopped
         if (stopped) {
#ifdef DEBUG
             qDebug() << "brain stopped";
#endif
             break;	// Go evaluate the best move calculated so far.
         }

#ifdef DEBUG
         qDebug() << "checking cube " << c2m[i].row << "," << c2m[i].column;
#endif
         // Simulate every likely move and store the assessment.
         worth[c2m[i].row][c2m[i].column] =
                          doMove (c2m[i].row, c2m[i].column, player, box);

#ifdef DEBUG
         qDebug() << "cube "  << c2m[i].row << "," << c2m[i].column << " : "
                  << worth[c2m[i].row][c2m[i].column];
#endif
      }


      // Find the maximum
      double max = -1E99;

#ifdef DEBUG
      qDebug() << "Searching for the maximum";
#endif

      for (i = 0; i < moves; i++) {
         if (box[c2m[i].row][c2m[i].column]->owner() != (Cube::Owner)opponent) {
            if (worth[c2m[i].row][c2m[i].column] > max) {
               max = worth[c2m[i].row][c2m[i].column];
            }
	    qDebug() << "Move X" << c2m[i].row << "Y" << c2m[i].column
		     << "value" << c2m[i].val
		     << "worth" << worth[c2m[i].row][c2m[i].column]
		     << "max" << max;
         }
      }

#ifdef DEBUG
      qDebug() << "found Maximum : " << max;
#endif

      // found maximum more than one time ?
      int counter = 0;
      for (i = 0; i < moves; i++) {
#ifdef DEBUG
         qDebug() << c2m[i].row << "," << c2m[i].column << " : "
                  << worth[c2m[i].row][c2m[i].column];
#endif
         if (worth[c2m[i].row][c2m[i].column] == max)
            if (box[c2m[i].row][c2m[i].column]->owner() !=
               (Cube::Owner)opponent) {
               c2m[counter].row = c2m[i].row;
               c2m[counter].column = c2m[i].column;
               counter++;
            }
      }

      assert (counter > 0);

      // if some moves are equal, choose a random one
      if (counter > 1) {

         qDebug() << "Choosing a random cube, from" << counter; // IDW test.
#ifdef DEBUG
         qDebug() << "choosing a random cube: ";
#endif
         counter = random.getLong (counter);
      }
      // else { // IDW test. If there is a single maximum it should be chosen.
          // counter = 0; // IDW test.
          // qDebug() << "Choosing a single maximum"; // IDW test.
      // } // IDW test.

      row = c2m[counter].row;
      column = c2m[counter].column;
      qDebug() << "CHOSEN CUBE: X" << row << "Y" << column;
#ifdef DEBUG
      qDebug() << "cube: " << row << "," << column;
#endif
   }

   // clean up
   for (i = 0; i < box.dim(); i++)
      delete[] worth[i];
   delete [] worth;

   delete [] c2m;

   active = false;

   return (! stopped);
}

double Brain::doMove (int row, int column, CubeBox::Player player, CubeBox box)
{
   double worth = 0;
   currentLevel++; // increase the current depth of recurse calls

   QString tag = QString("").leftJustified (currentLevel * 2, '-');
   tag = tag % " doMove(";
   tag = tag.leftJustified (16, ' ');
   qDebug() << tag << row << column << player
            << "level" << currentLevel << "maxLevel" << maxLevel;
   // if the maximum depth isn't reached
   if (currentLevel < maxLevel) {
       // test, if possible to increase this cube
      if (! box.simulateMove (player, row, column)) {
         currentLevel--;
         return 0;
      }

      // if the player has won after simulating this move, return the assessment of the field
      if (box.playerWon(player)) {
         currentLevel--;

         // return (long int) pow ((float)box.dim() * box.dim(),
         double result = pow ((float)box.dim() * box.dim(),
                                (maxLevel - currentLevel)) *
	                        m_currentAI->assessField (currentPlayer, box);
	 qDebug() << "PLAYER WON" << player << "currentPlayer" << currentPlayer << "result" << result;
	 return result;
      }


      int i;
      int moves = 0;
      // If more than one cube has the same rating, select one from this array. 
      coordinate * c2m = new coordinate[box.dim() * box.dim()];

      // The next move is for the other player.
      player = (player == CubeBox::One) ? CubeBox::Two : CubeBox::One;

      // Find likely cubes to move.
      moves = findCubes2Move (c2m, player, box);

      // If only one cube is found, do not check the move recursively.
      if (moves == 1) {
         box.simulateMove (player, c2m[0].row, c2m[0].column);
         worth = (long int) pow ((float) box.dim() * box.dim(),
                                 (maxLevel - currentLevel - 1)) *
                                 m_currentAI->assessField (currentPlayer, box);
	 qDebug() << "ONLY ONE MOVE at X" << c2m[0].column << "Y" << c2m[0].row << "result" << worth;
      }
      else {
         for (i = 0; i < moves; i++) {
            qApp->processEvents();

            // if thinking process stopped
            if (stopped) {
               currentLevel--;
               delete [] c2m;	// Fix to avoid a memory leak.
               return 0;
            }

            // simulate every possible move
	    qDebug() << "RECURSION at X" << c2m[i].column << "Y" << c2m[i].row << "result so far" << worth;
            worth += doMove (c2m[i].row, c2m[i].column, player, box);
         }
      }
      delete [] c2m;
      currentLevel--;
      return worth;
   }
   else {
      // If maximum depth of recursive calls is reached, return the assessment.
      currentLevel--;
      box.simulateMove (player, row, column);

      // return m_currentAI->assessField (currentPlayer, box);
      double result = m_currentAI->assessField (currentPlayer, box);
      qDebug() << "MAXIMUM LEVEL at X" << column << "Y" << row << "result" << result;
      return result;
   }
}

int Brain::findCubes2Move (coordinate * c2m,
                           CubeBox::Player player, CubeBox& box)
{
   int i, j;
   int opponent = (player == CubeBox::One) ? CubeBox::Two : CubeBox::One;
   int moves    = 0;
   int min      = 9999;

   if (_skill == Prefs::EnumSkill1::Beginner) {
      // Select the cubes with the most number of pips on them.
      int max = 0;
      for (i = 0; i < box.dim(); i++)
        for (j = 0; j < box.dim(); j++) {
           if (box[i][j]->owner() != opponent) {
              c2m[moves].row = i;
              c2m[moves].column = j;
              c2m[moves].val = box[i][j]->value();

              if (c2m[moves].val > max)
                 max = c2m[moves].val;

              // qDebug() << "cube" << i << j << "val" << c2m[moves].val << "max" << max;
              moves++;
           }
        }

      // Find all moves with maximum value.
      int counter=0;
      for (i = 0; i < moves; i++) {
         if (c2m[i].val == max) {
            c2m[counter].row = c2m[i].row;
            c2m[counter].column = c2m[i].column;
            c2m[counter].val = c2m[i].val;

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
      for (i = 0; i < box.dim(); i++)
        for (j = 0; j < box.dim(); j++) {
           // Use only cubes that do not belong to the opponent.
           // qDebug() << "\nCUBE" << i << j << "OWNER" << box[i][j]->owner() << "OPPONENT" << opponent;
           if (box[i][j]->owner() != opponent) {
              int val;

              // Check the neighbours of each cube.
              val = m_currentAI->assessCube (i, j, player, box);

#ifdef DEBUG
              if (currentLevel == 0)
                 qDebug() << i << "," << j << " : " << val;
#endif
              // Only if val > 0 is it a likely move.
              if ( val > 0 ) {
                 if (val < min) {
                    secondMin = min;
                    min = val;
                 }

                 // Store coordinates.
                 c2m[moves].row = i;
                 c2m[moves].column = j;
                 c2m[moves].val = val;
                 moves++;
              }
           }
        }
        if (currentLevel == 0)
        qDebug() << "\nMinimum is" << min << ", second minimum is" << secondMin;

        // If all cubes are bad, check all cubes for the next move.
        if (moves == 0) {
           min = 4;
           for (i = 0; i < box.dim(); i++)
              for (j = 0; j < box.dim(); j++) {
                 if (box[i][j]->owner() != opponent) {
                    c2m[moves].row = i;
                    c2m[moves].column = j;
                    c2m[moves].val = (box[i][j]->max() - box[i][j]->value());
                    if (c2m[moves].val < min)
                       min = c2m[moves].val;
                    moves++;
                 }
              }
        }

        int counter = 0;
        // Find all moves with minimum assessment
        for (i = 0; i < moves; i++) {
           if (c2m[i].val == min) {
              c2m[counter].row = c2m[i].row;
              c2m[counter].column = c2m[i].column;
              c2m[counter].val = c2m[i].val;

              counter++;
           }
           else if (_skill == Prefs::EnumSkill1::Average) {
              if (c2m[i].val == secondMin) {
                 c2m[counter].row = c2m[i].row;
                 c2m[counter].column = c2m[i].column;
                 c2m[counter].val = c2m[i].val;

                 counter++;
              }
           }
        }

        if (counter != 0) {
           moves = counter;
        }
   }

   // If more than maxMoves moves are favorable, take maxMoves random
   // moves because it will take too long to check more.
   int maxMoves = 10;
   if (moves > maxMoves) {
      coordinate * tempC2M = new coordinate[maxMoves];

      coordinate tmp = {-1,-1,0};
      for (i = 0; i < maxMoves; i++)
         tempC2M[i] = tmp;

      // This array holds the random numbers chosen.
      int * results = new int[moves];
      for (i = 0; i < moves; i++)
         results[i] = 0;

      for (i = 0; i < maxMoves; i++) {
         int temp;
         do {
            temp = random.getLong (moves);
         } while (results[temp] != 0);

         results[temp] = 1;

         tempC2M[i].row = c2m[temp].row;
         tempC2M[i].column = c2m[temp].column;
         tempC2M[i].val = c2m[temp].val;
      }
      delete [] results;

      for (i = 0; i < maxMoves; i++) {
         c2m[i].row = tempC2M[i].row;
         c2m[i].column = tempC2M[i].column;
         c2m[i].val = tempC2M[i].val;
      }
      delete [] tempC2M;

      moves = maxMoves;
   }

   return moves;
}
