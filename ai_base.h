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

#include "cubebox.h"
#include "cube.h"

/**
* A pure-virtual class that is a base for alternative KJumpinCube AI algorithms.
*/
class AI_Base
{
public:
   /**
   * @param initValue value to initialize the random number generator with
   *        if no value is given a truly random value is used
   */
   AI_Base()          {};
   virtual ~AI_Base() {};

   /**
   * Assess the priority of playing a cube at a particular position.  The
   * highest priority cubes are used in look-ahead moves and calculating the
   * values of the positions reached.
   *
   * @param row      The row-position of the cube
   * @param col      The column-position of the cube
   * @param player   The player who is to move
   * @param box      The whole grid of cubes in the box
   *
   * @return         < 0 - The move is invalid or wasteful
   *                 > 0 - The priority of a useful move (1 is highest)
   */
   virtual int assessCube (int row, int column,
                           CubeBox::Player player, CubeBox& box) const = 0;

   /**
    * Assess the value of a position reached after trying a move.  The move that
    * leads to the highest value is chosen by the caller or a random choice is
    * made among moves leading to positions of equal value.
    *
    * @param player  The player whose position is to be assessed
    * @param box     The state of the whole grid of cubes in the box
    *
    * @return        The value of the position
    */
   virtual double assessField (CubeBox::Player forWhom, CubeBox& box) const = 0;
};

#endif // AI_BASE_H
