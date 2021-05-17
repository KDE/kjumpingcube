/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ai_kepler.h"

AI_Kepler::AI_Kepler()
{
}

int AI_Kepler::assessCube (const int index,         const Player player,
                           const int neighbors [4], const Player owners[],
                           const int values[],      const int    maxValues[]
                          ) const
{
   int diff = VeryHighValue;
   int temp = VeryHighValue;
   int pos  = 0;

   // Check the neighbors.
   for (int i = 0; i < 4; i++) {
      if ((pos = neighbors [i]) >= 0) {
         temp = (owners [pos] != player) ? maxValues[index] - values[index]
                                         : maxValues[index] - values[index] + 1;
         if (temp < diff) {
            diff = temp;
	 }
      }
      // Else, do nothing: no neighbor on this side.
   }

   int val;
   temp = maxValues[index] - values[index];
   val  = diff - temp + 1;
   val  = val * (temp + 1);
   if (val <= 0) {
      val = HighValue - val;		// Always return values > 0.
   }

   return val;
}
