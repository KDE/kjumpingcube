/*
    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ai_box.h"
// 
#include "kjumpingcube_debug.h"
#include <cstdio>

AI_Box::AI_Box (QObject * parent, int side)
    :
    QObject      (parent)
{
    m_parent = parent; // IDW test.
    createBox (side);
}

AI_Box::~AI_Box()
{
    destroyBox();
}

void AI_Box::createBox (int side)
{
    m_side         = ((side >= minSide) && (side <= maxSide)) ? side : 5;
    m_nCubes       = side * side;
    Position * pos = emptyPosition (m_nCubes);
    pos->player    = One;
    pos->isAI      = false;
    pos->index     = 0;
    m_maxValues    = new int [m_nCubes];
    m_neighbors    = new int [4 * m_nCubes];
    m_stack        = new int [m_nCubes];
    m_stackPtr     = -1;
    m_owners       = pos->owners;
    m_values       = pos->values;
    indexNeighbors();
    clear();
    m_undoList.append (pos);
}

void AI_Box::destroyBox()
{
    delete[] m_maxValues;
    delete[] m_neighbors;
    delete[] m_stack;
    while (! m_undoList.isEmpty()) {
	discard (m_undoList.takeLast());
    }
}

void AI_Box::resizeBox (int side)
{
    destroyBox();
    createBox (side);
}


// ----------------------------------------------------------------
//                         Moves


bool AI_Box::doMove (Player player, int index, MoveUndodata * undodata, QList<int> * steps)
{
    // Check for move legality.
    Player oldOwner = m_owners[index];
    if ((oldOwner != player) && (oldOwner != Nobody)) {
	qCDebug(KJUMPINGCUBE_LOG) << "ILLEGAL MOVE: player" << player << "old" << oldOwner
                 << "at" << index/m_side << index%m_side;

        return false;			// The move is not valid.
    }

    if (undodata) {
        undodata->oldCubesToWin[Nobody] = m_cubesToWin[Nobody];
        undodata->oldCubesToWin[One] = m_cubesToWin[One];
        undodata->oldCubesToWin[Two] = m_cubesToWin[Two];
    }

    // Bitfield to mark saved cube indices.
    quint64 savedCubes[(maxSide * maxSide - 1) / 64 + 1];
    for (int i = 0; i < (maxSide * maxSide - 1) / 64 + 1; ++i)
        savedCubes[i] = 0ULL;

    // Save old values of changed cubes (owner + value) into the
    // MoveUndodata to be restored by undoMove().
    int saveCubes = 0;
    if (undodata) {
        undodata->changedCubes[saveCubes++] = ((index << 8) 
                                               | (m_owners[index] << 4)
                                               | (m_values[index] << 0));
        savedCubes[index / 64] |= 1ULL << (index % 64);

    }

    m_stackPtr = -1;
    m_owners[index] = player;		// Take ownership if not already owned.
    if (m_maxValues [index] == m_values [index]++) {	// Increase the cube.
	m_stack [++m_stackPtr] = index;	// Stack an expansion step.
    }
    if (steps) {
        steps->append (index + 1);	// Record the beginning of the move.
    }
    if (oldOwner != player) {		// If owner changed, update cubesToWin.
        m_cubesToWin [oldOwner]++;
        m_cubesToWin [player]--;
        if (m_cubesToWin [player] <= 0) {
            // Append 0 to the move-step list and return player-won.
            if (steps) {
                steps->append (0);
            }
            // printBox();
            return true;;
        }
    }

    while (m_stackPtr >= 0) {
        // Pop the stack and decrease an overloaded cube.
	index = m_stack [m_stackPtr--];

        m_values[index] = m_values[index] - m_maxValues[index];

        // Append -index-1 to move list, if not still overloaded.
        if (steps && (m_values[index] <= m_maxValues[index])) {
            steps->append (-index - 1);	// Record the end of a step or move.
        }

        // Move each neighbor.
        int indexN;
	int offset = index * 4;
        for (int nb = 0; nb < 4; nb++) {
            if ((indexN = m_neighbors [offset + nb]) < 0)
                continue;		// No neighbor on this side.

            if (undodata && !(savedCubes[indexN / 64] & (1ULL << (indexN % 64)))) {
                undodata->changedCubes[saveCubes++] = ((indexN << 8) 
                                                       | (m_owners[indexN] << 4)
                                                       | (m_values[indexN] << 0));
                savedCubes[indexN / 64] |= 1ULL << (indexN % 64);
            }

            // Increase the neighbor and take over ownership.
            oldOwner = m_owners[indexN];
            m_owners[indexN] = player;
            if (m_maxValues [indexN] == m_values [indexN]++) {
                // Continue a cascade by pushing a new step onto the stack.
                m_stack [++m_stackPtr] = indexN;
            }
            if (steps) {
                steps->append (indexN + 1); // Record beginning of move-step.
            }
            if (oldOwner != player) {	// If owner changed, update cubesToWin.
                m_cubesToWin [oldOwner]++;
                m_cubesToWin [player]--;
            }
        }
        if (m_values[index] > m_maxValues[index]) {
            // The cube is still overloaded, so push it back onto the stack.
            m_stack [++m_stackPtr] = index;
        }
        if (m_cubesToWin [player] <= 0) {
            // Append 0 to the move-step list and return player-won.
            if (steps) {
                steps->append (0);
            }

            // Mark the end of changed cubes in the undodata.
            if (undodata) {
                undodata->changedCubes[saveCubes++] = 0xffff;
            }

            // printBox();
            return true;
        }
        // printBox();
    } // End while()

    if (undodata) {
        undodata->changedCubes[saveCubes++] = 0xffff;
    }

    // printBox();
    return false;
}

void AI_Box::undoMove (MoveUndodata * undodata)
{
    m_cubesToWin[Nobody] = undodata->oldCubesToWin[Nobody];
    m_cubesToWin[One] = undodata->oldCubesToWin[One];
    m_cubesToWin[Two] = undodata->oldCubesToWin[Two];

    for (int i = 0; undodata->changedCubes[i] != 0xffff; ++i) {
        int index = (undodata->changedCubes[i] >> 8) & 0xff;
        m_owners[index] = Player((undodata->changedCubes[i] >> 4) & 0xf);
        m_values[index] = (undodata->changedCubes[i] >> 0) & 0xf;
    }
}


// ----------------------------------------------------------------
//                         Game history


void AI_Box::copyPosition (Player player, bool isAI, int index)
{
#if AILog > 4
    qCDebug(KJUMPINGCUBE_LOG) << "AI_Box::copyPosition (" << player << "," << isAI << ")";
    printBox();
#endif
    if (m_undoIndex >= m_undoList.count()) {
#if AILog > 4
	qCDebug(KJUMPINGCUBE_LOG) << "Call emptyPosition (" << m_nCubes << ")";
#endif
        m_undoList.append (emptyPosition (m_nCubes));
    }
#if AILog > 4
    qCDebug(KJUMPINGCUBE_LOG) << "m_undoIndex" << m_undoIndex << "m_undoList.count()" << m_undoList.count();
#endif
    Position * pos = m_undoList.at (m_undoIndex);
    save (pos, player, isAI);
    pos->index = index; // IDW TODO - Do this in save()?
    m_owners = pos->owners;
    m_values = pos->values;
#if AILog > 4
    printBox();
#endif
    m_undoIndex++;
    m_redoLimit = m_undoIndex;
}

bool AI_Box::undoPosition (Player & player, bool & isAI, int & index)
{
    bool result = undoPosition (player);
    if (m_undoIndex > 0) {
        Position * pos = m_undoList.at (m_undoIndex);
        isAI   = pos->isAI;
        index  = pos->index;
    }
    return result;
}

bool AI_Box::undoPosition (Player & player)
{
    if (m_undoIndex > 1) {
	m_undoIndex--;
	Position * pos = m_undoList.at (m_undoIndex - 1);
	restore (pos);
	player = pos->player;
    }
#if AILog > 4
    qCDebug(KJUMPINGCUBE_LOG) << "AI_Box::undoPosition (player =" << player << "), m_undoIndex" << m_undoIndex << "UNDONE POSITION";
    printBox();
#endif
    return (m_undoIndex > 1);
}

bool AI_Box::redoPosition (Player & player, bool & isAI, int & index)
{
    if (m_undoIndex < m_redoLimit) {
	Position * pos = m_undoList.at (m_undoIndex);
	restore (pos);
	player = pos->player;
	isAI   = pos->isAI;
	index  = pos->index;
	m_undoIndex++;
    }
#if AILog > 4
    qCDebug(KJUMPINGCUBE_LOG) << "AI_Box::redoPosition (player =" << player << "), m_undoIndex" << m_undoIndex << "REDONE POSITION";
    printBox();
#endif
    return (m_undoIndex < m_redoLimit);
}

void AI_Box::initPosition (const AI_Box * box, Player player, bool isAI)
{
    if (box->side() != m_side) return;

    Position * pos = m_undoList.at (0);
    m_owners = pos->owners;
    m_values = pos->values;

    for (int n = 0; n < m_nCubes; n++) {
        m_owners [n] = box->owner (n);
        m_values [n] = box->value (n);
    }
    restore (pos);
    pos->player = player;
    pos->isAI   = isAI;
    // IDW TODO - Need index parameter? initPosition() is used only by AI_Main.
    pos->index  = 0;
    m_undoIndex = 1;
    m_redoLimit = m_undoIndex;
}

void AI_Box::save (Position * pos, Player player, bool isAI)
{
    // The NEXT player will face this position, after THIS player has moved.
    pos->player = (player == Two) ? One : Two;
    pos->isAI   = isAI;
    pos->nCubes = m_nCubes;
    for (int n = 0; n < m_nCubes; n++) {
	pos->owners [n] = m_owners [n];
	pos->values [n] = m_values [n];
    }
}

void AI_Box::restore (Position * pos)
{
    if (pos->nCubes != m_nCubes) return;

    m_owners = pos->owners;
    m_values = pos->values;

    m_cubesToWin [Nobody] = m_nCubes;
    m_cubesToWin [One]    = m_nCubes;
    m_cubesToWin [Two]    = m_nCubes;

    for (int n = 0; n < m_nCubes; n++) {
	m_cubesToWin [m_owners [n]]--;
    }
}

void AI_Box::discard (Position * pos)
{
    delete[] pos->owners;
    delete[] pos->values;
    delete pos;
}

AI_Box::Position * AI_Box::emptyPosition (int nCubes)
{
    Position * pos = new Position;
    pos->nCubes = nCubes;
    pos->owners = new Player [nCubes];
    pos->values = new int [nCubes];
    return pos;
}

void AI_Box::clear()
{
    for (int index = 0; index < m_nCubes; index++) {
        m_owners [index] = Nobody;
        m_values [index] = 1;
    }
    m_stackPtr = -1;

    m_cubesToWin [Nobody] = 0;
    m_cubesToWin [One]    = m_nCubes;
    m_cubesToWin [Two]    = m_nCubes;

    m_undoIndex = 1;
    m_redoLimit = m_undoIndex;
}

void AI_Box::indexNeighbors()
{
    int offset = 0;
    for (int index = 0; index < m_nCubes; index++) {
        m_maxValues [index] = 0;
        for (int nb = 0; nb < 4; nb++) {
            m_neighbors [offset + nb] = -1;
        }

        int x = index / m_side;
        int y = index % m_side;
        int limit = m_side - 1;
        if (y > 0) {
            m_maxValues [index]++;	// Has a neighbor on the North side.
            m_neighbors [offset]     = index - 1;
        }
        if (y < limit) {
            m_maxValues [index]++;	// Has a neighbor on the South side.
            m_neighbors [offset + 1] = index + 1;
        }
        if (x < limit) {
            m_maxValues [index]++;	// Has a neighbor on the East side.
            m_neighbors [offset + 2] = index + m_side;
        }
        if (x > 0) {
            m_maxValues [index]++;	// Has a neighbor on the West side.
            m_neighbors [offset + 3] = index - m_side;
        }
	offset = offset + 4;
    }
}

#if AILog > 0
void AI_Box::printBox()
{
    // return;
    // IDW test. For debugging.
    for (int y = 0; y < m_side; y++) {
        fprintf (stderr, "   ");
        for (int x = 0; x < m_side; x++) {
	    int index = x * m_side + y;
	    if (m_owners[index] == Nobody) fprintf (stderr, "  .");
	    else fprintf (stderr, " %2d", (m_owners[index] == One) ?
		     m_values[index] : -m_values[index]);
        }
        fprintf (stderr, "\n");
    }
}
#endif


