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

#include "ai_newton.h"
#include "cube.h"

AI_Newton::AI_Newton()
{
}

int AI_Newton::assessCube (int row, int column, CubeBox::Player player,
                       CubeBox & box) const
{
   enum Value {InvalidMove = -2, StrongerOpponent = -1,
               TakeOrBeTaken = 1, EqualOpponent, CanTake,
               OccupyCorner, OccupyEdge, OccupyCenter,
               CanExpand, CanConsolidate,
               CanReachMaximum, IncreaseEdge, IncreaseCenter,
               PlayHereAnyway};
   Cube::Owner p         = (Cube::Owner) player;	// This player.
   Cube::Owner o         = (p == Cube::One) ?
                            Cube::Two : Cube::One;	// The other player.
   Cube *      c         = box[row][column];		// This cube.
   Cube::Owner cOwner    = c->owner();		 	// This cubes's owner.

   if (cOwner == o) return InvalidMove;			// Other player owns it.

   int         pCount    = 0;
   int         pRank = 4;
   int         oCount    = 0;
   int         oRank = 4;
   int         cRank = c->max() - c->value();	// Cube's  strength.
   Cube *      n[4];
   int         nCount = 0;

   // Get a list of neighbors.
   if (row != 0) {
       n[nCount++] = box[row-1][column];
   }
   if (row != box.dim()-1) {
       n[nCount++] = box[row+1][column];
   }
   if (column != 0) {
       n[nCount++] = box[row][column-1];
   }
   if (column != box.dim()-1) {
       n[nCount++] = box[row][column+1];
   }

   // Get statistics for neighbors: count and best rank for player and other.
   for (int i = 0; i < nCount; i++) {
      int rank;
      Cube * nb = n[i];
      if (nb->owner() == p) {		// Neighbor is owned by this player.
         pCount++;
         rank  = nb->max() - nb->value();	// Rank = how near to full.
         pRank = (rank < pRank) ? rank : pRank;
      }
      else if (nb->owner() == o) {	// Neighbor is owned by other player.
         oCount++;
         rank  = nb->max() - nb->value();	// Rank = how near to full.
         oRank = (rank < oRank) ? rank : oRank;
      }
      // Otherwise, nobody owns it.
   }

   if (oRank < cRank)                	return StrongerOpponent;

   if ((cRank <= 0) && (oRank <= 0))	return TakeOrBeTaken;
   if (cRank == oRank)               	return EqualOpponent;
   if ((cRank <= 0) && (oCount > 0))    return CanTake;

   bool vacant  =  (cOwner == Cube::Nobody);
   bool nVacant = ((pCount + oCount) == 0);
   int  cMax    = c->max();
   if (vacant && nVacant && (cMax == 2)) return OccupyCorner;
   if (vacant && nVacant && (cMax == 3)) return OccupyEdge;
   if (vacant && nVacant && (cMax == 4)) return OccupyCenter;
   if ((cRank <= 0)  && (pCount == 0))   return CanExpand;
   if ((cRank <= 0)  && (pCount > 0))    return CanConsolidate;
   if (cRank == 1)                       return CanReachMaximum;
   if (cMax == 3)                        return IncreaseEdge;
   if (cMax == 4)                        return IncreaseCenter;

   return PlayHereAnyway;
}

double AI_Newton::assessField (CubeBox::Player player, CubeBox& box) const
{
   int    cubesOne       = 0;
   int    cubesTwo       = 0;
   int    pointsOne      = 0;
   int    pointsTwo      = 0;
   CubeBox::Player otherPlayer = (player == CubeBox::One) ?
                                  CubeBox::Two : CubeBox::One;
   bool   playerWon      = true;
   bool   otherPlayerWon = true;

   int d = box.dim();
   int i, j;

   for (i = 0; i < d; i++) {
      for (j = 0; j < d; j++) {
         Cube * cube = box[i][j];
	 int points  = cube->value();
         if (cube->owner() == (Cube::Owner)CubeBox::One) {
            cubesOne++;
            pointsOne += points * points;
         }
         else if (cube->owner() == (Cube::Owner)CubeBox::Two) {
            cubesTwo++;
	    pointsTwo += points * points;
         }

         if(cube->owner() != (Cube::Owner)player)
            playerWon = false;

         if(cube->owner() != (Cube::Owner)otherPlayer)
            otherPlayerWon = false;
      }
   }

   if (player == CubeBox::One) {
      return cubesOne * cubesOne + pointsOne - cubesTwo * cubesTwo - pointsTwo;
   }
   else {
      return cubesTwo * cubesTwo + pointsTwo - cubesOne * cubesOne - pointsOne;
   }
}
