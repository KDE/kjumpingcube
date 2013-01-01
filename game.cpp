/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 1998-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
  Copyright (C) 2012-2013 by Ian Wadham      <iandw.au@gmail.com>

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

#include "game.h"
#include "version.h"
#include "ai_main.h"
#include "ai_box.h"
#include "kcubeboxwidget.h"

#include <KLocalizedString>
#include <KMessageBox>
#include <KFileDialog>
#include <KTemporaryFile>
#include <kio/netaccess.h>
#include <QTimer>

#include "prefs.h"

Game::Game (const int d, KCubeBoxWidget * view)
   :
   QObject ((QObject *) view),	// Auto-delete Game when view is deleted.
   m_state         (NotReady),
   m_activity      (Idle),
   m_waitingState  (Nil),
   m_view          (view),
   m_side          (d),
   m_currentPlayer (One),
   m_fullSpeed     (false),
   m_ignoreComputerMove    (false)
{
   qDebug() << "CONSTRUCT Game: side" << m_side;
   m_box            = new AI_Box  (this, m_side);
   m_ai             = new AI_Main (this, m_side);
   m_steps          = new QList<int>;
   computerPlOne    = false;
   computerPlTwo    = false;

   loadSettings();

   connect (m_ai,   SIGNAL(done(int)), SLOT(computerMoveDone(int)));
   connect (m_view, SIGNAL(animationDone(int)), SLOT(animationDone(int)));
   connect (m_view, SIGNAL(mouseClick(int,int)), SLOT(mouseClick(int,int)));

   emit playerChanged (One);

   m_ai->startStats();
   m_gameHasBeenWon = false;
   m_waitingForStep = false;
   m_playerWaiting  = computerPlOne ? One : Nobody;
   m_state          = computerPlOne ? Waiting : HumanMoving;
   m_waitingState   = computerPlOne ? ComputerToMove : Nil;
}

Game::~Game()
{
   if (isActive()) {
      stopActivities();
   }
   delete m_steps;
}

void Game::gameActions (const int action)
{
   // IDW TODO - Do a mode-switch test?

   qDebug() << "GAME ACTION IS" << action;
   switch (action) {
   case NEW:
      newGame();
      break;
   case HINT:
      if (m_state == HumanMoving) {
         computeMove (Hinting);
      }
      break;
   case BUTTON:
      buttonClick();
      break;
   case UNDO:
      undo();
      break;
   case REDO:
      redo();
      break;
   case SAVE:
   case SAVE_AS:
      saveGame (action == SAVE_AS);
      break;
   case LOAD:
      loadGame();
      break;
   default:
      break;
   }
}

void Game::loadSettings()
{
  m_fullSpeed = Prefs::animationNone();		// Animate cascade moves?

  bool reColorCubes  = m_view->loadSettings();
  bool reSizeCubes   = (m_side != Prefs::cubeDim());

  m_pauseForStep     = Prefs::pauseForStep();
  m_pauseForComputer = Prefs::pauseForComputer();
  qDebug() << "m_pauseForComputer" << m_pauseForComputer
           << "m_pauseForStep" << m_pauseForStep; // IDW test.

  /*
   * IDW TODO. Co-ordinate this with KCubeBoxWidget::loadSettings().
  if (! m_pauseForComputer) {
      hidePopup();
  }
  */
  setDim (Prefs::cubeDim());

  if (reColorCubes) {
     emit playerChanged (m_currentPlayer);	// Change color in status bar.
  }

  m_ai->setSkill (Prefs::skill1(), Prefs::kepler1(), Prefs::newton1(),
                  Prefs::skill2(), Prefs::kepler2(), Prefs::newton2());
  qDebug() << "PLAYER 1 settings: skill" << Prefs::skill1() << "Kepler" << Prefs::kepler1() << "Newton" << Prefs::newton1();
  qDebug() << "PLAYER 2 settings: skill" << Prefs::skill2() << "Kepler" << Prefs::kepler2() << "Newton" << Prefs::newton2();

  setComputerplayer (One, Prefs::computerPlayer1());
  setComputerplayer (Two, Prefs::computerPlayer2());

  // IDW TODO - There are just too many actions flowing from settings changes.
  //            Simplify them or work out proper inter-dependencies.
  // IDW TODO - This code is not needed if we leave things so that the popup is
  //            still showing and the first click triggers all remaining steps.
  // if ((! m_pauseForStep) && oldPauseForStep) {
     // IDW TODO - This is the same as part 1 of checkClick().
     // if (animationTimer->isActive() || m_waitingForStep) {
        // m_waitingForStep = false;
        // animationTimer->start();
     // }
  // }
  if (reSizeCubes) {
     newGame();					// Box changed: start new game.
     return;
  }
  else if (! m_box->isClear()) {		// If not first move...
  // IDW TODO - Fix test for empty box. if (m_cubesToWin [Cube::Nobody] > 0) {
     // Continue current game, maybe with change of computer player settings.
     checkComputerplayer (m_currentPlayer);
     return;
  }
  // There is no more to do: we are waiting for a click to start the game.
}

