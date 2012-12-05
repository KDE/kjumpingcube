/* ****************************************************************************
  Copyright 2012 Ian Wadham <iandw.au@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************** */
#ifndef AI_BOX_H
#define AI_BOX_H

#include "ai_globals.h"		// Include Player enum.

#include <QList>

/**
* Class AI_Box
*
* @short The Box AI algorithms
*/

typedef struct {
    Player   player;
    bool     isAI;
    int      nCubes;
    Player * owners;
    int *    values;
} Position;

class AI_Box
{
friend class AI_Main;

public:
    /**
    * The KJumpingCube AI_Box constructor.
    */
    AI_Box          (int side);
    virtual  ~AI_Box();

    int      side()             { return m_side; }
    Player   owner  (int index) { return (((index >= 0) && (index < m_nCubes)) ?
                                          m_owners [index] : Nobody); }
    int      value  (int index) { return (((index >= 0) && (index < m_nCubes)) ?
                                          m_values [index] : 1); }
    int      maxValue (int index) { return (((index >= 0) && (index < m_nCubes))
                           ? m_maxValues [index] : 4); }

    // For performance, avoid setOwner() and setValue() in the game engine (AI).
    // However, they are good to use when loading a saved game, for example.
    void     setOwner (int index, Player owner)
                                { if ((index >= 0) && (index < m_nCubes) &&
                                      (owner >= Nobody) && (owner <= Two)) {
                                      if (owner != m_owners [index]) {
                                          m_cubesToWin [m_owners [index]] ++;
                                          m_cubesToWin [owner] --;
                                      }
                                      m_owners [index] = owner;
                                  } 
                                }
    void     setValue (int index, int value)
                                { if ((index >= 0) && (index < m_nCubes) &&
                                      (value >= 1)) {
                                      m_values [index] = value;
                                  } 
                                }

    bool     doMove  (Player player, int index, QList<int> * steps = 0);
    void     printBox();
    bool     oldMove (Player player, int index);

    void     copyPosition (Player   player, bool   isAI);
    bool     undoPosition (Player & player, bool & isAI);
    bool     redoPosition (Player & player, bool & isAI);
    void     initPosition (AI_Box * box, Player player, bool isAI);

    void     clear();
    bool     isClear()          { return (m_cubesToWin [Nobody] == 0); }

private:
    int      m_side;
    int      m_nCubes;
    Player * m_owners;
    int *    m_values;
    int *    m_maxValues;
    int      m_cubesToWin [3];
    int *    m_neighbors;
    int *    m_stack;
    int      m_stackPtr;

    QList<Position *> m_undoList;
    int      m_undoIndex;
    int      m_redoLimit;

    void     indexNeighbors();

    void     save    (Position * position, Player player, bool isAI);
    void     restore (Position * position);
    void     discard (Position * position);
};

#endif // AI_BOX_H
