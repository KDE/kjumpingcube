/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 2012 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "ai_main.h"
#include "ai_kepler.h"
#include "ai_newton.h"


#include "kjumpingcube_debug.h"

#include "prefs.h"

// Use a thread and return the move via a signal.
class ThreadedAI : public QThread
{
   Q_OBJECT
public:
   ThreadedAI (AI_Main * ai)
      : m_ai (ai)
   { }

   void run() override {
      int index = m_ai->computeMove();
      Q_EMIT done (index);
   }
/* IDW test. TODO - This is not actually used. Is it needed?
 *                  I think AI_Main::stop() sets m_stopped atomically and even
 *                  if it does not, the thread will see it next time around. And
 *                  the thread is read-only with respect to m_stopped ...
 *
 *                  ATM, KCubeBoxWidget calls AI_Main::stop() not this stop()
 *                  and it works ...
 *
 * IDW test. TODO - See AI_Main::stop(). It works, but does it need a QMutex?
 *
   void stop() {
      qCDebug(KJUMPINGCUBE_LOG) << "STOP THREAD REQUESTED";
      { QMutexLocker lock (&m_ai->endMutex); m_ai->stop(); }
      wait();
      qCDebug(KJUMPINGCUBE_LOG) << "STOP THREAD DONE";
   }
*/
Q_SIGNALS:
   void done (int index);

private:
   AI_Main * m_ai;
};

const char * text[] = {"TakeOrBeTaken",  "EqualOpponent",   "CanTake",
                       "OccupyCorner",   "OccupyEdge",      "OccupyCenter",
                       "CanConsolidate", "CanReachMaximum", "CanExpand",
                       "IncreaseEdge",   "IncreaseCenter",  "PlayHereAnyway"};

void test (int array[4]) {
   for (int n = 0; n < 4; n++) {
      printf ("nb %d %d: ", n, array[n]);
   }
   printf ("\n");
}

void makeRandomSequence (int nMax, int * seq, QRandomGenerator & random)
{
   // Helper routine.  Sets up a random sequence of integers 0 to (nMax - 1).
   for (int n = 0; n < nMax; n++) {
      seq[n] = n;
   }

   int last = nMax;
   int z, temp;
   for (int n = 0; n < nMax; n++) {
      z = random.bounded(last);
      last--;
      temp = seq[z];
      seq[z] = seq[last];
      seq[last] = temp;
   }
   return;
}

AI_Main::AI_Main (QObject * parent, int side)
    : AI_Box (parent, side)
    , m_random(QRandomGenerator::global()->generate())
{
   qCDebug(KJUMPINGCUBE_LOG) << "AI_Main CONSTRUCTOR";
   m_thread = new ThreadedAI (this);
   m_randomSeq = new int [m_nCubes];
#if AILog > 0
   startStats();
#endif

   m_AI_Kepler = new AI_Kepler();
   m_AI_Newton = new AI_Newton();

   setSkill (Prefs::EnumSkill1::Beginner, true, false,
             Prefs::EnumSkill2::Beginner, true, false);

   m_stopped = false;
   m_currentLevel = 0;

   connect(m_thread, &ThreadedAI::done, this, &AI_Main::done);
}

AI_Main::~AI_Main()
{
   delete m_AI_Kepler;
   delete m_AI_Newton;
   delete m_thread;
}

void AI_Main::setSkill (int skill1, bool kepler1, bool newton1,
                        int skill2, bool kepler2, bool newton2)
{
   m_ai[0] = nullptr;
   m_ai[1] = kepler1 ? m_AI_Kepler : m_AI_Newton;
   m_ai[2] = kepler2 ? m_AI_Kepler : m_AI_Newton;

   m_ai_skill[0] = 0;
   m_ai_skill[1] = skill1;
   m_ai_skill[2] = skill2;

   m_ai_maxLevel[0] = 0;
   m_ai_maxLevel[1] = depths[skill1];
   m_ai_maxLevel[2] = depths[skill2];

   for (int player = 1; player <= 2; player++) {
       qCDebug(KJUMPINGCUBE_LOG) << "AI_Main::setSkill: Player" << player << m_ai[player]->whoami()
                << "skill" << m_ai_skill[player]
                << "maxLevel" << m_ai_maxLevel[player];
   }
}