void Game::newGame()
{
   shutdown();				// Stop the current move (if any).
   reset();				// Clear the cubebox.
   emit setAction (UNDO, false);
   emit setAction (REDO, false);
   // IDW TODO - Use a signal. statusBar()->showMessage (i18n("New Game"));
}

void Game::saveGame (bool saveAs)
{
   // IDW TODO. if (m_game->waitToSave (saveAs)) return;

   if (saveAs || m_gameURL.isEmpty()) {
      int result=0;
      KUrl url;

      do {
         url = KFileDialog::getSaveUrl (m_gameURL.url(), "*.kjc", m_view, 0);

         if (url.isEmpty())
            return;

         // check filename
         QRegExp pattern ("*.kjc", Qt::CaseSensitive, QRegExp::Wildcard);
         if (! pattern.exactMatch (url.fileName())) {
            url.setFileName (url.fileName()+".kjc");
         }

         if (KIO::NetAccess::exists (url, KIO::NetAccess::DestinationSide,
                                     m_view)) {
            QString mes=i18n("The file %1 exists.\n"
               "Do you want to overwrite it?", url.url());
            result = KMessageBox::warningContinueCancel
               (m_view, mes, QString(), KGuiItem(i18n("Overwrite")));
            if (result == KMessageBox::Cancel)
               return;
         }
      } while (result == KMessageBox::No);

      m_gameURL = url;
   }

   KTemporaryFile tempFile;
   tempFile.open();
   KConfig config (tempFile.fileName(), KConfig::SimpleConfig);
   KConfigGroup main (&config, "KJumpingCube");
   main.writeEntry ("Version", KJC_VERSION);
   KConfigGroup game (&config, "Game");
   saveProperties (game);
   config.sync();

   if (KIO::NetAccess::upload (tempFile.fileName(), m_gameURL, m_view)) {
      QString s = i18n("Game saved as %1", m_gameURL.url());
      // IDW TODO - Use a signal. statusBar()->showMessage (s);
   }
   else {
      KMessageBox::sorry (m_view, i18n("There was an error in saving file\n%1",
                                       m_gameURL.url()));
   }
}

void Game::loadGame()
{
   // IDW TODO. if (m_game->waitToLoad()) return;

   bool fileOk=true;
   KUrl url;

   do {
      url = KFileDialog::getOpenUrl (m_gameURL.url(), "*.kjc", m_view, 0);
      if (url.isEmpty())
         return;
      if (! KIO::NetAccess::exists(url, KIO::NetAccess::SourceSide, m_view)) {
         QString mes = i18n("The file %1 does not exist!", url.url());
         KMessageBox::sorry (m_view, mes);
         fileOk = false;
      }
   } while (! fileOk);

   QString tempFile;
   if (KIO::NetAccess::download (url, tempFile, m_view)) {
      KConfig config( tempFile, KConfig::SimpleConfig);
      KConfigGroup main (&config, "KJumpingCube");
      if (! main.hasKey ("Version")) {
         QString mes = i18n("The file %1 is not a KJumpingCube gamefile!",
                            url.url());
         KMessageBox::sorry (m_view, mes);
         return;
      }

      m_gameURL = url;
      KConfigGroup game (&config, "Game");
      readProperties (game);

      emit setAction (UNDO, false);

      KIO::NetAccess::removeTempFile (tempFile);
   }
   else
      KMessageBox::sorry (m_view, i18n("There was an error loading file\n%1",
                                       url.url()));
}

void Game::undo()
{
   // IDW TODO. if (m_game->waitToUndo()) return;

   // IDW test. if(m_game->isActive())
      // IDW test. return;
   int moreToUndo = undoRedo (-1);
   if (moreToUndo >= 0) {
      emit setAction (UNDO, (moreToUndo == 1));
   }
   emit setAction (REDO, true);
}

