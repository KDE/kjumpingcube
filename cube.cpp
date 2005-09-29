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
#include "cube.h"
#include <assert.h>

/* ****************************************************** **
**                      Class Cube                        **  
** ****************************************************** */

Cube::Cube(Owner owner,int value,int maximum)
{
   _owner = owner;
   _value = value;
   _max = maximum;
}


Cube::Owner Cube::setOwner(Owner owner)
{
   Owner old=_owner;
   _owner=owner;
   
   return old;
}

void Cube::setValue(int value)
{
#ifdef DEBUG
   assert(value>0);
#endif
         
   _value = (value<1)? 1 : value;
}


void Cube::setMax(int max)
{
#ifdef DEBUG
   assert(max>1);
#endif
   
   _max = (max<2)? 2 : max;
}


void Cube::decrease()
{
   setValue(_value-_max);
}

Cube::Owner Cube::owner() const
{
   return _owner;
}


int Cube::value() const
{
   return _value;
}

bool Cube::increase(Owner newOwner)
{
   setValue(value()+1);
   setOwner(newOwner);
   
   return (_value > _max);
}

int Cube::max() const
{
   return _max;
}


bool Cube::overMax() const
{
   return (_value > _max);   
}

