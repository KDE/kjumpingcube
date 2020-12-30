/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef AI_KEPLER_H
#define AI_KEPLER_H

#include "ai_base.h"

/**
* Class AI_Kepler computes the value of moving a cube.  It assists the main AI
* class and contains the algorithm from the original KJumpingCube code.
*
* @short The Kepler AI algorithm
*/
class AI_Kepler : public AI_Base
{
public:
   QString whoami() override { return QStringLiteral ("Kepler"); } // IDW test.

   /**
   * The Kepler AI constructor.
   */
   AI_Kepler();

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
   *                   1 are best and those with priority >= HighValue (999) are
   *                   worst but may be forced (e.g. when defeat is imminent).
   */
   int assessCube (const int index,         const Player player,
                   const int neighbors [4], const Player owners[],
                   const int values[],      const int    maxValues[]) const override;
};

#endif // AI_KEPLER_H