void Game::redo()
{
   // IDW test. if(m_game->isActive())
      // IDW test. return;
   int moreToRedo = undoRedo (+1);
   if (moreToRedo >= 0) {
      emit setAction (REDO, (moreToRedo == 1));
   }
   emit setAction (UNDO, true);
}

void Game::reset()
{
   // IDW TODO - Delete? stopActivities(); shutdown() here only (except Quit)?

   m_view->reset();
   m_box->clear();

   m_fullSpeed = Prefs::animationNone();	// Animate cascade moves?

   m_currentPlayer = One;

   m_waitingForStep = false;
   m_playerWaiting  = computerPlOne ? One : Nobody;
   m_state          = computerPlOne ? Waiting : HumanMoving;
   m_waitingState   = computerPlOne ? ComputerToMove : Nil;
   if (computerPlOne) {
      emit buttonChange (true, false, i18n("Start computer move"));
   }
   else {
      emit buttonChange (false);
   }

   emit playerChanged (One);
   // checkComputerplayer(One); // Enable this line for IDW high-speed test.

   m_ai->startStats();
}

int Game::undoRedo (int actionType)
{
   // IDW TODO - A user-friendly way to defer undo/redo if computer is moving.
   // IDW test. if (isActive())
      // IDW test. return -1;
   if (isActive()) {
      stopActivities();
      m_ignoreComputerMove = true;
   }

   Player oldPlayer = m_currentPlayer;

   bool isAI = false;
   bool moreToDo = (actionType < 0) ?
      m_box->undoPosition (m_currentPlayer, isAI) :
      m_box->redoPosition (m_currentPlayer, isAI);

   // Update the cube display after an undo or redo.
   // IDW DELETE. m_view->displayPosition (m_box);
   for (int n = 0; n < (m_side * m_side); n++) {
      m_view->displayCube (n, m_box->owner (n), m_box->value (n));
   }

   if (oldPlayer != m_currentPlayer)
      emit playerChanged (m_currentPlayer);

   if ((actionType > 0) && (! moreToDo)) {	// If end of Redo's: restart AI.
      checkComputerplayer (m_currentPlayer);
   }
   return (moreToDo ? 1 : 0);
}

void Game::checkComputerplayer(Player player)
{
   // Check if a process is running or the widget is not shown yet
   if (isActive() || (! m_view->isVisible()))
      return;
   if ((player == One && computerPlOne && m_currentPlayer == One) ||
       (player == Two && computerPlTwo && m_currentPlayer == Two)) {
      if (m_pauseForComputer) {
         if (m_playerWaiting == Nobody) {
            m_playerWaiting = player;
	    // IDW DELETE. showPopup (i18n ("Click anywhere to continue"));
            m_state = Waiting;
            m_waitingState = ComputerToMove;
            if (computerPlOne && computerPlTwo) {
                emit buttonChange (true, false, i18n("Continue game"));
            }
            else {
                emit buttonChange (true, false, i18n("Start computer move"));
            }
            return;
         }
	 else {
            // IDW DELETE. hidePopup();
         }
      }
      m_playerWaiting = Nobody;
      KCubeWidget::enableClicks (false);

      qDebug() << "Calling m_ai->getMove() for player" << player;
      t.start(); // IDW test.
      computeMove (ComputerMoving);
   }
   else {
      m_state = HumanMoving;
   }
}

void Game::computeMove (State moveType)
{
   m_state = moveType;
   m_view->setWaitCursor();
   emit buttonChange (true, true, i18n("Stop computing"));	// Red look.
   m_activity = Computing;
   m_ignoreComputerMove = false;
   m_ai->getMove (m_currentPlayer, m_box);
}

