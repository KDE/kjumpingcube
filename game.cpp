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
#include "settingswidget.h"

#include <KConfigDialog> // IDW test.

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
   m_state         (     NotStarted),
   m_activity           (Idle),
   m_waitingState       (Nil),
   m_interrupting       (false),
   m_newSettings        (false),
   m_view               (view),
   m_dialog             (0),
   m_side               (d),
   m_currentPlayer      (One),
   m_gameHasBeenWon     (false),
   m_index              (0),
   m_fullSpeed          (false),
   computerPlOne        (false),
   computerPlTwo        (false),
   m_pauseForComputer   (false),
   m_pauseForStep       (false),
   m_ignoreComputerMove (false)
{
   qDebug() << "CONSTRUCT Game: side" << m_side;
   m_box                = new AI_Box  (this, m_side);
   m_ai                 = new AI_Main (this, m_side);
   m_steps              = new QList<int>;

   connect (m_view, SIGNAL(mouseClick(int,int)), SLOT(startHumanMove(int,int)));
   connect (m_ai,   SIGNAL(done(int)), SLOT(moveCalculationDone(int)));
   connect (m_view, SIGNAL(animationDone(int)), SLOT(animationDone(int)));
}

Game::~Game()
{
   if (isActive()) {
      shutdown();
   }
   delete m_steps;
}

