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

#include "cubeboxbase.h"
#include "kcubewidget.h"
#include "brain.h"
#include <QWidget>
//Added by qt3to4:
#include <QGridLayout>
#include <QPaintEvent>
#include <QResizeEvent>

class KConfigGroup;
class QGridLayout;
class CubeBox;
class QTimer;

/**
*@internal
*/
struct Loop
{
   int row;
   int column;
   bool finished;
};


class KCubeBoxWidget : public QWidget , public CubeBoxBase<KCubeWidget>
{
   Q_OBJECT
public:
   explicit KCubeBoxWidget(const int dim=1,QWidget *parent=0);

   explicit KCubeBoxWidget(CubeBox& box, QWidget *parent=0);
   KCubeBoxWidget(const KCubeBoxWidget& box,QWidget *parent=0);
   virtual ~KCubeBoxWidget();

   KCubeBoxWidget& operator= (CubeBox& box);
   KCubeBoxWidget& operator= ( const KCubeBoxWidget& box);

   /**
   * reset cubebox for a new game
   */
   void reset();

   /** undo last move */
   void undo();

   /**
   * Set colors that are used to show owners of the cubes.
   */
   void setColors ();

   /**
   * sets number of Cubes in a row/column to 'size'.
   */
   virtual void setDim(int dim);

   /**
   * sets player 'player' as computer or human
   *
   * @param player
   * @param flag: true for computer, false for human
   */
   void setComputerplayer(Player player,bool flag);

   /** returns current skill, according to Prefs::EnumSkill */
   int skill() const;

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

signals:
   void playerChanged(int newPlayer);
   void colorChanged(int player);
   void playerWon(int player);
   void startedMoving();
   void startedThinking();
   void stoppedMoving();
   void stoppedThinking();
   void dimensionsChanged();

protected:
   virtual QSize sizeHint() const;
   virtual void deleteCubes();
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
   CubeBox *undoBox;
   Brain brain;

   QTimer *moveTimer;
   int moveDelay;
   Loop loop;
   /** */
   void startLoop();
   /** */
   void stopLoop();

   Player changePlayer();
   bool hasPlayerWon(Player player);
   bool computerPlOne;
   bool computerPlTwo;

   /**
   * increases the cube at row 'row' and column 'column' ,
   * and starts the Loop for checking the playingfield
   */
   void doMove(int row,int column);

   void increaseNeighbours(KCubeBoxWidget::Player forWhom,int row,int column);

private slots:
   void nextLoopStep();
   /**
   * checks if cube at ['row','column'] is clickable by the current player.
   * if true, it increases this cube and checks the playingfield
   */
   bool checkClick(int row,int column,bool isClick);

   /** turns off blinking, if an other cube is clicked */
   void stopHint();

};

#endif // KCUBEBOXWIDGET_H

