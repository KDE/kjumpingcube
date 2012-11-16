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
#ifndef AI_NEWTON_H
#define AI_NEWTON_H

#include "ai_base.h"
// IDW test. #include "cubebox.h"

// IDW test. class CubeBox;

/**
* Class AI_Newton computes the priority of moving a cube and the value of the
* resulting position.  It assists the main AI class.
*
* @short The Newton AI algorithms
*/
class AI_Newton : public AI_Base
{
public:
   QString whoami() { return QString ("Newton"); } // IDW test.

   /**
   * The Newton AI constructor.
   */
   AI_Newton();

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
   // IDW test. int    assessCube (int row, int col, CubeBox::Player, CubeBox& box) const;
   int    assessCube (int row, int col, Player player, int side,
                      int * owners, int * values, int * maxValues) const;

   /**
    * Assess the value of a position reached after trying a move.  The move that
    * leads to the highest value is chosen by the Brain class or a random choice
    * is made among moves leading to positions of equal value.
    *
    * @param player  The player whose position is to be assessed
    * @param box     The state of the whole grid of cubes in the box
    *
    * @return        The value of the position
    */
   // IDW test. double assessField (CubeBox::Player player, CubeBox& box) const;
   double assessField (Player player,
                       int side, int * owners, int * values) const;
};

#endif // AI_NEWTON_H