void Game::gameActions (const int action)
{
   qDebug() << "GAME ACTION IS" << action;
   if (isActive() && (action != BUTTON) && (action != NEW)) {
      m_view->showPopup (i18n("Sorry, doing a move..."));
      return;
   }

   switch (action) {
   case NEW:
      newGame();
      break;
   case HINT:
      if (! isComputer (m_currentPlayer)) {
         computeMove();
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

void Game::newSettings()
{
   qDebug() << "NEW SETTINGS" << m_newSettings << "m_activity" << m_activity
            << "size:" << Prefs::cubeDim() << "m_side" << m_side;
   m_newSettings = true;
   if (m_side != Prefs::cubeDim()) {
       QMetaObject::invokeMethod (this, "newGame", Qt::QueuedConnection);
       return;
   }
   else if (m_activity == Idle) {
      loadSettings();
      m_newSettings = false;
   }
}

void Game::loadSettings()
{
  qDebug() << "GAME LOAD SETTINGS entered";
  bool oldPauseForStep     = m_pauseForStep;
  bool oldPauseForComputer = m_pauseForComputer;
  bool oldComputerPlayer   = isComputer (m_currentPlayer);
  bool oldFullSpeed        = m_fullSpeed;

  m_pauseForStep           = Prefs::pauseForStep();
  m_pauseForComputer       = Prefs::pauseForComputer();
  // IDW TODO - Compound settings, e.g. for "Interrupt game" action ...
  //            m_freeRunning = (computerPlOne && computerPlTwo &&
  //                             (! m_pauseForComputer) && (! m_pauseForStep));

  computerPlOne            = Prefs::computerPlayer1();
  computerPlTwo            = Prefs::computerPlayer2();

  m_fullSpeed              = Prefs::animationNone();

  // IDW TODO - Could this cause a (harmful) race condition?
  m_ai->setSkill (Prefs::skill1(), Prefs::kepler1(), Prefs::newton1(),
                  Prefs::skill2(), Prefs::kepler2(), Prefs::newton2());

  qDebug() << "m_pauseForComputer" << m_pauseForComputer
           << "m_pauseForStep" << m_pauseForStep; // IDW test.
  qDebug() << "PLAYER 1 settings: skill" << Prefs::skill1()
           << "Kepler" << Prefs::kepler1() << "Newton" << Prefs::newton1();
  qDebug() << "PLAYER 2 settings: skill" << Prefs::skill2()
           << "Kepler" << Prefs::kepler2() << "Newton" << Prefs::newton2();

// IDW TODO - There are just too many actions flowing from settings changes.
//            Simplify them or work out proper inter-dependencies.

  // NOTE: For a change in box-size (Prefs::cubeDim()), see Game::newSettings().
  //       It might require a current game to finish and a new game to start.

  if (oldPauseForComputer && (! m_pauseForComputer) &&
      (m_waitingState == ComputerToMove)) {
     if (isComputer (m_currentPlayer)) {
        buttonClick();
	return;
     }
     else {
	m_waitingState = Nil;
     }
  }
  if (oldPauseForStep && (! m_pauseForStep) &&
      (m_waitingState == WaitingToStep)) {
     buttonClick();
  }
  bool reColorCubes        = m_view->loadSettings();
  if (m_state == NotStarted) {		// If first move...
     return;
  }
  startNextTurn();
}

void Game::startHumanMove (int x, int y)
{
   int  index = x * m_side + y;
   bool humanPlayer = (! isComputer (m_currentPlayer));
   qDebug() << "CLICK" << x << y << "index" << index;
   if (humanPlayer && ((m_currentPlayer == m_box->owner(index)) ||
       (m_box->owner(index) == Nobody))) {
      m_state = HumanMoving;		// Needed when m_state == NotStarted.
      qDebug() << "doMove (" << index;
      doMove (index);
   }
}

void Game::startNextTurn()
{
   // Called from load settings, load game, change player and undo/redo.
   qDebug() << "startNextTurn" << m_currentPlayer
            << computerPlOne << computerPlTwo << "pause" << m_pauseForComputer
            << "wait" << m_waitingState << "state" << m_state;
   if (isComputer (m_currentPlayer)) {
      // A computer player is to move.
      KCubeWidget::enableClicks (false);
      qDebug() << "(m_pauseForComputer || (m_waitingState == ComputerToMove))"
               << (m_pauseForComputer || (m_waitingState == ComputerToMove));
      if (m_pauseForComputer || (m_waitingState == ComputerToMove)) {
         m_waitingState = ComputerToMove;
         if (computerPlOne && computerPlTwo) {
            if (m_state == NotStarted) {
               emit buttonChange (true, false, i18n("Start game"));
            }
            else {
               emit buttonChange (true, false, i18n("Continue game"));
            }
         }
         else {
             emit buttonChange (true, false, i18n("Start computer move"));
         }
	 // Wait for a button-click to show that the user is ready.
	 qDebug() << "COMPUTER MUST WAIT";
         return;
      }
      // Start the computer's move.
      qDebug() << "COMPUTER MUST MOVE";
      m_waitingState = Nil;
      m_state = ComputerMoving;
      computeMove();
   }
   else {
      // A human player is to move.
      qDebug() << "HUMAN TO MOVE";
      KCubeWidget::enableClicks (true);
      m_waitingState = Nil;
      if (computerPlOne || computerPlTwo) {
         emit buttonChange (false, false, i18n("Your turn"));
      }
      else {
         emit buttonChange (false, false, i18n("Player %1", m_currentPlayer));
      }
      // Wait for a click on the cube to be moved.
      m_state = (m_state == NotStarted) ? NotStarted : HumanMoving;
   }
}

void Game::computeMove()
{
   qDebug() << "Calling m_ai->getMove() for player" << m_currentPlayer;
   t.start(); // IDW test.
   m_view->setWaitCursor();
   if (computerPlOne && computerPlTwo && (! m_interrupting)) {
      emit buttonChange (true, true, i18n("Interrupt game"));	// Red look.
   }
   else {
      emit buttonChange (true, true, i18n("Stop computing"));	// Red look.
   }
   emit setAction (HINT, false);
   emit statusMessage (i18n("Computing a move."), false);
   m_activity = Computing;
   m_ignoreComputerMove = false;
   m_ai->getMove (m_currentPlayer, m_box);
}

void Game::moveCalculationDone (int index)
{
   // We do not care if we interrupted the computer.  It was probably taking
   // too long, so we will use the best move it had so far.

   m_activity = Idle;
   if (m_ignoreComputerMove) {
      // We are starting a new game or closing KJumpingCube.
      m_ignoreComputerMove = false;
      return;
   }
   emit statusMessage (QString(""), false);
   // IDW TODO - Three cases of blank blue button to be resolved. Delete all 3?
   if ((index < 0) || (index >= (m_side * m_side))) {
      m_view->setNormalCursor();
      // IDW TODO - DELETE? emit buttonChange (false);		// Inactive.
      KMessageBox::sorry (m_view,
                          i18n ("The computer could not find a valid move."));
      // IDW TODO - What to do about state values and BUTTON ???
      return;
   }

   qDebug() << "TIME of MOVE" << t.elapsed();
   qDebug() << "===========================================================";

   // Blink the cube to be moved (twice).
   m_view->startAnimation (false, index);

   m_activity = ShowingMove;
   if ((! (computerPlOne && computerPlOne)) || m_interrupting) {
      emit buttonChange (true, true, i18n("Stop showing move"));
   }
   // IDW TODO - DELETE? emit statusMessage (i18n("Showing a move."), false);
}

void Game::showingDone (int index)
{
   if (isComputer (m_currentPlayer)) {
      doMove (index);			// Animate computer player's move.
   }
   else {
      moveDone();			// Finish Hint animation.
      m_state = (m_state == NotStarted) ? NotStarted : HumanMoving;
      // IDW TODO - Duplicated code: need something like setHumanPlayer().
      if (computerPlOne || computerPlTwo) {
         emit buttonChange (false, false, i18n("Your turn"));
      }
      else {
         emit buttonChange (false, false, i18n("Player %1", m_currentPlayer));
      }
      emit statusMessage (QString(""), false);
   }
}

void Game::doMove (int index)
{
   // IDW TODO. Allow undo and redo of computer moves.
   // IDW TODO - When undoing/redoing computer vs. human, cannot click and
   //            change a move. Should be OK for human and NOT for computer.
   //
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
      emit statusMessage (i18n("Performing a move."), false);
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
            moveDone();
	    emit buttonChange (false, false, i18n("Game over"));
	    m_ai->dumpStats();	// IDW test.
            QString s = i18n("The winner is Player %1!", m_currentPlayer);
            KMessageBox::information (m_view, s, i18n("Winner"));
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
         else {						// Animate cascade step.
            if (m_pauseForStep && (m_waitingState != WaitingToStep)) {
               // Pause: return the step to the list and wait for a buttonClick.
               m_steps->prepend (-index - 1);
               m_waitingState = WaitingToStep;
               emit buttonChange (true, false, i18n("Show next step"));
               return;
            }
	    // Now set the button up and start the animation.
	    if (m_pauseForStep) {
               // IDW TODO - DELETE? emit buttonChange (false);
            }
	    else if ((! (computerPlOne && computerPlOne)) || m_interrupting) {
               emit buttonChange (true, true, i18n("Stop animation"));
            }
            m_view->startAnimation (true, index);
         }
      }
      else {
         // Views of the cubes at all steps of the move have been updated.
         moveDone();
         changePlayer();
         return;
      }
   } while (startStep || m_fullSpeed);
}

void Game::stepAnimationDone (int index)
{
   // Finished a move step.  Decrease the cube that expanded and display it.
   int value = m_view->cubeValue (index);
   int max   = m_box->maxValue (index);
   m_view->displayCube (index, m_currentPlayer, value - max);

   doStep();				// Do next animation step (if any).
}

void Game::moveDone()
{
   // Called after non-animated move, animated move, end of game or hint action.
   m_view->setNormalCursor();
   m_activity = Idle;
   setAction (HINT, true);
   emit statusMessage (QString(""), false);
   m_fullSpeed = Prefs::animationNone();
   m_view->hidePopup();
   if (m_interrupting) {
      m_interrupting = false;
      m_waitingState = ComputerToMove;
      // IDW TODO - Do suspended action?
   }
   if (m_newSettings) {
      m_newSettings = false;
      loadSettings();
   }
}

Player Game::changePlayer()
{
   m_currentPlayer = (m_currentPlayer == One) ? Two : One;

   emit playerChanged (m_currentPlayer);
   startNextTurn();
   return m_currentPlayer;
}

void Game::buttonClick()
{
   qDebug() << "BUTTON CLICK seen: m_state" << m_state
            << "m_activity" << m_activity << "m_waitingState" << m_waitingState;
   // IDW TODO - Add an "Interrupt game" action for computer v. computer games.
   //            This is to allow easy break-in for Save, Load or Undo.
   //            m_freeRunning = (computerPlOne && computerPlTwo && (! m_pauseForComputer) && (! m_pauseForStep));
   if (m_waitingState == Nil) {
      if ((! m_interrupting) && computerPlOne && computerPlTwo) {
         m_interrupting = true;
	 m_view->showPopup (i18n("Finishing move..."));
      }
      else if (m_activity == Computing) {
         m_ai->stop();		// Keep Stop mode on for the blink animation.
         emit statusMessage (i18n("Interrupted calculation of move."), true);
      }
      else if (m_activity == ShowingMove) {
	 int index = m_view->killAnimation();
         showingDone (index);
	 m_view->highlightCube (index, false);
      }
      else if (m_activity == AnimatingMove) {
         qDebug() << "STOP ANIMATING MOVE";
         m_fullSpeed = true;
         stepAnimationDone (m_view->killAnimation());
      }
   }
   else {
      switch (m_waitingState) {
      case WaitingToStep:
	 doStep();
         break;
      case ComputerToMove:
	 m_state = ComputerMoving;
         computeMove();
         break;
      default:
         break;
      }
      m_waitingState = Nil;
   }
}

/* ************************************************************************** */
/*                           STANDARD GAME ACTIONS                            */
/* ************************************************************************** */

void Game::newGame()
{
   qDebug() << "NEW GAME entered" << m_state << "won?" << m_gameHasBeenWon;
   if (newGameOK()) {
      shutdown();			// Stop the current move (if any).
      m_view->setNormalCursor();
      m_view->hidePopup();
      qDebug() << "setDim (" << Prefs::cubeDim() << ") m_side" << m_side;
      setDim (Prefs::cubeDim());
      qDebug() << "Entering reset();";
      reset();				// Clear the cubebox.
      emit setAction (UNDO, false);
      emit setAction (REDO, false);
      emit statusMessage (i18n("New Game"), false);
      m_state = NotStarted;
      startNextTurn();
   }
}

void Game::saveGame (bool saveAs)
{
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
      emit statusMessage (s, false);
   }
   else {
      KMessageBox::sorry (m_view, i18n("There was an error in saving file\n%1",
                                       m_gameURL.url()));
   }
}