void AI_Main::stop()
{
   m_stopped = true;
   m_thread->wait();
}

void AI_Main::getMove (const Player player, const AI_Box * box)
{
#if AILog > 1
   qCDebug(KJUMPINGCUBE_LOG) << "\nEntering AI_Main::getMove() for player" << player;
#endif
   // These settings are immutable once the thread starts and getMove() returns.
   // If AI_Main::setSkill() is called when the thread is active, the new values
   // will not take effect until the next move or hint.
   m_currentAI = m_ai[player];
   m_skill     = m_ai_skill[player];
   m_maxLevel  = m_ai_maxLevel[player];

#if AILog > 0
   initStats (player);	// IDW test. Statistics collection.
#endif
#if AILog > 1
   qCDebug(KJUMPINGCUBE_LOG) << tag(0) << "PLAYER" << player << m_currentAI->whoami() << "skill" << m_skill << "max level" << m_maxLevel;
#endif

   m_stopped = false;
   m_player  = player;

   checkWorkspace (box->side());
   initPosition (box, player, true);

#if AILog > 1
   qCDebug(KJUMPINGCUBE_LOG) << "INITIAL POSITION";
   printBox();
#endif

   m_thread->start (QThread::IdlePriority);	// Run computeMove() on thread.
   return;
}

int AI_Main::computeMove()
{
   // IDW test. // Set up a random sequence of integers 0 to (m_nCubes - 1).
   // IDW test. makeRandomSequence (m_side * m_side, m_randomSeq, m_random);

   // Start the recursive MiniMax algorithm on a copy of the current cube box.
#if AILog > 2
   qCDebug(KJUMPINGCUBE_LOG) << tag(0) << "Calling tryMoves(), level zero, player" << m_player;
#endif

   Move move = tryMoves (m_player, 0);

#if AILog > 2
   qCDebug(KJUMPINGCUBE_LOG) << tag(0) << "Returned from tryMoves(), level zero, player"
            << m_player << "value" << move.val
            << "simulate" << n_simulate << "assess" << n_assess;
#endif

#if AILog > 0
   saveStats (move);		// IDW test. Statistics collection.
#endif

#if AILog > 1
   qCDebug(KJUMPINGCUBE_LOG) << "==============================================================";
   qCDebug(KJUMPINGCUBE_LOG) << tag(0) << "MOVE" << m_currentMoveNo << "for PLAYER" << m_player
            << "X" << move.index/m_side << "Y" << move.index%m_side;
#endif

   return (move.index);		// Return the best move found, via a signal.
}

