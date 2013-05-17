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
