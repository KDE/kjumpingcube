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
#include "cubebox.h"

class CubeBox;

/**
* Class AI_Newton computes a (good) possibility to move
* for a given playingfield.
*
* It puts a value on every cube by looking at its neighbours
* and searches the best cubes to move. It then simulates what would
* happen, if you would click on these cubes. This is done recursively
* to a certain depth and the playingfield will be valued.
*
* @short The games brain
*/
class AI_Newton : public AI_Base
{
public:
   /**
   * @param initValue value to initialize the random number generator with
   *        if no value is given a truly random value is used
   */
   AI_Newton();

   int    assessCube (int row,int column,CubeBox::Player,CubeBox& box) const;
   double assessField (CubeBox::Player forWhom, CubeBox& box) const;
};

#endif // AI_NEWTON_H