Move AI_Main::tryMoves (Player player, int level)
{
   m_currentLevel = level; // IDW test. To limit qCDebug(KJUMPINGCUBE_LOG) in findCubesToMove().
   long maxValue = -WinnerPlus1;

   Move bestMove = {-1, -WinnerPlus1};

   // Find likely cubes to move.
   Move * cubesToMove = new Move [m_nCubes];
#if AILog > 3
   qCDebug(KJUMPINGCUBE_LOG) << "FIND CUBES TO MOVE for Player" << player
            << "from POSITION AT LEVEL" << level;
   if (level > 0) boxPrint (m_side, (int *) m_owners, m_values); // IDW test.
#endif
   int moves = findCubesToMove (cubesToMove, player,
                       m_owners, m_values, m_maxValues);

#if AILog > 2
   qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "Level" << level << "Player" << player
            << "number of likely moves" << moves;
   if (level == 0) {
      for (int n = 0; n < moves; n++) {
         int v = cubesToMove[n].val;
         QString s = "";
         if ((v > 0) && (v <= 12)) s = QString(text[v-1]);
         qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "    " << "X" << cubesToMove[n].index/m_side
                                << "Y" << cubesToMove[n].index%m_side
                                << "val" << cubesToMove[n].val << s;
      }
      saveLikelyMoves (moves, cubesToMove); // IDW test.
   }

   m_currentMove->searchStats->at(level)->n_moves += moves;
#endif

   // IDW TODO - Sort the moves by priority in findCubesToMove() (1 first),
   //    shuffle moves that have the same value (to avoid repetitious openings).
   // IDW TODO - Apply alpha-beta pruning to the sorted moves.  Maybe we can
   //    allow low-priority moves and sacrifices ...

   for (int n = 0; n < moves; n++) {
#if AILog > 2
      if (level == 0) qCDebug(KJUMPINGCUBE_LOG)
            << "==============================================================";
      qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "TRY" << (n+1) << "at level" << level
               << "Player" << player
               << "X"<< cubesToMove[n].index/m_side
               << "Y" << cubesToMove[n].index%m_side
               << "val" << cubesToMove[n].val;
#endif

      MoveUndodata  undodata;
      bool won = doMove (player, cubesToMove[n].index, &undodata);

#if AILog > 2
      n_simulate++;
#endif

      long val;
      if (won) {
         // Accept a winning move.
         bestMove = cubesToMove[n];
         bestMove.val = WinnerPlus1 - 1;
#if AILog > 2
         n_assess++;
#endif
         cubesToMove[n].val = bestMove.val; // IDW test. For debug output.
#if AILog > 2
         qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "Player" << player
                  << "wins at level" << level
                  << "move" << cubesToMove[n].index/m_side
                  << cubesToMove[n].index%m_side;
#endif
         undoMove(&undodata);
         break;
      }
      else if (level >= m_maxLevel) {
	 // Stop the recursion.
         val = m_currentAI->assessPosition (player, m_nCubes,
                                            m_owners, m_values);
#if AILog > 2
	 n_assess++;
#endif
         cubesToMove[n].val = val; // IDW test. For debug output.
#if AILog > 3
         qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "END RECURSION: Player" << player
                  << "X" << cubesToMove[n].index/m_side
                  << "Y" << cubesToMove[n].index%m_side
                  << "assessment" << val << "on POSITION";
         boxPrint (m_side, (int *)(m_owners), m_values);// IDW test.
#endif
      }
      else {
         // Switch players.
         Player opponent = (player == One) ?  Two : One;

         // Do the MiniMax calculation for the next recursion level.
/*         qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "CALL tryMoves: Player" << opponent
                                << "level" << level+1; */
         Move move = tryMoves (opponent, level + 1);
         val = move.val;
         cubesToMove[n].val = val; // IDW test. For debug output.
/*         qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "RETURN to level" << level
                  << "Player" << player << "X" << move.index/m_side
                  << "Y" << move.index%m_side << "assessment" << val; */
      }

      if (val > maxValue) {
	 maxValue = val;
	 bestMove = cubesToMove[n];
	 bestMove.val = val;
         cubesToMove[n].val = val; // IDW test. For debug output.
#if AILog > 2
	 qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "NEW MAXIMUM at level" << level
                  << "Player" << player << "X" << bestMove.index/m_side
                  << "Y" << bestMove.index%m_side << "assessment" << val;
#endif
      }
      Player p = player;
      undoMove(&undodata);
      if (p != player) qCDebug(KJUMPINGCUBE_LOG) << "ERROR: PLAYER CHANGED: from" << p <<
                                                            "to" << player;
      if (m_stopped) {
	 qCDebug(KJUMPINGCUBE_LOG) << "STOPPED AT LEVEL" << level;
         break;
      }
   }

#if AILog > 2
   if (level == 0) {
      qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "VALUES OF MOVES - Player" << player
            << "number of moves" << moves;
      for (int n = 0; n < moves; n++) {
         qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "    " << "X" << cubesToMove[n].index/m_side
                            << "Y" << cubesToMove[n].index%m_side
                            << "val" << cubesToMove[n].val;
      }
   }
#endif

   delete [] cubesToMove;

#if AILog > 2
   qCDebug(KJUMPINGCUBE_LOG);
   qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "BEST MOVE at level" << level << "Player" << player
            << "X" << bestMove.index/m_side << "Y" << bestMove.index%m_side
            << "assessment" << bestMove.val;
#endif

   // Apply the MiniMax rule.
   if (level > 0) {
#if AILog > 2
      qCDebug(KJUMPINGCUBE_LOG) << tag(level) << "CHANGE SIGN" << bestMove.val
                                              << "to" << -bestMove.val;
#endif
      bestMove.val = - bestMove.val;
   }

   return bestMove;
}