void Game::loadGame()
{
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
   int moreToUndo = undoRedo (-1);
   if (moreToUndo >= 0) {
      emit setAction (UNDO, (moreToUndo == 1));
   }
   emit setAction (REDO, true);
}

void Game::redo()
{
   int moreToRedo = undoRedo (+1);
   if (moreToRedo >= 0) {
      emit setAction (REDO, (moreToRedo == 1));
   }
   emit setAction (UNDO, true);
}

bool Game::newGameOK()
{
   if (m_newSettings) {
      loadSettings();
      m_newSettings = false;
      qDebug() << "newGame() loadSettings DONE" << m_state
               << "won?" << m_gameHasBeenWon;
   }
   if ((m_state == NotStarted) && (m_side == Prefs::cubeDim())) return false;
   if ((m_state == NotStarted) || m_gameHasBeenWon) return true;

   QString query;
   if (m_side != Prefs::cubeDim()) {
      query = i18n("You have changed the board size setting and "
                   "that will end the game you are currently playing.\n\n"
                   "Do you wish to abandon the current game or "
                   "continue and and keep the previous setting?");
   }
   else {
      query = i18n("You have requested a new game, but "
                   "there is already a game in progress.\n\n"
                   "Do you wish to abandon the current game?");
   }
   qDebug() << "QUERY:" << query;
   int reply = KMessageBox::questionYesNo (m_view, query, i18n("New Game?"),
                                           KGuiItem (i18n("Abandon Game")),
                                           KGuiItem (i18n("Continue Game")));
   if (reply == KMessageBox::No) {
      qDebug() << "CONTINUE GAME: reset size" << Prefs::cubeDim()
               << "back to" << m_side;
      Prefs::setCubeDim (m_side);
      if (m_dialog) {		// Update dialog if it has been loaded.
          m_dialog->kcfg_CubeDim->setValue (m_side);
      }
      Prefs::self()->writeConfig();
      return false;
   }
   qDebug() << "ABANDON GAME";
   return true;
}

