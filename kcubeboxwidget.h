/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 1998-2000 by Matthias Kiefer
                            <matthias.kiefer@gmx.de>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

**************************************************************************** */
#ifndef KCUBEBOXWIDGET_H
#define KCUBEBOXWIDGET_H

#include <QSvgRenderer>
#include <QTime> // IDW

#include "kcubewidget.h"

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QList>

class KConfigGroup;
class AI_Main;
class AI_Box;
class QTimer;
class QLabel;

class KCubeBoxWidget : public QWidget
{
   Q_OBJECT
public:
   enum AnimationType {None, Hint, ComputerMove, Darken, RapidBlink, Scatter};

   explicit KCubeBoxWidget(const int dim=1,QWidget *parent=0);

   virtual ~KCubeBoxWidget();

   /**
    * Make sure all animation and AI activity is over before destroying widget.
    * */
   void shutdown();

   /**
   * reset cubebox for a new game
   */
   void reset();

   /**
    * Undo or redo a move.
    *
    * @param   actionType  -1 = undo, +1 = redo
    *
    * @return  -1 = Busy, 0 = No more to undo/redo, 1 = More moves to undo/redo.
    */
   int undoRedo (int actionType);

   /**
   * Set colors that are used to show owners of the cubes.
   */
   void setColors ();

   /**
   * Set the number of cubes in a row or column.  If the number has changed,
   * delete the existing set of cubes and create a new one.
   */
   virtual void setDim (int dim);

   /**
   * sets player 'player' as computer or human
   *
   * @param player
   * @param flag: true for computer, false for human
   */
   void setComputerplayer(Player player,bool flag);

   /** returns true if player 'player' is a computerPlayer */
   bool isComputer(Player player) const;

   /** returns true if CubeBox is doing a move or getting a hint */
   bool isActive() const;
   bool isMoving() const;

   /**
   * checks if 'player' is a computerplayer an computes next move if TRUE
   */
   void checkComputerplayer(Player player);

   inline void saveGame(KConfigGroup&c) { saveProperties(c); }
   inline void restoreGame(const KConfigGroup&c) { readProperties(c); }

   void makeStatusPixmaps (const int width);
   const QPixmap & playerPixmap (const int p);

public slots:
   /** stops all activities like getting a hint or doing a move */
   void stopActivities();
   /**
    * computes a possibility to move and shows it by highlightning
    * this cube
    */
   void getHint();

   void loadSettings();

   void computerMoveDone (int index);

   void buttonClick();

signals:
   void playerChanged(int newPlayer);
   void colorChanged(int player);
   void newMove();
   void playerWon(int player);
   void startedMoving();
   void startedThinking();
   void stoppedMoving();
   void stoppedThinking();
   void dimensionsChanged();
   void buttonChange(bool enabled, bool stop = false,
                     const QString & caption = QString(""));

protected:
   virtual QSize sizeHint() const;
   virtual void initCubes();
   virtual void paintEvent (QPaintEvent * event);
   virtual void resizeEvent (QResizeEvent * event);

   void saveProperties(KConfigGroup&);
   void readProperties(const KConfigGroup&);

protected slots:
   /** sets the cursor to an waitcursor */
   void setWaitCursor();
   /** restores the original cursor */
   void setNormalCursor();

private:
   void init();

   QSvgRenderer svg;
   QTime t; // IDW
   void makeSVGBackground (const int w, const int h);
   void makeSVGCubes (const int width);
   void colorImage (QImage & img, const QColor & c, const int w);
   void reCalculateGraphics (const int w, const int h);

   enum State        {NotStarted, HumanToMove, ComputingMove, Busy, Waiting};
   enum BusyState    {NA, ComputingHint, ShowingHint, ShowingMove,
                      AnimatingMove};
   enum WaitingState {Nil, WaitingToUndo, WaitingToLoad, WaitingToSave,
                      WaitingToStep, ComputerToMove};

   State              m_state;
   BusyState          m_busyState;
   WaitingState       m_waitingState;

   int sWidth;			// Width of status pixmaps (used if recoloring).
   QPixmap status1;		// Status-bar pixmaps for players 1 and 2.
   QPixmap status2;
   QPixmap background;		// Pixmap for background.
   QList<QPixmap> elements;	// Pixmaps for cubes, pips and blinking.
   QColor color1;		// Player 1's color.
   QColor color2;		// Player 2's color.
   QColor color0;		// Color for neutral cubes.
   bool drawHairlines;		// T = draw hairlines between cubes, F = do not.

   QPoint topLeft;
   int cubeSize;

   AI_Box * m_box;
   int      m_side;
   Player   m_currentPlayer;
   QList<KCubeWidget *> cubes;
   bool     m_gameHasBeenWon;
   QList<int> * m_steps;

   AI_Main * m_ai;

   QTimer *animationTimer;

   int  m_index;
   AnimationType cascadeAnimation;
   AnimationType currentAnimation;
   // IDW DELETE. AnimationType m_computerMoveType;
   int  animationCount;
   int  animationSteps;
   int  animationTime;

   // IDW test. Moved to slot section. Player changePlayer();
   bool   computerPlOne;
   bool   computerPlTwo;

   bool   m_pauseForComputer;
   Player m_playerWaiting;
   bool   m_pauseForStep;
   bool   m_waitingForStep;
   QLabel * m_popup;
   bool   m_ignoreComputerMove;

   /**
   * Increases the cube at 'index' and starts the animation loop, if required.
   */
   void doMove (int index);
   void doStep();
   void startAnimation (AnimationType type, int index);
   void scatterDots (int step);
   void stopAnimation (bool completeAllSteps);

   Player changePlayer();

   void showPopup (const QString & message);
   void hidePopup();

private slots:
   void nextAnimationStep();

   /**
   * checks if cube at [x, y] is clickable by the current player.
   * if true, it increases this cube and checks the playingfield
   */
   bool checkClick (int x, int y, bool isClick);
};

#endif // KCUBEBOXWIDGET_H
