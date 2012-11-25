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

AI_Newton::AI_Newton()
{
}

int AI_Newton::assessCube (int row, int col, Player player,
                           int side, Player * owners, int * values,
                           int * maxValues) const
{
   enum Value {InvalidMove = -2, StrongerOpponent = 999,
               TakeOrBeTaken = 1, EqualOpponent, CanTake,
               OccupyCorner, OccupyEdge, OccupyCenter,
//               CanExpand, CanConsolidate, // 1 Aug 2012, changed the order.
//               CanReachMaximum, IncreaseEdge, IncreaseCenter,
               CanConsolidate,
               CanReachMaximum, CanExpand, IncreaseEdge, IncreaseCenter,
               PlayHereAnyway};
   Player      p         = player;			// This player.
   Player      o         = (p == One) ?  Two : One;	// The other player.
   int         index     = row * side + col;		// This cube.
   Player      cOwner    = owners[index];		// This cubes's owner.

   if (cOwner == o)
      return InvalidMove;	// ERROR: The other player owns this cube.

   int         pCount = 0;
   int         pRank  = 4;
   int         oCount = 0;
   int         oRank  = 4;
   int         cRank  = maxValues[index] - values[index];
   int         n[4];
   int         nCount = 0;

   // Get a list of neighbors.
   if (row > 0) {
       n[nCount++] = index - side;
   }
   if (row < side-1) {
       n[nCount++] = index + side;
   }
   if (col > 0) {
       n[nCount++] = index - 1;
   }
   if (col < side-1) {
       n[nCount++] = index + 1;
   }

   // Get statistics for neighbors: count and best rank for player and other.
   for (int i = 0; i < nCount; i++) {
      int rank;
      int pos = n[i];
      if (owners[pos] == p) {		// Neighbor is owned by this player.
         pCount++;
         rank  = maxValues[pos] - values[pos];	// Rank = how near to full.
         pRank = (rank < pRank) ? rank : pRank;
      }
      else if (owners[pos] == o) {	// Neighbor is owned by other player.
         oCount++;
         rank  = maxValues[pos] - values[pos];	// Rank = how near to full.
         oRank = (rank < oRank) ? rank : oRank;
      }
      // Otherwise, nobody owns it.
   }

   if (oRank < cRank)                	 return StrongerOpponent;
   // return PlayHereAnyway; // IDW test. Try ALL REASONABLE MOVES.

   if ((cRank <= 0) && (oRank <= 0))	 return TakeOrBeTaken;
   if (cRank == oRank)               	 return EqualOpponent;
   if ((cRank <= 0) && (oCount > 0))     return CanTake;

   bool vacant  =  (cOwner == Nobody);
   bool nVacant = ((pCount + oCount) == 0);
   int  cMax    = maxValues[index];
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

double AI_Newton::assessField (Player player,
                               int side, Player * owners, int * values) const
{
   int    cubesOne       = 0;
   int    cubesTwo       = 0;
   int    pointsOne      = 0;
   int    pointsTwo      = 0;
   Player otherPlayer = (player == One) ? Two : One;
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