int AI_Main::findCubesToMove (Move * c2m, const Player player,
                              const Player * owners, const int * values,
                              const int * maxValues)
{
   int index, n;
   int opponent  = (player == One) ? Two : One;
   int moves     = 0;
   int min       = VeryHighValue;
   bool beginner = (m_skill == Prefs::EnumSkill1::Beginner);
   int secondMin = min;

   // Set up a random sequence of integers 0 to (m_nCubes - 1).
   makeRandomSequence (m_nCubes, m_randomSeq, m_random);

   // Put values on the cubes.
   int * neighbors = m_neighbors;
   int val;
   for (n = 0; n < m_nCubes; n++) {
      index = m_randomSeq [n];

      // Use only cubes that do not belong to the opponent.
      if (owners[index] == opponent) {
         continue;
      }

      // The beginner selects the cubes with the most pips on them:
      // other players check the neighbours of each cube.
      val = beginner ? (5 - values [index]) :
                       m_currentAI->assessCube (index, player,
                                                (neighbors + 4 * index),
                                                owners, values, maxValues);
      if (val < min) {
         secondMin = min;
         min = val;
      }
      else if ((val > min) && (val < secondMin)) {
         secondMin = val;
      }

      // Store the move.
      c2m[moves].index = index;
      c2m[moves].val = val;
      moves++;
   }
#if AILog > 2
   if (m_currentLevel == 0) qCDebug(KJUMPINGCUBE_LOG) << "\nMinimum is" << min
       << ", second minimum is" << secondMin << "available moves" << moves;
#endif

   if (moves == 0) {
      // Should not happen? Even bad moves are given a value > 0.
      qCDebug(KJUMPINGCUBE_LOG) << "NO LIKELY MOVES AVAILABLE: selecting" << moves
               << "min" << min;
      return moves;
   }

   int counter = 0;
   // Find all moves with minimum assessment
   for (n = 0; n < moves; ++n) {
       if (c2m[n].val == min) {
          c2m[counter].index = c2m[n].index;
          c2m[counter].val = c2m[n].val;
          counter++;
       }
   }

   // IDW TODO - Finalise the logic for limiting the number of moves tried.
   //            The 1/3 gizmo is to limit searches on an empty cube box.
   //            Should not use secondMin on Expert level?
   //            Should not use secondMin on deeper recursion levels?
   //            Should it all depend on how many moves are at "min"?
   //            Should AI_Newton have overlapping values of move types?

   // if (true || m_skill == Prefs::EnumSkill1::Average) // IDW test.
   // if ((counter <= 2) || (m_skill == Prefs::EnumSkill1::Average))
   // if ((m_skill == Prefs::EnumSkill1::Average) &&
   if (m_currentMoveNo > (m_nCubes / 3)) {	// If board > 1/3 full.
      for (n = 0; n < moves; ++n) {
         if (c2m[n].val == secondMin) {
            c2m[counter].index = c2m[n].index;
            c2m[counter].val = c2m[n].val;
            counter++;
         }
      }
   }

   if (counter != 0) {
      moves = counter;
   }

   // IDW TODO - Can we find a more subtle rule?

   // If more than maxMoves moves are favorable, take maxMoves random
   // moves because it will take too long to check more.
   return qMin (moves, maxBreadth);
}

