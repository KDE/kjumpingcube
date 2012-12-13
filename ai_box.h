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

#include <QObject>
#include <QList>

/**
* Class AI_Box
*
* @short The Box AI algorithms
*/

// Minimum and maximum size of cube box.  Must be consistent with settings.ui.
const int minSide = 3;
const int maxSide = 15;

class AI_Box : public QObject
{
    Q_OBJECT
public:
    /**
    * The KJumpingCube AI_Box constructor.
    */
    AI_Box          (QObject * parent = 0, int side = 5);
    virtual  ~AI_Box();

    int      side() const       { return m_side; }
    Player   owner    (int index) const
                      { return (((index >= 0) && (index < m_nCubes)) ?
                                m_owners [index] : Nobody); }
    int      value    (int index) const
                      { return (((index >= 0) && (index < m_nCubes)) ?
                                m_values [index] : 1); }
    int      maxValue (int index) const
                      { return (((index >= 0) && (index < m_nCubes)) ?
                                m_maxValues [index] : 4); }

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

    void     copyPosition (Player   player, bool   isAI);
    bool     undoPosition (Player & player, bool & isAI);
    bool     redoPosition (Player & player, bool & isAI);
    void     initPosition (const AI_Box * box, Player player, bool isAI);

    void     clear();
    bool     isClear()          { return (m_cubesToWin [Nobody] == 0); }

protected:
    int      m_side;
    int      m_nCubes;
    Player * m_owners;
    int *    m_values;
    int *    m_maxValues;
    int *    m_neighbors;

    void     resizeBox (int side);

private:
    typedef struct {
        Player   player;
        bool     isAI;
        int      nCubes;
        Player * owners;
        int *    values;
    } Position;

    int      m_cubesToWin [3];
    int *    m_stack;
    int      m_stackPtr;

    QList<Position *> m_undoList;
    int      m_undoIndex;
    int      m_redoLimit;

    void     indexNeighbors();

    void     save    (Position * position, Player player, bool isAI);
    void     restore (Position * position);
    void     discard (Position * position);
    Position * emptyPosition (int nCubes);
    void     createBox (int side);
    void     destroyBox();

    QObject * m_parent; // IDW test.
};

#endif // AI_BOX_H