void Game::reset()
{
   // IDW TODO - Delete? stupActivities(); shutdown() here only (except Quit)?

   m_view->reset();
   m_box->clear();

   m_fullSpeed = Prefs::animationNone();	// Animate cascade moves?

   m_currentPlayer = One;
   m_gameHasBeenWon = false;

   // m_state          = computerPlOne ? ComputerMoving : HumanMoving;
   m_waitingState   = computerPlOne ? ComputerToMove : Nil;
   qDebug() << "RESET: state" << m_state << "wait" << m_waitingState;
   if (computerPlOne) {
      if (computerPlTwo) {
         emit buttonChange (true, false, i18n("Start game"));
      }
      else {
         emit buttonChange (true, false, i18n("Start computer move"));
      }
   }
   else {
      // IDW TODO - DELETE? Will something later set the button?
      // emit buttonChange (false);
   }

   emit playerChanged (One);
   // startNextTurn(); // Enable this line for IDW high-speed test.

   m_ai->startStats();
}

int Game::undoRedo (int actionType)
{
   Player oldPlayer = m_currentPlayer;

   bool isAI = false;
   bool moreToDo = (actionType < 0) ?
      m_box->undoPosition (m_currentPlayer, isAI) :
      m_box->redoPosition (m_currentPlayer, isAI);

   // Update the cube display after an undo or redo.
   for (int n = 0; n < (m_side * m_side); n++) {
      m_view->displayCube (n, m_box->owner (n), m_box->value (n));
      m_view->highlightCube (n, false);
   }
   // IDW TODO - Blink the cube where the undone or redone move started.

   if (oldPlayer != m_currentPlayer)
      emit playerChanged (m_currentPlayer);

   if ((actionType > 0) && (! moreToDo)) {	// If end of Redo's: restart AI
      if (! m_gameHasBeenWon) {
         startNextTurn();			// ... but not if game is over.
      }
   }
   return (moreToDo ? 1 : 0);
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
      reset();
   }
}

