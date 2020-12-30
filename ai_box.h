/*
    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

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
    explicit AI_Box          (QObject * parent = nullptr, int side = 5);
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

    // This struct is passed to doMove() and is used to store
    // everything that is needed by undoMove() to actually undo it.
    struct MoveUndodata {
        Player oldPlayer;       // The player previously to move in the position
        int    oldCubesToWin[3];
        quint16 changedCubes[maxSide * maxSide]; // 8 bits index, 4 bits owner and 4 bits value
                                                 // end with 0xffff
    };

    bool     doMove   (Player player, int index,
                       MoveUndodata * undodata = nullptr, QList<int> * steps = nullptr);
    void     undoMove (MoveUndodata * undodata);
#if AILog > 0
    void     printBox();
#endif

    void     copyPosition (Player   player, bool   isAI, int index);
    bool     undoPosition (Player & player, bool & isAI, int & index);
    bool     undoPosition (Player & player);
    bool     redoPosition (Player & player, bool & isAI, int & index);
    void     initPosition (const AI_Box * box, Player player, bool isAI);

    void     clear();

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
        int      index;
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
