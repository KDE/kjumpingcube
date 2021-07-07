/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
    SPDX-FileCopyrightText: 2012-2013 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "game.h"
#include "kjumpingcube_version.h"
#include "ai_main.h"
#include "ai_box.h"
#include "kcubeboxwidget.h"
#include "settingswidget.h"

#include "kjumpingcube_debug.h"
#include <QFileDialog>
#include <QTemporaryFile>

#include <KConfigDialog> // IDW test.
#include <KIO/CopyJob>
#include <KIO/Job>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <kio_version.h>

#include "prefs.h"

#define LARGE_NUMBER 999999

Game::Game (const int d, KCubeBoxWidget * view, QWidget * parent)
   :
   QObject ((QObject *) parent),	// Delete Game when window is deleted.
   m_activity           (Idle),
   m_waitingState       (Nil),
   m_waitingToMove      (false),
   m_moveNo             (0),		// Game not started.
   m_endMoveNo          (LARGE_NUMBER),	// Game not finished.
   m_interrupting       (false),
   m_newSettings        (false),
   m_parent             (parent),
   m_view               (view),
   m_settingsPage       (nullptr),
   m_side               (d),
   m_currentPlayer      (One),
   m_index              (0),
   m_fullSpeed          (false),
   computerPlOne        (false),
   computerPlTwo        (false),
   m_pauseForComputer   (false),
   m_pauseForStep       (false)
{
   qCDebug(KJUMPINGCUBE_LOG) << "CONSTRUCT Game: side" << m_side;
   m_box                = new AI_Box  (this, m_side);
   m_ai                 = new AI_Main (this, m_side);
   m_steps              = new QList<int>;

   connect(m_view, &KCubeBoxWidget::mouseClick, this, &Game::startHumanMove);
   connect(m_ai, &AI_Main::done, this, &Game::moveCalculationDone);
   connect(m_view, &KCubeBoxWidget::animationDone, this, &Game::animationDone);
}

Game::~Game()
{
   if ((m_activity != Idle) && (m_activity != Aborting)) {
      shutdown();
   }
   delete m_steps;
}

