/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 1998-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
  Copyright (C) 2012      by Ian Wadham <iandw.au@gmail.com>

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
#ifndef AI_KEPLER_H
#define AI_KEPLER_H

#include "ai_base.h"

/**
* Class AI_Kepler computes the value of moving a cube.  It assists the main AI
* class and contains the algorithm from the original KJumpingCube code.
*
* @short The Kepler AI algorithm
*/
class AI_Kepler : public AI_Base
{
public:
   QString whoami() { return QString ("Kepler"); } // IDW test.

   /**
   * The Kepler AI constructor.
   */
   AI_Kepler();

   /**
   * Assess the priority of playing a cube at a particular position.  The
   * highest priority cubes are used by the AI_Main class for look-ahead moves
   * and calculating the values of the positions reached.  The cube to be
   * assessed has to be neutral or owned by the player who is to move.
   *
   * @param index      The index-position of the cube to be assessed
   * @param player     The player who is to move
   * @param neighbors  The index-positions of the cube's neighbors (array),
   *                   where a value of -1 means no neighbor on that side
   * @param owners     The current owners of the cubes in the box (array)
   * @param values     The current point values of the cubes in the box (array)
   * @param maxValues  The maximum point values of the cubes in the box (array)
   *
   * @return           The priority of a move (always > 0): moves with priority
   *                   1 are best and those with priority >= HighValue (999) are
   *                   worst but may be forced (e.g. when defeat is imminent).
   */
   int assessCube (const int index,         const Player player,
                   const int neighbors [4], const Player owners[],
                   const int values[],      const int    maxValues[]) const;
};

#endif // AI_KEPLER_H
