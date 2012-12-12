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
#ifndef AI_BASE_H
#define AI_BASE_H

#include "ai_globals.h"		// Include Player enum.

#include <QString> // IDW test.

/**
* A pure-virtual class that is a base for alternative KJumpinCube AI algorithms.
*/
class AI_Base
{
public:
   virtual QString whoami() = 0; // IDW test.

   AI_Base()          {};
   virtual ~AI_Base() {};

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
    *                   1 are best and those with priority >= HighValue (999)
    *                   are worst but may be forced (e.g. when defeat is near).
    */
   virtual int assessCube (const int index, const Player player,
                           const int neighbors [4], const Player owners[],
                           const int values[], const int maxValues[]) const = 0;

   /**
   * Assess the value of a position reached after trying a move.  Moves that
   * lead to the highest values are chosen by the main AI class.
   *
   * @param player  The player whose position is to be assessed
   * @param nCubes  The number of cubes in the box
   * @param owners     The current owners of the cubes in the box (array)
   * @param values     The current point values of the cubes in the box (array)
   *
   * @return        The value of the position
   */
   virtual long assessPosition (const Player player,   const int nCubes,
                                const Player * owners, const int * values
                               ) const;
};

#endif // AI_BASE_H
