/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ai_newton.h"

AI_Newton::AI_Newton()
{
}

int AI_Newton::assessCube (const int index,         const Player player,
                           const int neighbors [4], const Player owners[],
                           const int values[],      const int    maxValues[]
                          ) const
{
   enum Value {StrongerOpponent = HighValue,
               TakeOrBeTaken = 1, EqualOpponent, CanTake,
               OccupyCorner, OccupyEdge, OccupyCenter,
               CanConsolidate,
               CanReachMaximum, CanExpand, IncreaseEdge, IncreaseCenter,
               PlayHereAnyway};
   Player      p         = player;			// This player.
   Player      o         = (p == One) ?  Two : One;	// The other player.
   Player      cOwner    = owners[index];		// This cubes's owner.

   int         pCount = 0;
   int         pRank  = 4;
   int         oCount = 0;
   int         oRank  = 4;
   int         cRank  = maxValues[index] - values[index];
   int         pos    = 0;

   // Get statistics for neighbors: count and best rank for player and other.
   for (int i = 0; i < 4; i++) {
      if ((pos = neighbors [i]) < 0) {
         continue;			// No neighbor on this side.
      }
      int rank = maxValues[pos] - values[pos];
      if (owners[pos] == p) {		// Neighbor is owned by this player.
         pCount++;
         pRank = (rank < pRank) ? rank : pRank;
      }
      else if (owners[pos] == o) {	// Neighbor is owned by other player.
         oCount++;
         oRank = (rank < oRank) ? rank : oRank;
      }
      else {				// Otherwise, nobody owns it.
         oRank = (rank < oRank) ? rank : oRank;
      }
   }

   if (oRank < cRank)                	 return StrongerOpponent;
   // return PlayHereAnyway; // IDW test. Try ALL REASONABLE MOVES.

   if ((cRank <= 0) && (oRank <= 0))	 return TakeOrBeTaken;	// Value 1.
   if ((cRank == oRank) && (oCount > 0)) return EqualOpponent;	// Value 2.
   if ((cRank <= 0) && (oCount > 0))     return CanTake;	// Value 3.

   bool vacant  =  (cOwner == Nobody);
   bool nVacant = ((pCount + oCount) == 0);
   int  cMax    = maxValues[index];
   if (vacant && nVacant && (cMax == 2)) return OccupyCorner;	// Value 4.
   if (vacant && nVacant && (cMax == 3)) return OccupyEdge;	// Value 5.
   if (vacant && nVacant && (cMax == 4)) return OccupyCenter;	// Value 6.
   // Sun 2 Dec 2012 - This seems to play well on sizes 3, 5 and 7.
   return PlayHereAnyway; // IDW test. Ignore val > 6. Try ALL REASONABLE MOVES.
   if ((cRank <= 0)  && (pCount == 0))   return CanExpand;	// Value 9.
   if ((cRank <= 0)  && (pCount > 0))    return CanConsolidate;	// Value 7.
   if (cRank == 1)                       return CanReachMaximum;// Value 8.
   if (cMax == 3)                        return IncreaseEdge;	// Value 10.
   if (cMax == 4)                        return IncreaseCenter;	// Value 11.

   return PlayHereAnyway;					// Value 12.
}