void Game::shutdown()
{
   // Shut down gracefully, avoiding a possible crash when the user hits Quit.
   m_view->killAnimation();	// Stop animation immediately (if active).
   if (m_activity == Computing) {
      m_ai->stop();		// Stop AI ASAP (moveCalculationDone() => Idle).
      m_ignoreComputerMove = true;
   }
   else {
      m_activity = Idle;	// In case it was ShowingMove or AnimatingMove.
   }
}

void Game::stopActivities()
{
   // IDW TODO - REWRITE THIS .................................................
   // IDW TODO - Status bar messages when activity stops - not necessarily here.
   qDebug() << "STOP ACTIVITIES";
   m_fullSpeed = true;
   if (m_activity == AnimatingMove) {
      m_view->killAnimation();
   }
   else if (m_activity == Computing) {
      qDebug() << "BRAIN IS ACTIVE";
      m_ai->stop();		// Keep Stop enabled, for the blink animation.
   }
   else if (m_activity == ShowingMove) {
   }
}

void Game::saveProperties (KConfigGroup & config)
{
   // IDW TODO - Save settings for computer player(s), animation, etc.
   //            Is Undo right for interrupted animation????
   //            Do we need Undo for interrupted computer move?
   //            What happens to the signal when the computer move ends?

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
  //            Prefs::set<name> (xxx); // <name> must a "mutable" kcfg item.
  //            if (m_dialog) m_dialog->kcfg_<name>->setValue (xxx);
  //            Prefs::self()->writeConfig();
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
   // IDW TODO - Set appropriate states and button text: NOT "Start Game".
   m_currentPlayer = (Player) onTurn;
   emit playerChanged (m_currentPlayer);
   startNextTurn();
   qDebug() << "Leaving Game::readProperties ...";
}

bool Game::isActive() const
{
   // IDW TODO - What if shutdown() is in effect? Could have
   //            m_activity == Computing and m_ignoreComputerMove == true.
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

void Game::animationDone (int index)
{
   if (m_activity == ShowingMove) {
      showingDone (index);
   }
   else {
      stepAnimationDone (index);
   }
}

#include "game.moc"
