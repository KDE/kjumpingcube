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
#ifndef CUBEBOX_H
#define CUBEBOX_H

#include "cubeboxbase.h"

class Cube;
class KCubeBoxWidget;

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/**
* Class for storing information about the playingfield, e.g.
* to undo a move or computing the next move
*/
class CubeBox : public CubeBoxBase<Cube>
{
public:
   /**
   * constructs a CubeBox with 'dim' x 'dim' Cubes
   */
   CubeBox(const int dim=1);
   CubeBox(const CubeBox&);
   CubeBox(KCubeBoxWidget&);
   virtual ~CubeBox();
   
   CubeBox& operator= (const CubeBox& box);
   CubeBox& operator= (KCubeBoxWidget& box);
   
   bool simulateMove(Player fromWhom,int row, int column);
   double assessField(Player forWhom) const;
   bool playerWon(Player who) const;

private:
   void increaseNeighbours(CubeBox::Player forWhom,int row,int column);

};

#endif // CUBEBOX_H