void Game::gameActions (const int action)
{
   qCDebug(KJUMPINGCUBE_LOG) << "GAME ACTION IS" << action;
   if ((m_activity != Idle) && (action != BUTTON) && (action != NEW)) {
      m_view->showPopup (i18n("Sorry, doing a move..."));
      return;
   }

   switch (action) {
   case NEW:
      newGame();
      break;
   case HINT:
      if (! isComputer (m_currentPlayer)) {
         KCubeWidget::enableClicks (false);
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

void Game::showWinner()
{
   Q_EMIT buttonChange (false, false, i18n("Game over"));
   QString s = i18n("The winner is Player %1!", m_currentPlayer);
   KMessageBox::information (m_view, s, i18n("Winner"));
}

void Game::showSettingsDialog (bool show)
{
   // Show the Preferences/Settings/Configuration dialog.
   KConfigDialog * settings = KConfigDialog::exists (QStringLiteral("settings"));
   if (! settings) {
      settings = new KConfigDialog (m_parent, QStringLiteral("settings"), Prefs::self());
      settings->setFaceType (KPageDialog::Plain);
      SettingsWidget * widget = new SettingsWidget (m_parent);
      settings->addPage (widget, i18n("General"), QStringLiteral("games-config-options"));
      connect(settings, &KConfigDialog::settingsChanged, this, &Game::newSettings);
      m_settingsPage = widget;		// Used when reverting/editing settings.
   }
   if (! show) return;
   settings->show();
   settings->raise();			// Force the dialog to be in front.
}

void Game::newSettings()
{
   qCDebug(KJUMPINGCUBE_LOG) << "NEW SETTINGS" << m_newSettings << "m_activity" << m_activity
            << "size:" << Prefs::cubeDim() << "m_side" << m_side;
   loadImmediateSettings();

   m_newSettings = true;
   if (m_side != Prefs::cubeDim()) {
      QMetaObject::invokeMethod (this, "newGame", Qt::QueuedConnection);
      return;
   }
   // Waiting to move and not Hinting?
   else if (m_waitingToMove && (m_activity == Idle)) {
      loadPlayerSettings();
      m_newSettings = false;
      setUpNextTurn();
   }
   // Else, the remaining settings will be loaded at the next start of a turn.
   // They set computer pause on/off and computer player 1 or 2 on/off.
}

void Game::loadImmediateSettings()
{
   qCDebug(KJUMPINGCUBE_LOG) << "GAME LOAD IMMEDIATE SETTINGS entered";

   // Color changes can take place as soon as control returns to the event loop.
   // Changes of animation type or speed will take effect next time there is an
   // animation step to do, regardless of current activity.
   bool reColorCubes = m_view->loadSettings();
   m_fullSpeed = Prefs::animationNone();
   m_pauseForStep = Prefs::pauseForStep();
   if (reColorCubes) {
      Q_EMIT playerChanged (m_currentPlayer);	// Re-display status bar icon.
   }

   // Choices of computer AIs and skills will take effect next time there is a
   // computer move or hint.  They will not affect any calculation in progress.
   m_ai->setSkill (Prefs::skill1(), Prefs::kepler1(), Prefs::newton1(),
                   Prefs::skill2(), Prefs::kepler2(), Prefs::newton2());

   qCDebug(KJUMPINGCUBE_LOG) << "m_pauseForStep" << m_pauseForStep;
   qCDebug(KJUMPINGCUBE_LOG) << "PLAYER 1 settings: skill" << Prefs::skill1()
            << "Kepler" << Prefs::kepler1() << "Newton" << Prefs::newton1();
   qCDebug(KJUMPINGCUBE_LOG) << "PLAYER 2 settings: skill" << Prefs::skill2()
            << "Kepler" << Prefs::kepler2() << "Newton" << Prefs::newton2();
}

void Game::loadPlayerSettings()
{
   qCDebug(KJUMPINGCUBE_LOG) << "GAME LOAD PLAYER SETTINGS entered";
   bool oldComputerPlayer   = isComputer (m_currentPlayer);

   m_pauseForComputer       = Prefs::pauseForComputer();
   computerPlOne            = Prefs::computerPlayer1();
   computerPlTwo            = Prefs::computerPlayer2();

   qCDebug(KJUMPINGCUBE_LOG) << "AI 1" << computerPlOne << "AI 2" << computerPlTwo
            << "m_pauseForComputer" << m_pauseForComputer;

   if (isComputer (m_currentPlayer) && (! oldComputerPlayer)) {
      qCDebug(KJUMPINGCUBE_LOG) << "New computer player set: must wait.";
      m_waitingState = ComputerToMove;	// New player: don't start playing yet.
   }
}

void Game::startHumanMove (int x, int y)
{
   int  index = x * m_side + y;
   bool humanPlayer = (! isComputer (m_currentPlayer));
   qCDebug(KJUMPINGCUBE_LOG) << "CLICK" << x << y << "index" << index;
   if (! humanPlayer) {
      buttonClick();
   }
   else if (humanPlayer && ((m_currentPlayer == m_box->owner(index)) ||
       (m_box->owner(index) == Nobody))) {
      m_waitingToMove = false;
      m_moveNo++;
      m_endMoveNo = LARGE_NUMBER;
      qCDebug(KJUMPINGCUBE_LOG) << "doMove (" << index;
      KCubeWidget::enableClicks (false);
      doMove (index);
   }
}

void Game::setUpNextTurn()
{
   // Called from newSettings(), new game, load, change player and undo/redo.
   if (m_newSettings) {
      m_newSettings = false;
      loadPlayerSettings();
   }
   qCDebug(KJUMPINGCUBE_LOG) << "setUpNextTurn" << m_currentPlayer
            << computerPlOne << computerPlTwo << "pause" << m_pauseForComputer
            << "wait" << m_waitingState << "waiting" << m_waitingToMove;
   if (isComputer (m_currentPlayer)) {
      // A computer player is to move.
      qCDebug(KJUMPINGCUBE_LOG) << "(m_pauseForComputer || (m_waitingState == ComputerToMove))"
               << (m_pauseForComputer || (m_waitingState == ComputerToMove));
      if (m_pauseForComputer || (m_waitingState == ComputerToMove) ||
          (m_moveNo == 0)) {
         m_waitingState = ComputerToMove;
	 m_waitingToMove = true;
         if (computerPlOne && computerPlTwo) {
            if (m_moveNo == 0) {
               Q_EMIT buttonChange (true, false, i18n("Start game"));
            }
            else {
               Q_EMIT buttonChange (true, false, i18n("Continue game"));
            }
         }
         else {
             Q_EMIT buttonChange (true, false, i18n("Start computer move"));
         }
	 // Wait for a button-click to show that the user is ready.
	 qCDebug(KJUMPINGCUBE_LOG) << "COMPUTER MUST WAIT";
         KCubeWidget::enableClicks (true);
         return;
      }
      // Start the computer's move.
      qCDebug(KJUMPINGCUBE_LOG) << "COMPUTER MUST MOVE";
      m_waitingState = Nil;
      m_waitingToMove = false;
      KCubeWidget::enableClicks (false);
      computeMove();
   }
   else {
      // A human player is to move.
      qCDebug(KJUMPINGCUBE_LOG) << "HUMAN TO MOVE";
      KCubeWidget::enableClicks (true);
      m_waitingState = Nil;
      m_waitingToMove = true;
      if (computerPlOne || computerPlTwo) {
         Q_EMIT buttonChange (false, false, i18n("Your turn"));
      }
      else {
         Q_EMIT buttonChange (false, false, i18n("Player %1", m_currentPlayer));
      }
      // Wait for a click on the cube to be moved.
   }
}

void Game::computeMove()
{
#if AILog > 1
   t.start();			// Start timing the AI calculation.
#endif
   m_view->setWaitCursor();
   m_activity = Computing;
   setStopAction();
   Q_EMIT setAction (HINT, false);
   if (isComputer (m_currentPlayer)) {
       Q_EMIT statusMessage (i18n("Computer player %1 is moving", m_currentPlayer), false);
   }
   m_ai->getMove (m_currentPlayer, m_box);
}

void Game::moveCalculationDone (int index)
{
   // We do not care if we interrupted the computer.  It was probably taking
   // too long, so we will use the best move it had so far.

   // We are starting a new game or closing KJumpingCube.  See shutdown().
   if (m_activity == Aborting) {
      m_activity = Idle;
      return;
   }

   m_activity = Idle;
   if ((index < 0) || (index >= (m_side * m_side))) {
      m_view->setNormalCursor();
      KMessageBox::sorry (m_view,
                          i18n ("The computer could not find a valid move."));
      // IDW TODO - What to do about state values and BUTTON ???
      return;
   }

#if AILog > 1
   qCDebug(KJUMPINGCUBE_LOG) << "TIME of MOVE" << t.elapsed();
   qCDebug(KJUMPINGCUBE_LOG) << "==============================================================";
#endif

   // Blink the cube to be moved (twice).
   m_view->startAnimation (false, index);

   m_activity = ShowingMove;
   setStopAction();
}

void Game::showingDone (int index)
{
   if (isComputer (m_currentPlayer)) {
      m_moveNo++;
      m_endMoveNo = LARGE_NUMBER;
      qCDebug(KJUMPINGCUBE_LOG) << "m_moveNo" << m_moveNo << "isComputer()" << (isComputer (m_currentPlayer));
      doMove (index);			// Animate computer player's move.
   }
   else {
      moveDone();			// Finish Hint animation.
      setUpNextTurn();			// Wait: unless player setting changed.
   }
}

void Game::doMove (int index)
{
   bool computerMove = ((computerPlOne && m_currentPlayer == One) ||
                        (computerPlTwo && m_currentPlayer == Two));

   // Make a copy of the position and player to move for the Undo function.
   m_box->copyPosition (m_currentPlayer, computerMove, index);

#if AILog > 0
   if (! computerMove) { // Record a human move in the statistics.
      m_ai->postMove (m_currentPlayer, index, m_side);
   }
#endif
   Q_EMIT setAction (UNDO, true);	// Update Undo and Redo actions.
   Q_EMIT setAction (REDO, false);
   m_steps->clear();
   bool won = m_box->doMove (m_currentPlayer, index, nullptr, m_steps);
#if AILog > 1
   qCDebug(KJUMPINGCUBE_LOG) << "GAME WON?" << won << "STEPS" << (* m_steps);
   // m_box->printBox();
#endif
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
            moveDone();
	    m_endMoveNo = m_moveNo;
#if AILog > 0
	    qCDebug(KJUMPINGCUBE_LOG) << "\nCALLING dumpStats()";
	    m_ai->dumpStats();
#endif
	    showWinner();
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
               Q_EMIT buttonChange (true, false, i18n("Show next step"));
               return;
            }
	    // Now set the button up and start the animation.
            setStopAction();
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
   Q_EMIT statusMessage (QLatin1String(""), false);	// Clear the status bar.
   m_activity = Idle;
   setAction (HINT, true);
   m_fullSpeed = Prefs::animationNone();
   m_view->hidePopup();
   if (m_interrupting) {
      m_interrupting = false;
      m_waitingState = ComputerToMove;
   }
}

Player Game::changePlayer()
{
   m_currentPlayer = (m_currentPlayer == One) ? Two : One;
   Q_EMIT playerChanged (m_currentPlayer);
   setUpNextTurn();
   return m_currentPlayer;
}

void Game::buttonClick()
{
   qCDebug(KJUMPINGCUBE_LOG) << "BUTTON CLICK seen: waiting" << m_waitingToMove
            << "m_activity" << m_activity << "m_waitingState" << m_waitingState;
   if (m_waitingState == Nil) {		// Button is red: stop an activity.
      if ((! m_pauseForComputer) && (! m_interrupting) &&
          (computerPlOne && computerPlTwo)) {
         m_interrupting = true;		// Interrupt a non-stop AI v AI game.
	 m_view->showPopup (i18n("Finishing move..."));
	 setStopAction();		// Change to text for current activity.
      }
      else if (m_activity == Computing) {
         m_ai->stop();			// Stop calculating a move or hint.
         m_activity = Stopping;
      }
      else if (m_activity == ShowingMove) {
	 int index = m_view->killAnimation();
         showingDone (index);		// Stop showing where a move or hint is.
	 m_view->highlightCube (index, false);
      }
      else if (m_activity == AnimatingMove) {
	 int index = m_view->killAnimation();
         m_fullSpeed = true;		// Go to end of move right now, skipping
         stepAnimationDone (index);	// all later steps of animation.
      }
   }
   else {				// Button is green: start an activity.
      switch (m_waitingState) {
      case WaitingToStep:
	 doStep();			// Start next animation step.
         break;
      case ComputerToMove:
         computeMove();			// Start next computer move.
         break;
      default:
         break;
      }
      m_waitingState = Nil;
   }
}

void Game::setStopAction()
{
   // Red button setting for non-stop AI v. AI game.
   if ((! m_pauseForComputer) && (! m_interrupting) &&
       (computerPlOne && computerPlTwo)) {
      if (m_activity == Computing) {		// Starting AI v. AI move.
         Q_EMIT buttonChange (true, true, i18n("Interrupt game"));
      }
      return;					// Continuing AI v. AI move.
   }
   // Red button settings for AI v. human, two human players or pausing game.
   if (m_activity == Computing) {		// Calculating hint or AI move.
      Q_EMIT buttonChange (true, true, i18n("Stop computing"));
   }
   else if (m_activity == ShowingMove) {	// Showing hint or AI move.
      Q_EMIT buttonChange (true, true, i18n("Stop showing move"));
   }
   else if (m_activity == AnimatingMove) {	// Animating AI or human move.
      Q_EMIT buttonChange (true, true, i18n("Stop animation"));
   }
}

/* ************************************************************************** */
/*                           STANDARD GAME ACTIONS                            */
/* ************************************************************************** */

void Game::newGame()
{
   qCDebug(KJUMPINGCUBE_LOG) << "NEW GAME entered: waiting" << m_waitingToMove
            << "won?" << (m_moveNo >= m_endMoveNo);
   if (newGameOK()) {
      qCDebug(KJUMPINGCUBE_LOG) << "QDEBUG: newGameOK() =" << true;
      shutdown();			// Stop the current move (if any).
      m_view->setNormalCursor();
      m_view->hidePopup();
      loadImmediateSettings();
      loadPlayerSettings();
      m_newSettings = false;
      qCDebug(KJUMPINGCUBE_LOG) << "newGame() loadSettings DONE: waiting" << m_waitingToMove
               << "won?" << (m_moveNo >= m_endMoveNo)
               << "move" << m_moveNo << m_endMoveNo;
      qCDebug(KJUMPINGCUBE_LOG) << "setDim (" << Prefs::cubeDim() << ") m_side" << m_side;
      setDim (Prefs::cubeDim());
      qCDebug(KJUMPINGCUBE_LOG) << "Entering reset();";
      reset();				// Clear cubebox, initialise states.
      Q_EMIT setAction (UNDO, false);
      Q_EMIT setAction (REDO, false);
      Q_EMIT statusMessage (i18n("New Game"), false);
      m_moveNo = 0;
      m_endMoveNo = LARGE_NUMBER;
      setUpNextTurn();
   }
   else qCDebug(KJUMPINGCUBE_LOG) << "QDEBUG: newGameOK() =" << false;
}

void Game::saveGame (bool saveAs)
{
   if (saveAs || m_gameURL.isEmpty()) {
      int result=0;
      QUrl url;

      do {
         url = QFileDialog::getSaveFileUrl (m_view, QString(), m_gameURL, QStringLiteral("*.kjc"));

         if (url.isEmpty())
            return;

         // check filename
         QRegExp pattern (QStringLiteral("*.kjc"), Qt::CaseSensitive, QRegExp::Wildcard);
         if (! pattern.exactMatch (url.fileName())) {
            url.setPath(url.path() + QLatin1String(".kjc"));
         }

         KIO::StatJob* statJob = KIO::statDetails(url, KIO::StatJob::DestinationSide, KIO::StatNoDetails);
         KJobWidgets::setWindow(statJob, m_view);
         if (statJob->exec()) {
            QString mes=i18n("The file %1 already exists.\n"
               "Do you want to overwrite it?", url.url());
            result = KMessageBox::warningContinueCancel
               (m_view, mes, QString(), KStandardGuiItem::overwrite());
            if (result == KMessageBox::Cancel)
               return;
         }
      } while (result == KMessageBox::No);

      m_gameURL = url;
   }

   QTemporaryFile tempFile;
   tempFile.open();
   KConfig config (tempFile.fileName(), KConfig::SimpleConfig);
   KConfigGroup main (&config, "KJumpingCube");
   main.writeEntry ("Version", KJUMPINGCUBE_VERSION_STRING);
   KConfigGroup game (&config, "Game");
   saveProperties (game);
   config.sync();

   KIO::FileCopyJob *job = KIO::file_copy(QUrl::fromLocalFile(tempFile.fileName()), m_gameURL, -1, KIO::Overwrite);
   KJobWidgets::setWindow(job, m_view);
   job->exec();
   if (! job->error() ) {
      Q_EMIT statusMessage (i18n("Game saved as %1", m_gameURL.url()), false);
   }
   else {
      KMessageBox::sorry (m_view, i18n("There was an error in saving file\n%1",
                                       m_gameURL.url()));
   }
}

void Game::loadGame()
{
   bool fileOk=true;
   QUrl url;

   do {
      url = QFileDialog::getOpenFileUrl (m_view, QString(), m_gameURL, QStringLiteral("*.kjc"));
      if (url.isEmpty())
         return;
      KIO::StatJob* statJob = KIO::statDetails(url, KIO::StatJob::SourceSide, KIO::StatNoDetails);
      KJobWidgets::setWindow(statJob, m_view);
      if (! statJob->exec()) {
         QString mes = i18n("The file %1 does not exist!", url.url());
         KMessageBox::sorry (m_view, mes);
         fileOk = false;
      }
   } while (! fileOk);

   QTemporaryFile tempFile;
   tempFile.open();
   KIO::FileCopyJob *job = KIO::file_copy(url, QUrl::fromLocalFile(tempFile.fileName()), -1, KIO::Overwrite);
   KJobWidgets::setWindow(job, m_view);
   job->exec();
   if (! job->error() ) {
      KConfig config( tempFile.fileName(), KConfig::SimpleConfig);
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

      Q_EMIT setAction (UNDO, false);
   }
   else
      KMessageBox::sorry (m_view, i18n("There was an error loading file\n%1",
                                       url.url()));
}

void Game::undo()
{
   bool moreToUndo = undoRedo (-1);
   Q_EMIT setAction (UNDO, moreToUndo);
   Q_EMIT setAction (REDO, true);
}

void Game::redo()
{
   bool moreToRedo = undoRedo (+1);
   Q_EMIT setAction (REDO, moreToRedo);
   Q_EMIT setAction (UNDO, true);
}

bool Game::newGameOK()
{
   if ((m_moveNo == 0) || (m_moveNo >= m_endMoveNo)) {
      // OK: game finished or not yet started.  Settings might have changed.
      return true;
   }

   // Check if it is OK to abandon the current game.
   QString query;
   if (m_side != Prefs::cubeDim()) {
      query = i18n("You have changed the size setting of the game and "
                   "that requires a new game to start.\n\n"
                   "Do you wish to abandon the current game or continue "
                   "playing and restore the previous size setting?");
   }
   else {
      query = i18n("You have requested a new game, but "
                   "there is already a game in progress.\n\n"
                   "Do you wish to abandon the current game?");
   }
   qCDebug(KJUMPINGCUBE_LOG) << "QUERY:" << query;
   int reply = KMessageBox::questionYesNo (m_view, query, i18n("New Game?"),
                                           KGuiItem (i18n("Abandon Game")),
                                           KGuiItem (i18n("Continue Game")));
   if (reply == KMessageBox::Yes) {
      qCDebug(KJUMPINGCUBE_LOG) << "ABANDON GAME";
      return true;			// Start a new game.
   }
   if (m_side != Prefs::cubeDim()) {
      // Restore the setting: also the dialog-box copy if it has been created.
      qCDebug(KJUMPINGCUBE_LOG) << "Reset size" << Prefs::cubeDim() << "back to" << m_side;
      Prefs::setCubeDim (m_side);
      if (m_settingsPage) {
          m_settingsPage->kcfg_CubeDim->setValue (m_side);
      }
      Prefs::self()->save();
   }
   qCDebug(KJUMPINGCUBE_LOG) << "CONTINUE GAME";
   return false;			// Continue the current game.
}

void Game::reset()
{
   m_view->reset();
   m_box->clear();

   m_fullSpeed = Prefs::animationNone();	// Animate cascade moves?

   m_currentPlayer = One;

   m_waitingState   = computerPlOne ? ComputerToMove : Nil;
   qCDebug(KJUMPINGCUBE_LOG) << "RESET: activity" << m_activity << "wait" << m_waitingState;

#if AILog > 0
   m_ai->startStats();
#endif
}

bool Game::undoRedo (int change)
{
   Player oldPlayer = m_currentPlayer;

   bool isAI = false;
   int  index = 0;
   bool moreToDo = (change < 0) ?
      m_box->undoPosition (m_currentPlayer, isAI, index) :
      m_box->redoPosition (m_currentPlayer, isAI, index);

   // Update the cube display after an undo or redo.
   for (int n = 0; n < (m_side * m_side); n++) {
      m_view->displayCube (n, m_box->owner (n), m_box->value (n));
      m_view->highlightCube (n, false);
   }
   m_view->timedCubeHighlight (index);		// Show which cube was moved.

   m_moveNo = m_moveNo + change;
   if (m_moveNo < m_endMoveNo) {
      if (oldPlayer != m_currentPlayer) {
         Q_EMIT playerChanged (m_currentPlayer);
      }
      m_waitingState = isComputer (m_currentPlayer) ? ComputerToMove
                                                    : m_waitingState;
      setUpNextTurn();
   }
   else {					// End of game: show winner.
      moreToDo = false;
      m_currentPlayer = oldPlayer;
      showWinner();
   }
   return moreToDo;
}

void Game::setDim (int d)
{
   if (d != m_side) {
      shutdown();
      delete m_box;
      m_box   = new AI_Box (this, d);
      qCDebug(KJUMPINGCUBE_LOG) << "AI_Box CONSTRUCTED by Game::setDim()";
      m_side  = d;
      m_view->setDim (d);
   }
}

void Game::shutdown()
{
   // Shut down gracefully, avoiding a possible crash when the user hits Quit.
   m_view->killAnimation();	// Stop animation immediately (if active).
   if (m_activity == Computing) {
      m_ai->stop();		// Stop AI ASAP (moveCalculationDone() => Idle).
      m_activity = Aborting;
   }
   else if (m_activity == Stopping) {
      m_activity = Aborting;
   }
   else if (m_activity != Aborting) {
      m_activity = Idle;	// In case it was ShowingMove or AnimatingMove.
   }
}

void Game::saveProperties (KConfigGroup & config)
{
   // Save the current player.
   config.writeEntry ("onTurn", (int) m_currentPlayer);

   QStringList list;
   QString owner, value, key;

   // Save the position currently on the board.
   for (int x = 0; x < m_side; x++) {
     for (int y = 0; y < m_side; y++) {
    	key = QString::asprintf ("%u,%u", x, y);
	int index = x * m_side + y;
	owner = QString::asprintf ("%u", m_box->owner (index));
	value = QString::asprintf ("%u", m_box->value (index));
	list.append (owner);
	list.append (value);
	config.writeEntry (key, list);

	list.clear();
     }
   }

   // Save the game and player settings.
   config.writeEntry ("CubeDim",          m_side);
   config.writeEntry ("PauseForComputer", m_pauseForComputer ? 1 : 0);
   config.writeEntry ("ComputerPlayer1",  computerPlOne);
   config.writeEntry ("ComputerPlayer2",  computerPlTwo);
   config.writeEntry ("Kepler1",          Prefs::kepler1());
   config.writeEntry ("Kepler2",          Prefs::kepler2());
   config.writeEntry ("Newton1",          Prefs::newton1());
   config.writeEntry ("Newton2",          Prefs::newton2());
   config.writeEntry ("Skill1",           Prefs::skill1());
   config.writeEntry ("Skill2",           Prefs::skill2());
}

void Game::readProperties (const KConfigGroup& config)
{
  QStringList list;
  QString     key;
  int         owner, value, maxValue;

  // Dimension must be 3 to 15 (see definition in ai_box.h).
  int cubeDim = config.readEntry ("CubeDim", minSide);
  if ((cubeDim < minSide) || (cubeDim > maxSide)) {
     KMessageBox::sorry (m_view, i18n("The file's cube box size is outside "
                                    "the range %1 to %2. It will be set to %1.",
                                    minSide, maxSide));
     cubeDim = 3;
  }

  m_side = 1;					// Create a new cube box.
  setDim (cubeDim);
  reset();		// IDW TODO - NEEDED? Is newGame() init VALID here?

  for (int x = 0; x < m_side; x++) {
    for (int y = 0; y < m_side; y++) {
	key = QString::asprintf ("%u,%u", x, y);
	list = config.readEntry (key, QStringList());
	// List length must be 2, owner must be 0-2, value >= 1 and <= max().
	if (list.count() < 2) {
	    KMessageBox::sorry (m_view, i18n("Missing input line for cube %1.", key));
	    owner = 0;
	    value = 1;
	}
	else {
	    owner = list.at(0).toInt();
	    value = list.at(1).toInt();
	}
	if ((owner < 0) || (owner > 2)) {
	    KMessageBox::sorry (m_view, i18n("Owner of cube %1 is outside the "
                                           "range 0 to 2.", key));
	    owner = 0;
	}
	int index = x * m_side + y;
	maxValue = (owner == 0) ? 1 : m_box->maxValue (index);
	if ((value < 1) || (value > maxValue)) {
	    KMessageBox::sorry (m_view, i18n("Value of cube %1 is outside the "
                                           "range 1 to %2.", key, maxValue));
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
   Q_EMIT playerChanged (m_currentPlayer);

   // Restore the game and player settings.
   loadSavedSettings (config);
   Prefs::self()->save();
   setUpNextTurn();
}

void Game::loadSavedSettings (const KConfigGroup& config)
{
   showSettingsDialog (false);	// Load the settings dialog but do not show it.
   if (m_side != Prefs::cubeDim()) {
      // Update the size setting for the loaded game.
      Prefs::setCubeDim (m_side);
      m_settingsPage->kcfg_CubeDim->setValue (m_side);
   }

   int pause = config.readEntry ("PauseForComputer", -1);
   if (pause < 0) {		// Older files will not contain more settings,
      return;			// so keep the existing settings.
   }

   // Load the PauseForComputer and player settings.
   bool boolValue = pause > 0 ? true : false;
   Prefs::setPauseForComputer (boolValue);
   m_settingsPage->kcfg_PauseForComputer->setChecked (boolValue);
   m_pauseForComputer = boolValue;

   boolValue = config.readEntry ("ComputerPlayer1", false);
   Prefs::setComputerPlayer1 (boolValue);
   m_settingsPage->kcfg_ComputerPlayer1->setChecked (boolValue);
   computerPlOne = boolValue;

   boolValue = config.readEntry ("ComputerPlayer2", true);
   Prefs::setComputerPlayer2 (boolValue);
   m_settingsPage->kcfg_ComputerPlayer2->setChecked (boolValue);
   computerPlTwo = boolValue;

   boolValue = config.readEntry ("Kepler1", true);
   Prefs::setKepler1 (boolValue);
   m_settingsPage->kcfg_Kepler1->setChecked (boolValue);

   boolValue = config.readEntry ("Kepler2", true);
   Prefs::setKepler2 (boolValue);
   m_settingsPage->kcfg_Kepler2->setChecked (boolValue);

   boolValue = config.readEntry ("Newton1", false);
   Prefs::setNewton1 (boolValue);
   m_settingsPage->kcfg_Newton1->setChecked (boolValue);

   boolValue = config.readEntry ("Newton2", false);
   Prefs::setNewton2 (boolValue);
   m_settingsPage->kcfg_Newton2->setChecked (boolValue);

   int intValue = config.readEntry ("Skill1", 2);
   Prefs::setSkill1 (intValue);
   m_settingsPage->kcfg_Skill1->setValue (intValue);

   intValue = config.readEntry ("Skill2", 2);
   Prefs::setSkill2 (intValue);
   m_settingsPage->kcfg_Skill2->setValue (intValue);

   m_ai->setSkill (Prefs::skill1(), Prefs::kepler1(), Prefs::newton1(),
                   Prefs::skill2(), Prefs::kepler2(), Prefs::newton2());
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