void Game::computerMoveDone (int index)
{
   // We do not care if we interrupted the computer.  It was probably
   // taking too long, so we will just take the best move it had so far.

   m_activity = Idle;
   if (m_ignoreComputerMove) {
      // The main thread will be starting a new game or closing KJumpingCube.
      m_ignoreComputerMove = false;
      return;
   }
   if ((index < 0) || (index >= (m_side * m_side))) {
      m_view->setNormalCursor();
      emit buttonChange (false);				// Inactive.
      KMessageBox::sorry (m_view,
                          i18n ("The computer could not find a valid move."));
      // IDW TODO - What to do about state values ???
      return;
   }

   // IDW TODO - Check states.  ComputingMove, Busy, Waiting.
   // IDW TODO - Set appropriate button states and captions.

   // Blink the cube to be moved (twice).
   switch (m_state) {
   case ComputerMoving:
      qDebug() << "TIME of MOVE" << t.elapsed();
      qDebug() << "===========================================================";
      m_view->startAnimation (false, index);
      break;
   case Hinting:
      qDebug() << "HINT FOR PLAYER" << m_currentPlayer
               << "X" << index / m_side << "Y" << index % m_side;
      m_view->startAnimation (false, index);
      break;
   case Waiting:
      qDebug() << "TIME of MOVE" << t.elapsed();
      qDebug() << "===========================================================";
      m_view->startAnimation (false, index);
      break;
   default:
      return;
      break;
   }

   m_activity = ShowingMove;
   emit buttonChange (true, true, i18n("Stop moving"));
}

void Game::setDim (int d)
{
   if (d != m_side) {
      shutdown();
      delete m_box;
      m_box   = new AI_Box (this, d);
      qDebug() << "AI_Box CONSTRUCTED by Game::setDim()";
      m_side  = d;
      m_view->setDim (d);
      // IDW TODO - Crashed in slot computerMoveDone(int). Starts animation with
      //            too large an index? Going from larger to smaller box.
      // Should we do setDim and return and do the rest of loadSettings later?
      reset();
   }
}

void Game::setComputerplayer(Player player,bool flag)
{
   if(player==One)
      computerPlOne=flag;
   else if(player==Two)
      computerPlTwo=flag;
   // IDW TODO - If current player is not computer, m_playerWaiting = Nobody.
   if ((player == m_currentPlayer) && (! flag)) {
      m_playerWaiting = Nobody;	// Current player is human.
   }
}

void Game::shutdown()
{
   // Shut down gracefully, avoiding a possible crash when the user hits Quit.
   m_view->killAnimation();	// Stop animation immediately (if active).
   if (m_activity == Computing) {
      m_ai->stop();		// Stop AI ASAP (computerMoveDone() sets Idle).
      m_ignoreComputerMove = true;
   }
   else {
      m_activity = Idle;	// In case it was ShowingMove or AnimatingMove.
   }
}

void Game::stopActivities()
{
   // IDW TODO - REWRITE THIS .................................................
   qDebug() << "STOP ACTIVITIES";
   // IDW DELETE. if (animationTimer->isActive() || m_waitingForStep) {
   if ((m_activity == AnimatingMove) || m_waitingForStep) {
      m_waitingForStep = false;
      // stopAnimation (true);	// If moving, do the rest of the steps ASAP.
   }
   if (m_activity == Computing) {
      qDebug() << "BRAIN IS ACTIVE";
      m_ai->stop();		// Keep Stop enabled, for the blink animation.
   }
}

void Game::saveProperties (KConfigGroup & config)
{
   // IDW TODO - Save settings for computer player(s), animation, etc.
   //            Is Undo right for interrupted animation????
   //            Do we need Undo for interrupted computer move?
   //            What happens to the signal when the computer move ends?
   if (isMoving()) {
      stopActivities();
      undoRedo (-1);		// Undo incomplete move.
   }
   else if (m_ai->isActive()) {
      stopActivities();
   }

   // save current player
   config.writeEntry ("onTurn", (int) m_currentPlayer);

   QStringList list;
   QString owner, value, key;

   for (int x = 0; x < m_side; x++) {
     for (int y = 0; y < m_side; y++) {
	key.sprintf ("%u,%u", x, y);
	int index = x * m_side + y;
	owner.sprintf ("%u", m_box->owner (index));
	value.sprintf ("%u", m_box->value (index));
	list.append (owner.toAscii());
	list.append (value.toAscii());
	config.writeEntry (key, list);

	list.clear();
     }
   }
   config.writeEntry ("CubeDim", m_side);
}

