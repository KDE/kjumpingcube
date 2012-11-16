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

#include "ai_kepler.h"
#include "cube.h"

AI_Kepler::AI_Kepler()
{
}

int AI_Kepler::assessCube (int row, int col, Player player,
                           int side, int * owners, int * values,
                           int * maxValues) const
{
   int diff = 10000, temp = 10000;

   // Check the neighbors.
   if (row > 0) {
      temp = getDiff (row - 1, col, player, side, owners, values, maxValues);
      if (temp < diff)
         diff = temp;
   }
   if (row < side-1) {
      temp = getDiff (row + 1, col, player, side, owners, values, maxValues);
      if (temp < diff)
         diff = temp;
   }
   if (col > 0) {
      temp = getDiff (row, col - 1, player, side, owners, values, maxValues);
      if (temp < diff)
         diff = temp;
   }
   if (col < side-1) {
      temp = getDiff (row, col + 1, player, side, owners, values, maxValues);
      if (temp < diff)
         diff = temp;
   }

   int index = row * side + col;
   temp = maxValues[index] - values[index];

   int val;
   val = diff - temp + 1;
   val = val * (temp + 1);
   if (val <= 0) {
      val = 999 - val;
   }

   return val;
}

double AI_Kepler::assessField (Player player,
                               int side, int * owners, int * values) const
{
   int    cubesOne       = 0;
   int    cubesTwo       = 0;
   int    pointsOne      = 0;
   int    pointsTwo      = 0;
   Player otherPlayer    = (player == One) ? Two : One;
   int x, y, index, points;

   for (x = 0; x < side; x++) {
      for (y = 0; y < side; y++) {
	 index = x * side + y;
	 points  = values[index];
         if (owners[index] == One) {
            cubesOne++;
            pointsOne += points * points;
         }
         else if (owners[index] == Two) {
            cubesTwo++;
	    pointsTwo += points * points;
         }
      }
   }

   if (player == One) {
      return cubesOne * cubesOne + pointsOne - cubesTwo * cubesTwo - pointsTwo;
   }
   else {
      return cubesTwo * cubesTwo + pointsTwo - cubesOne * cubesOne - pointsOne;
   }
}

int AI_Kepler::getDiff (int row, int col, Player player, int side,
                        int * owners, int * values, int * maxValues) const
{
   int diff;
   int index = row * side + col;

   if (owners[index] != (Cube::Owner)player) {
      diff = maxValues[index] - values[index];
   }
   else {
      diff = maxValues[index] - values[index] + 1;
   }

   return diff;
}
