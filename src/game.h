/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>
    SPDX-FileCopyrightText: 2012-2013 Ian Wadham <iandw.au@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef GAME_H
#define GAME_H

#include "ai_globals.h"

#include <QObject>
#include <QTime> // IDW test.

#include <QUrl>
#include <QList>

class KConfigGroup;
class KCubeBoxWidget;
class SettingsWidget;
class AI_Main;
class AI_Box;

/**
* Codes for actions available to users of the Game class.
*/
enum Action {NEW, HINT, BUTTON, UNDO, REDO, SAVE, SAVE_AS, LOAD};

class Game : public QObject
{
   Q_OBJECT
   friend class KJumpingCube;

public:
   explicit Game (const int dim, KCubeBoxWidget * view, QWidget * parent = nullptr);

   virtual ~Game();

   /**
    * Make sure all animation and AI activity is over before closing a game.
    * */
   void shutdown();

public Q_SLOTS:

   /**
    * Perform one of the game-actions listed in enum type Action.
    *
    * @param action  The action to be performed, taken from enum Action.
    *                This parameter must be of type int because KJumpingCube
    *                invokes gameActions() via a QSignalMapper of int type.
    */
   void gameActions (const int action);

private:
   /**
    * Show the winner when the game ends, either after a move or a redo.
    */
   void showWinner();

private Q_SLOTS:
   /**
    * Pop up the settings/preferences/configuration dialog window.
    */
   void showSettingsDialog (bool show = true);

   /**
    * Check changes in the user's preferences or settings.  Some settings can
    * be used immediately, some must wait until the start of the next turn and
    * a change in box size may require the user's OK (or not) to abandon a game
    * in progress and start a new game.
    */
   void newSettings();

   /**
   * Check if cube at [x, y] is available to the current (human) player.
   * If it is, make a move using doMove() and the cube at [x, y], animating
   * the move, if required.
   */
   void startHumanMove (int x, int y);

private:
   /**
   * Check if the current player is a computer player and, if so, start the
   * next move or perhaps wait for the user to click the button to show that
   * he or she is ready.  Otherwise, wait for a human player to move.
   */
   void setUpNextTurn();

   /**
    * Starts calculating a computer move or hint (using asynchronous AI thread).
    */
   void computeMove();

private Q_SLOTS:
   /**
    * Deliver a computer-move or hint calculated by the AI thread and start an
    * animation to show which cube is to move.
    *
    * @param index  The position of the move in the array of cubes. If the box
    *               has n x n cubes, index = x * n + y for co-ordinates [x, y].
    */
   void moveCalculationDone (int index);

private:
   /**
    * Finish showing a computer move or hint. If it is a computer move, use
    * doMove() to make the actual move and animate it, if required.
    */
   void showingDone (int index);

   /**
   * Increase the cube at 'index' and start the animation loop, if required.
   */
   void   doMove (int index);

   /**
    * Perform and display one or more steps of a move, with breaks for animation
    * if required, finishing when all steps have been done or the current player
    * has won the game. If there is no animation or if the user clicks the
    * action button to interrupt the animation, perform all steps at full speed.
    */
   void   doStep();

   /**
    * Finish one step of a move, if animating, and use doStep() to continue.
    */
   void stepAnimationDone (int index);

   /**
    * Finish an entire move.
    */
   void moveDone();

   /**
    * Switch to the other player, human or computer, after a non-winning move.
    */
   Player changePlayer();

   /**
    * Initiate or interrupt computer-move or animation activities. This is used
    * when the computer is first to move or when the user wishes to interrupt
    * or step through moves, as in a computer v. computer game or a complex
    * series of cascade moves.
    */
   void buttonClick();

   /**
    * Set the action-button to be red, enabled and with text relevant to
    * stopping the current activity (AI v AI game OR computing, showing or
    * animating a move).
    */
   void setStopAction();

   /**
    * Act on changes in settings that can be used immediately.  Color changes
    * can take effect when control returns to the event loop, the other settings
    * next time they are used.  They include animation settings, AI players and
    * their skill levels.
    */
   void loadImmediateSettings();