void AI_Main::checkWorkspace (int side)
{
   if (m_side != side) {
       qCDebug(KJUMPINGCUBE_LOG) << "NEW AI_Box SIZE NEEDED: was" << m_side << "now" << side;
       delete[] m_randomSeq;
       resizeBox (side);
       m_randomSeq = new int [side * side];
   }
}

#if AILog > 0
/*
 * Debugging methods for the AI.
 */
void AI_Main::boxPrint (int side, int * owners, int * values)
{
   // Print out a position reached during or after recursion.
   // fprintf (stderr, "AI_Main::boxPrint (%d, %lu, %lu)\n",
            // side, (long) owners, (long) values); // Tests push and pop logic.
   for (int y = 0; y < side; y++) {
      fprintf (stderr, "   ");
      for (int x = 0; x < side; x++) {
	 int index = x * side + y;
	 if (owners[index] == Nobody) fprintf (stderr, "  .");
	 else fprintf (stderr, " %2d", (owners[index] == One) ?
		 values[index] : -values[index]);
      }
      fprintf (stderr, "\n");
   }
}

void AI_Main::startStats()
{
   // IDW test. For debugging.
   m_currentMoveNo = 0;
   m_moveStats.clear();
}

void AI_Main::postMove (Player player, int index, int side)
{
   // IDW test. Statistics collection.
   // Used to record a move by a human player.
   checkWorkspace (side);

   int x = index / m_side;
   int y = index % m_side;
#if AILog > 1
   qCDebug(KJUMPINGCUBE_LOG) << "AI_Main::postMove(): index" << index << "at" << x << y << "m_side" << m_side;
#endif
   m_maxLevel    = m_ai_maxLevel[player];
   m_currentMove = new MoveStats [1];

   m_currentMoveNo++;
   m_currentMove->player     = player;
   m_currentMove->moveNo     = m_currentMoveNo;
   m_currentMove->n_simulate = 0;
   m_currentMove->n_assess   = 0;
   m_currentMove->nLikelyMoves = 0;
   m_currentMove->likelyMoves = 0;
   m_currentMove->searchStats = new QList<SearchStats *>();

   m_currentMove->x     = x;
   m_currentMove->y     = y;
   m_currentMove->value = 0;
   m_moveStats.append (m_currentMove);
#if AILog > 1
   qCDebug(KJUMPINGCUBE_LOG) << "==============================================================";
   qCDebug(KJUMPINGCUBE_LOG) << tag(0) << "MOVE" << m_currentMoveNo << "for PLAYER" << player
            << "X" << x << "Y" << y;
   qCDebug(KJUMPINGCUBE_LOG) << "==============================================================";
#endif
}

void AI_Main::initStats (int player)
{
   // IDW test. For debugging.
   m_currentMove = new MoveStats [1];

   m_currentMoveNo++;
   m_currentMove->player     = (Player) player;
   m_currentMove->moveNo     = m_currentMoveNo;
   m_currentMove->n_simulate = 0;
   m_currentMove->n_assess   = 0;
   m_currentMove->nLikelyMoves = 0;
   m_currentMove->likelyMoves = 0;
   m_currentMove->searchStats = new QList<SearchStats *>();

   for (int n = 0; n <= m_maxLevel; n++) {
      SearchStats * s = new SearchStats [1];
      s->n_moves = 0;
      m_currentMove->searchStats->append (s);
   }

   n_simulate = 0;
   n_assess = 0;
}

