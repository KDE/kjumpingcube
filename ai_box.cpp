/* ****************************************************************************
  Copyright (C) 2012 Ian Wadham <iandw.au@gmail.com>

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

#include "ai_box.h"
// #include <QStack>

#include <QDebug>
#include "stdio.h"

AI_Box::AI_Box (QObject * parent, int side)
    :
    QObject      (parent)
{
    m_parent = parent; // IDW test.
    fprintf (stderr, "\nAI_Box CONSTRUCTOR, side = %d\n", side);
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
    delete m_maxValues;
    delete m_neighbors;
    delete m_stack;
    while (! m_undoList.isEmpty()) {
	discard (m_undoList.takeLast());
    }
}

void AI_Box::resizeBox (int side)
{
    destroyBox();
    createBox (side);
}

bool AI_Box::doMove (Player player, int index, QList<int> * steps)
{
    Player oldOwner = m_owners[index];
    if ((oldOwner != player) && (oldOwner != Nobody)) {
	qDebug() << "ILLEGAL MOVE: player" << player << "old" << oldOwner <<
                                      "at" << index/m_side << index%m_side;
        return false;			// The move is not valid.
    }

    m_stackPtr = -1;
    m_owners[index] = player;		// Take ownership if not already owned.
    if (m_maxValues [index] == m_values [index]++) {	// Increase the cube.
	m_stack [++m_stackPtr] = index;	// Stack an expansion step.
	// fprintf (stderr, "Overload at %d, value %d\n", index, m_values[index]);
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
            // fprintf (stderr, "PLAYER WON\n");
            return true;;
        }
    }

    while (m_stackPtr >= 0) {
        // Pop the stack and decrease an overloaded cube.
	index = m_stack [m_stackPtr--];
	// fprintf (stderr, "  Expand at %d, value %d\n", index, m_values[index]);
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

            // Increase the neighbor and take over ownership.
            oldOwner = m_owners[indexN];
            m_owners[indexN] = player;
            if (m_maxValues [indexN] == m_values [indexN]++) {
                // Continue a cascade by pushing a new step onto the stack.
                m_stack [++m_stackPtr] = indexN;
		// fprintf (stderr, "Overload at %d, value %d\n", indexN, m_values[indexN]);
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
	    // fprintf (stderr, "  RE-Overload at %d, value %d\n", index, m_values[index]);
        }
        if (m_cubesToWin [player] <= 0) {
            // Append 0 to the move-step list and return player-won.
            if (steps) {
                steps->append (0);
            }
            // printBox();
            // fprintf (stderr, "PLAYER WON\n");
            return true;
        }
        // printBox();
    } // End while()

    // printBox();
    return false;
}

void AI_Box::copyPosition (Player player, bool isAI)
{
    qDebug() << "AI_Box::copyPosition (" << player << "," << isAI << ")";
    printBox();
    if (m_undoIndex >= m_undoList.count()) {
	qDebug() << "Call emptyPosition (" << m_nCubes << ")";
        m_undoList.append (emptyPosition (m_nCubes));
    }
    qDebug() << "m_undoIndex" << m_undoIndex << "m_undoList.count()" << m_undoList.count();
    Position * pos = m_undoList.at (m_undoIndex);
    save (pos, player, isAI);
    m_owners = pos->owners;
    m_values = pos->values;
    printBox();
    m_undoIndex++;
    m_redoLimit = m_undoIndex;
}

bool AI_Box::undoPosition (Player & player, bool & isAI)
{
    if (m_undoIndex > 1) {
	m_undoIndex--;
	Position * pos = m_undoList.at (m_undoIndex - 1);
	restore (pos);
	player = pos->player;
	isAI   = pos->isAI;
    }
    printBox();
    return (m_undoIndex > 1);
}

bool AI_Box::redoPosition (Player & player, bool & isAI)
{
    if (m_undoIndex < m_redoLimit) {
	Position * pos = m_undoList.at (m_undoIndex);
	restore (pos);
	player = pos->player;
	isAI   = pos->isAI;
	m_undoIndex++;
    }
    printBox();
    return (m_undoIndex < m_redoLimit);
}

void AI_Box::initPosition (AI_Box * box, Player player, bool isAI)
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
    delete pos->owners;
    delete pos->values;
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

void AI_Box::printBox()
{
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
/* IDW test.
    fprintf (stderr, "    %2d %2d %2d to win, pointers %lu %lu\n",
             m_cubesToWin [Nobody], m_cubesToWin [One], m_cubesToWin [Two],
	     (long) m_owners, (long) m_values);
    fprintf (stderr, "\n");
*/
}

#include "ai_box.moc"