void Game::readProperties (const KConfigGroup& config)
{
  // IDW TODO - Restore settings for computer player(s), animation, etc.
  // IDW TODO - If a computer player is "on turn", wait for a click or use
  //            a KMessageBox ...
  qDebug() << "Entering Game::readProperties ...";
  QStringList list;
  QString     key;
  int         owner, value, maxValue;

  // Dimension must be 3 to 15 (see definition in ai_box.h).
  int cubeDim = config.readEntry ("CubeDim", minSide);
  if ((cubeDim < minSide) || (cubeDim > maxSide)) {
     KMessageBox::sorry (m_view, i18n("The file's cube box size is outside "
                                    "the range %1 to %2. It will be set to %1.")
                                    .arg(minSide).arg(maxSide));
     cubeDim = 3;
  }
  m_side = 1;					// Create a new cube box.
  setDim (cubeDim);

  for (int x = 0; x < m_side; x++) {
    for (int y = 0; y < m_side; y++) {
	key.sprintf ("%u,%u", x, y);
	list = config.readEntry (key, QStringList());
	// List length must be 2, owner must be 0-2, value >= 1 and <= max().
	if (list.count() < 2) {
	    KMessageBox::sorry (m_view, i18n("Missing input line for cube %1.")
		    .arg(key));
	    owner = 0;
	    value = 1;
	}
	else {
	    owner = list.at(0).toInt();
	    value = list.at(1).toInt();
	}
	if ((owner < 0) || (owner > 2)) {
	    KMessageBox::sorry (m_view, i18n("Owner of cube %1 is outside the "
                                           "range 0 to 2.").arg(key));
	    owner = 0;
	}
	int index = x * m_side + y;
	maxValue = (owner == 0) ? 1 : m_box->maxValue (index);
	if ((value < 1) || (value > maxValue)) {
	    KMessageBox::sorry (m_view, i18n("Value of cube %1 is outside the "
                                           "range 1 to %2.")
                                           .arg(key).arg(maxValue));
	    value = maxValue;
	}
	m_view->displayCube (index, (Player) owner, value);
	m_box->setOwner (index, (Player) owner);
	m_box->setValue (index, value);

	list.clear();
    }
  }

   // Set current player - must be 1 or 2.
   int onTurn = config.readEntry ("onTurn", 1);
   if ((onTurn < 1) || (onTurn > 2)) {
       KMessageBox::sorry (m_view, i18n("Current player is neither 1 nor 2."));
       onTurn = 1;
   }
   m_currentPlayer = (Player) onTurn;
   emit playerChanged (m_currentPlayer);
   qDebug() << "Leaving Game::readProperties ...";
   return;
   checkComputerplayer (m_currentPlayer);	// IDW TODO - THIS CRASHES.
   qDebug() << "Leaving Game::readProperties ...";
}

/* ***************************************************************** **
**                               slots                               **
** ***************************************************************** */

void Game::mouseClick (int x, int y)
{
   // IDW TODO - Write a new mouse-click event for KCubeBoxWidget? Remove the
   //            one that KCubeWidget has?
   int  index = x * m_side + y;
   bool humanPlayer = (! isComputer (m_currentPlayer));
   qDebug() << "CLICK" << x << y << "index" << index;
   if (humanPlayer && ((m_currentPlayer == m_box->owner(index)) ||
       (m_box->owner(index) == Nobody))) {
      qDebug() << "doMove (" << index;
      doMove (index);
   }
}

void Game::buttonClick()
{
   qDebug() << "BUTTON CLICK seen: m_state" << m_state << "m_activity" << m_activity << "m_waitingState" << m_waitingState;
   switch (m_state) {
   case Waiting:
      switch (m_waitingState) {
      case WaitingToStep:
         m_waitingForStep = false;
	 doStep();
         break;
      case ComputerToMove:
         checkComputerplayer (m_playerWaiting);
         break;
      default:
         break;
      }
      break;
   case ComputerMoving:
   case Hinting:
      qDebug() << "BRAIN IS ACTIVE";
      m_ai->stop();		// Keep Stop on, for the blink animation.
      break;
   default:
      emit buttonChange (false);
      break;
   }
}

/* ***************************************************************** **
**                         status functions                          **
** ***************************************************************** */

bool Game::isActive() const
{
   return (m_activity != Idle);
}

bool Game::isMoving() const
{
   return (m_activity == AnimatingMove);
}

bool Game::isComputer (Player player) const
{
   if (player == One)
      return computerPlOne;
   else
      return computerPlTwo;
}

