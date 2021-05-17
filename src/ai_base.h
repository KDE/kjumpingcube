/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
