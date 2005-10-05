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
#ifndef BRAIN_H
#define BRAIN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <krandomsequence.h>

#include "cubebox.h"

/** @internal */
struct coordinate
{
   int row;
   int column;
   
   int val;
};


/**
* Class Brain computes a (good) possibility to move
* for a given playingfield.
* 
* It puts a value on every cube by looking at its neighbours
* and searches the best cubes to move. It then simulates what would 
* happen, if you would click on these cubes. This is done recursively 
* to a certain depth and the playingfield will be valued. 
* 
* @short The games brain
*/
class Brain
{
public:
   /** 
   * @param initValue value to initialize the random number generator with
   *        if no value is given a truly random value is used
   */
   Brain(int initValue=0);

   /**
   * Computes a good possible move at the given field.
   * The index of this Cube is stored in given 'row' and 'column' 
   * 
   * @return false if computing was stopped
   * @see Brain#stop;  
   */
   bool getHint(int& row, int& column, CubeBox::Player player, CubeBox field);
   
   /** stops thinking */
   void stop();
   /** @return true if the Brain is thinking at the moment */
   bool isActive() const;

   /** skill according to Prefs::EnumSkill **/
   void setSkill(int);
   int skill() const;
   
private:
   /**
   * checks if a move is possible at cube row,column from player 'player' and
   * simulates this move. Then it checks the new playingfield for possible moves
   * and calls itself for every possible move until the maximum depth 'maxLevel'
   * is reached.
   *
   * If the maximum depth is reached, it puts a value on the playingfield and returns this.
   * @see CubeBox#simulateMove
   * @see CubeBox#assessField
   * @see Brain#findCubes2Move
   *
   * @param row,column coordinates of cube to increase
   * @param player for which player the cube has to be increased
   * @param box playingfield to do the moves on
   * @return the value put on the field
   */
    double doMove(int row,int column,CubeBox::Player player, CubeBox box);
   /**
   * Checks the given playingfield, which cubes are favourable to do a move
   * by checking every cubes neighbours. And looking for the difference to overflow.
   *
   * @param c2m Array in which the coordinates of the best cubes to move will be stored
   * @param player for which player to check
   * @param box playingfield to check
   * @param debug if debugmessages should be printed
   * @return number of found cubes to move
   */
   int findCubes2Move(coordinate* c2m,CubeBox::Player player,CubeBox& box);
   /**
   *
   */
   int assessCube(int row,int column,CubeBox::Player,CubeBox& box) const;
   int getDiff(int row,int column, CubeBox::Player player, CubeBox& box) const;

   /** current depth of recursive simulating of the moves */
   int currentLevel;
   /** maximum depth of recursive thinking */
   int maxLevel;
   /** the player for which to check the moves */
   CubeBox::Player currentPlayer;
	
	
   /** flag, if the engine has to be stopped */
   bool stopped;
   /** flag, if the engine is active */
   bool active;
   /** skill of the Brain, see Prefs::EnumSkill */
   int _skill;

   /** Sequence generator */
   KRandomSequence random;    
};

#endif //BRAIN_H