void Game::doMove (int index)
{
   // If a move has not finished yet do not do another move.
   if (isActive())
      return;

   // IDW TODO. Allow undo and redo of computer moves.
   //           Done, but needs further testing, esp. for race conditions
   //           and the use of m_ignoreComputerMove.

   bool computerMove = ((computerPlOne && m_currentPlayer == One) ||
                        (computerPlTwo && m_currentPlayer == Two));

   // Make a copy of the position and player to move for the Undo function.
   m_box->copyPosition (m_currentPlayer, computerMove);

   if (! computerMove) { // IDW test. Record a human move in the statistics.
      m_ai->postMove (m_currentPlayer, index, m_side); // IDW test.
   }
   emit setAction (UNDO, true);	// Update Undo and Redo actions.
   emit setAction (REDO, false);
   m_steps->clear();
   m_gameHasBeenWon = m_box->doMove (m_currentPlayer, index, m_steps);
   m_box->printBox(); // IDW test.
   qDebug() << "GAME WON?" << m_gameHasBeenWon << "STEPS" << (* m_steps);
   if (m_steps->count() > 1) {
      m_view->setWaitCursor();	//This will be a stoppable animation.
   }
   m_activity = AnimatingMove;
   doStep();
}

void Game::doStep()
{
   // Re-draw all cubes affected by a move, proceeding one step at a time.
   int index;
   bool startStep = true;
   do {
      if (! m_steps->isEmpty()) {
         index = m_steps->takeFirst();		// Get a cube to be re-drawn.

         // Check if the player wins at this step (no more cubes are re-drawn).
         if (index == 0) {
            m_view->setNormalCursor();
            emit buttonChange (false);			// Inactive.
	    m_ai->dumpStats();	// IDW test.
            QString s = i18n("The winner is Player %1!", m_currentPlayer);
            // Comment this out for IDW high-speed test.
            KMessageBox::information (m_view, s, i18n("Winner"));
	    stopActivities();				// End the move display.
	    m_activity = Idle;
            reset();					// Clear the cube box.
            return;
         }

         // Update the view of a cube, either immediately or via animation.
         startStep = (index > 0);		// + -> increment, - -> expand.
         index = startStep ? (index - 1) : (-index - 1);
         int value = m_view->cubeValue (index);		// Pre-update in view.
         int max   = m_box->maxValue (index);
         if (startStep) {				// Add 1 and take.
	    m_view->displayCube (index, m_currentPlayer, value + 1);
            if ((value >= max) && (! m_fullSpeed)) {
               m_view->highlightCube (index, true);
            }
         }
         else if (m_fullSpeed) {			// Decrease immediately.
	    m_view->displayCube (index, m_currentPlayer, value - max);
	    m_view->highlightCube (index, false);	// Maybe user hit Stop.
         }
         else {
            m_view->startAnimation (true, index);
	    if (! m_pauseForStep) {
	       emit buttonChange (true, true, i18n("Stop animation"));
	    }
         }
      }
      else {
         // Views of the cubes at all steps of the move have been updated.
	 m_view->setNormalCursor();
         emit buttonChange (false);	// Inactive.
	 m_activity = Idle;
         KCubeWidget::enableClicks(true);
         changePlayer();
         return;
      }
   } while (startStep || m_fullSpeed);
}

void Game::animationDone (int index)
{
   if (m_activity == ShowingMove) {	// Could be Hinting or ComputerMoving.
      m_activity = Idle;		// Wait if still on human player's turn.
      if (m_state == ComputerMoving) {
	 doMove (index);		// Or animate computer player's move.
      }
      else {
         m_state = HumanMoving;		// End of Hint animation.
      }
      return;
   }

   // Finished a move step.  Decrease the cube that expanded and display it.
   int value = m_view->cubeValue (index);
   int max   = m_box->maxValue (index);
   m_view->displayCube (index, m_currentPlayer, value - max);

   // IDW TODO - Need to check m_pauseForStep here?
   // IDW TODO - Integrate this in properly.
   if (m_pauseForStep) {
      m_waitingForStep = true;
      m_state = Waiting;		// Timer will start on next buttonClick.
      m_waitingState = WaitingToStep;
      emit buttonChange (true, false, i18n("Show next step"));
      return;
   }
   doStep();				// Do next animation step (if any).
}

Player Game::changePlayer()
{
   m_currentPlayer = (m_currentPlayer == One) ? Two : One;

   emit playerChanged (m_currentPlayer);
   checkComputerplayer (m_currentPlayer);
   KCubeWidget::enableClicks (true);
   return m_currentPlayer;
}

#include "game.moc"