void AI_Main::saveLikelyMoves (int nMoves, Move * moves)
{
   Move * m = new Move [nMoves];
   m_currentMove->nLikelyMoves = nMoves;
   m_currentMove->likelyMoves = m;
   for (int n = 0; n < nMoves; n++) {
      m [n] = moves [n];
   }
}

void AI_Main::saveStats (Move & move)
{
   // IDW test. For debugging.
   m_currentMove->x = move.index/m_side;
   m_currentMove->y = move.index%m_side;
   m_currentMove->value = move.val;
   m_currentMove->n_simulate = n_simulate;
   m_currentMove->n_assess   = n_assess;
   m_moveStats.append (m_currentMove);
}

void AI_Main::dumpStats()
{
   // IDW test. For debugging. Replay all the moves, with statistics for each.
   qCDebug(KJUMPINGCUBE_LOG) << m_moveStats.count() << "MOVES IN THIS GAME";
   AI_Box * statsBox = new AI_Box (0, m_side);
   statsBox->printBox();
   for (MoveStats * m : qAsConst(m_moveStats)) {
      QList<int> l;
      int nMax = m->searchStats->count();
      for (int n = 0; n < nMax; n++) {
         l.append (m->searchStats->at(n)->n_moves);
      }
      qCDebug(KJUMPINGCUBE_LOG) << ((m->player == 1) ? "p1" : "p2") << "move" << m->moveNo
               << "X" << m->x << "Y" << m->y
               << "value" << m->value << m->n_simulate << m->n_assess << l;

      if (m->nLikelyMoves > 0) {
         qCDebug(KJUMPINGCUBE_LOG) << "     Number of likely moves" << m->nLikelyMoves;
         for (int n = 0; n < m->nLikelyMoves; n++) {
            int v = m->likelyMoves[n].val;
	    QString s = "";
	    if ((v > 0) && (v <= 12)) s = QString(text[v-1]);
            qCDebug(KJUMPINGCUBE_LOG) << "    " << "X" << m->likelyMoves[n].index/m_side
                               << "Y" << m->likelyMoves[n].index%m_side
                               << "val" << m->likelyMoves[n].val << s;
         }
         delete m->likelyMoves;
      }
      bool won = statsBox->doMove (m->player, m->x * m_side + m->y);
      statsBox->printBox();
      qDeleteAll (*(m->searchStats));
      delete[] m;
   }
   m_moveStats.clear();
   delete statsBox;
}

QString AI_Main::tag (int level)
{
    QString indent ("");
    indent.fill ('-', 2 * level);
    indent = indent.prepend (QString::number (level));
    indent = indent.leftJustified (2 * m_maxLevel + 1);
    QString mv = QString::number(m_currentMoveNo).rightJustified(3, '0');
    return (QString ("%1 %2 %3")).arg(m_currentMove->player).arg(mv).arg(indent);
}
#endif

/**
 * This is the default definition of virtual long AI_Base::assessPosition().
 * Inheritors of AI_Base can declare and define other versions.
 */
long AI_Base::assessPosition (const Player player,   const int nCubes,
                              const Player owners[], const int values[]) const
{
   int    cubesOne       = 0;
   int    cubesTwo       = 0;
   int    pointsOne      = 0;
   int    pointsTwo      = 0;
   Player otherPlayer = (player == One) ? Two : One;
   int index, points;

   for (index = 0; index < nCubes; index++) {
      points  = values[index];
      if (owners[index] == One) {
         cubesOne++;
         pointsOne += points * points;
      }
      else if (owners[index] == Two) {
         cubesTwo++;
	 pointsTwo += points * points;
      }
   }

   if (player == One) {
      return cubesOne * cubesOne + pointsOne - cubesTwo * cubesTwo - pointsTwo;
   }
   else {
      return cubesTwo * cubesTwo + pointsTwo - cubesOne * cubesOne - pointsOne;
   }
}

#include "ai_main.moc"
#include "moc_ai_main.cpp"
