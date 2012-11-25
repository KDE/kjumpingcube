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
   * highest priority cubes are used by the Brain class for look-ahead moves
   * and calculating the values of the positions reached.
   *
   * @param row      The row-position of the cube
   * @param col      The column-position of the cube
   * @param player   The player who is to move
   * @param box      The whole grid of cubes in the box
   *
   * @return         < 0 - The move is invalid or wasteful
   *                 > 0 - The priority of a useful move (1 is highest)
   */
   virtual int assessCube (int row, int col, Player player,
                           int side, Player * owners, int * values,
                           int * maxValues) const = 0;

   /**
    * Assess the value of a position reached after trying a move.  Moves that
    * lead to the highest values are chosen by the main AI class.
    *
    * @param player  The player whose position is to be assessed
    * @param box     The state of the whole grid of cubes in the box
    *
    * @return        The value of the position
    */
   virtual double assessField (Player player, int side,
                               Player * owners, int * values) const = 0;
};

#endif // AI_BASE_H