   /**
    * Act on changes in the user's choice of players and computer-player pause.
    * These can take effect only at the start of a turn.
    */
   void loadPlayerSettings();

private Q_SLOTS:
   /**
    * Indicate that showing a move or animating a move-step has finished.
    *
    * @param index  The position of the move in the array of cubes. If the box
    *               has n x n cubes, index = x * n + y for co-ordinates [x, y].
    */
   void animationDone (int index);

Q_SIGNALS:
   /**
    * Request that the current player be shown in the KJumpingCube main window.
    *
    * @param player   The ID of the player (= 1 or 2).
    */
   void playerChanged (int newPlayer);

   /**
    * Request a change of appearance or text of the general-purpose action
    * button in the KJumpingCube main window.
    *
    * @param enabled  True = button is active, false = button is inactive.
    * @param stop     If active, true = use STOP style, false = use GO style.
    * @param caption  Translated text to appear on the button.
    */
   void buttonChange  (bool enabled, bool stop = false,
                       const QString & caption = QLatin1String(""));

   /**
    * Request enabling or disabling of an action by KJumpingCube main window.
    *
    * @param a      The game-action to be enabled or disabled.
    * @param onOff  True = enable the action, false = disable it.
    */
   void setAction     (const Action a, const bool onOff);

   /**
    * Request display of a status message.
    *
    * @param message  The message to be displayed (translated).
    * @param timed    If true, display the message for a limited time.
    */
   void statusMessage (const QString & message, bool timed);

protected:
   void saveProperties(KConfigGroup&);
   void readProperties(const KConfigGroup&);

private:
   void loadSavedSettings (const KConfigGroup& config);

private:
   QTime t; // IDW test.

   enum Activity     {Idle, Computing, Stopping, Aborting,
                      ShowingMove, AnimatingMove};
   enum WaitingState {Nil, WaitingToStep, ComputerToMove};

   Activity         m_activity;		// Current computer activity.
   WaitingState     m_waitingState;	// Current pause state.
   bool             m_waitingToMove;	// If true, AI paused or human to move.
   int              m_moveNo;		// Current move number, 0 = not started.
   int              m_endMoveNo;	// Winning move number, if game is over.
   bool             m_interrupting;	// If user has interrupted AI v. AI.
   bool             m_newSettings;	// If new player settings during a move.

   QWidget *        m_parent;		// Game object's parent (main window).
   KCubeBoxWidget * m_view;		// Displayed cubes.

   // A pointer to the Settings widget defined in Qt Designer, when the
   // Preferences dialog is first created by Game::showSettingsDialog().
   SettingsWidget * m_settingsPage;	// Displayed settings, 0 = not yet used.

   AI_Box *         m_box;		// Game engine's cubes.
   int              m_side;		// Cube box size, from Prefs::cubeDim().
   Player           m_currentPlayer;	// Current player: One or Two.
   QList<int> *     m_steps;		// Steps in a move to be displayed.

   AI_Main *        m_ai;		// Current computer player.

   int              m_index;		// Location of move.
   bool             m_fullSpeed;	// If true, no animation of moves.

   bool             computerPlOne;	// If true, Player 1 is an AI.
   bool             computerPlTwo;	// If true, Player 2 is an AI.

   bool             m_pauseForComputer;	// If true, pause before each AI move.
   bool             m_pauseForStep;	// If true, pause before animation step.

   QUrl             m_gameURL;		// Location of load/save file.

private Q_SLOTS:
   void   newGame();			// Slot needed for queued invocation.

private:
   void   saveGame (bool saveAs);	// Standard save-game action.
   void   loadGame();			// Standard load-game action.
   void   undo();			// Standard undo-move action.
   void   redo();			// Standard redo-move action.

   /**
    * Check if it is OK to start a new game, maybe ending a current game.
    */
   bool   newGameOK();

   /**
   * Reset cubebox for a new game.
   */
   void reset();

   /**
    * Undo or redo a move.
    *
    * @param   change  -1 = undo, +1 = redo
    *
    * @return  true = More moves to undo/redo, false = No more to undo/redo.
    */
   bool undoRedo (int change);

   /**
   * Set the number of cubes in a row or column.  If the number has changed,
   * delete the existing set of cubes and create a new one.
   */
   virtual void setDim (int dim);

   /**
    * Returns true if the player is a computer player.
    */
   bool isComputer (Player player) const;

   inline void restoreGame(const KConfigGroup&c) { readProperties(c); }
};

#endif // GAME_H
