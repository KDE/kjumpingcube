/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef KCUBEBOXWIDGET_H
#define KCUBEBOXWIDGET_H

#include <QSvgRenderer>

#include "ai_globals.h"
#include "kcubewidget.h"

#include <QWidget>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QList>

class QTimer;
class QLabel;

class KCubeBoxWidget : public QWidget
{
   Q_OBJECT
public:
   explicit KCubeBoxWidget (const int dim = 1, QWidget * parent = nullptr);

   virtual ~KCubeBoxWidget();

   void displayCube        (int index, Player owner, int value);
   void highlightCube      (int index, bool highlight);
   void timedCubeHighlight (int index);
   int  cubeValue          (int index) { return cubes.at(index)->value(); }

   /**
   * reset cubebox for a new game
   */
   void reset();

   /**
   * Set colors that are used to show owners of the cubes.
   */
   void setColors ();

   /**
   * Set the number of cubes in a row or column.  If the number has changed,
   * delete the existing set of cubes and create a new one.
   */
   virtual void setDim (int dim);

   void makeStatusPixmaps (const int width);
   const QPixmap & playerPixmap (const int p);

   /** sets the cursor to an waitcursor */
   void setWaitCursor();
   /** restores the original cursor */
   void setNormalCursor();

   bool loadSettings();

Q_SIGNALS:
   void animationDone (int index);
   void mouseClick (int x, int y);

protected:
   QSize sizeHint() const override;
   virtual void initCubes();
   void paintEvent (QPaintEvent * event) override;
   void resizeEvent (QResizeEvent * event) override;

private:
   enum AnimationType {None, ComputerMove, Darken, RapidBlink, Scatter};

   void init();

   QSvgRenderer svg;
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

   QPoint topLeft;
   int cubeSize;

   int      m_side;
   QList<KCubeWidget *> cubes;

   QTimer *animationTimer;

   int  m_index;
   AnimationType cascadeAnimation;
   AnimationType currentAnimation;
   int  animationCount;
   int  animationSteps;
   int  animationTime;

   QTimer * m_highlightTimer;	// Timer for highlighted cube.
   int  m_highlighted;		// Cube that has been highlighted.

   QLabel * m_popup;

public:
   /**
   * Starts the animation loop.
   */
   void startAnimation (bool cascading, int index);
   int killAnimation();

   void showPopup (const QString & message);
   void hidePopup();

private:
   void setPopup();
   void scatterDots (int step);

private Q_SLOTS:
   void nextAnimationStep();
   void highlightDone();	// Timeout of the highlighted cube.

   bool checkClick (int x, int y);
};

#endif // KCUBEBOXWIDGET_H
