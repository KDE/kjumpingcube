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
#ifndef CUBE_H
#define CUBE_H


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/**
* This Class is the internal representation of a cube.
*/
class Cube
{
public:   
   enum Owner{Nobody=0,One=1,Two=2};

   /**
   * constructs a Cube
   */
   Cube(Owner owner=Nobody,int value=1,int max=4);   
   
   
   /**
   * changes owner of the Cube
   * @return old Owner
   */
   virtual Owner setOwner(Owner owner);
   
   /**
   * changes value of the Cube 
   */
   virtual void setValue(int value);
   
   /**
   * sets maximum value of the Cube
   */
   virtual void setMax(int max);
   
   /**
   * increase the value of the Cube and set the owner of the Cube
   * to 'newOwner'.
   * @return true if the Cube's new value is over maximum
   */
   virtual bool increase(Owner newOwner);
   
   /**
   * substracts the maximum from the Cube's value  
   */
   virtual void decrease();
   
   /**
   * returns current owner
   */
   Owner owner() const;
   /**
   * returns current value
   */
   int value() const;
   /**
   * returns the maximum value of the cube
   */
   int max() const;   
   
   /**
   * checks if the Cube's value is over maximum
   */
   bool overMax() const;
   
private:

   Owner _owner;
   int _value;
   int _max;
   
};


#endif
