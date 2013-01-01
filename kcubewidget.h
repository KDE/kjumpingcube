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
#ifndef KCUBEWIDGET_H
#define KCUBEWIDGET_H

#include <QFrame>
#include "ai_globals.h"

enum SVGElement {Neutral, Player1, Player2, Pip, BlinkLight, BlinkDark,
                 FirstElement = Neutral, LastElement = BlinkDark};

class QMouseEvent;
class QPaintEvent;

/**
* 
*/
class KCubeWidget : public QFrame
{
   Q_OBJECT
         
public:
   /** constructs a new KCubeWidget*/
   explicit KCubeWidget (QWidget * parent = 0);
   virtual ~KCubeWidget();

   Player owner() { return m_owner; }
   int    value() { return m_value; }

   void setOwner   (Player newOwner); 
   void setValue   (int newValue);
   void setPixmaps (QList<QPixmap> * ptr);

   /** 
   * sets the coordinates of the Cube in a Cubebox;
   * needed for identification when clicked.
   */ 
   void setCoordinates (int row, int col, int limit);

   /** enables or disables possibility to click a cube*/
   static void enableClicks(bool flag);

   void setLight()   { blinking = Light; update(); }
   void setDark()    { blinking = Dark; update(); }
   void setNeutral() { blinking = None; update(); }
   bool isNeutral()  { return (blinking == None); }

   void shrink (qreal scale);
   void expand (qreal scale);
   void migrateDot (int rowDiff, int colDiff, int step, Player player);

public slots:
   /** resets the Cube to default values */
   virtual void reset();
   /** shows changed colors*/
   virtual void updateColors();

signals:
   void clicked (int row, int column);

protected:
   /** checks, if mouseclick was inside this cube*/
   virtual void mouseReleaseEvent(QMouseEvent*);

   /** refreshes the contents of the Cube */
   virtual void paintEvent(QPaintEvent*);

private:
   int m_row;
   int m_col;
   int m_limit;
   Player m_owner;
   int m_value;

   QList<QPixmap> * pixmaps;
   enum Blink {None, Light, Dark};
   Blink blinking;

   static bool _clicksAllowed;
   int   migrating;
   qreal m_rowDiff;
   qreal m_colDiff;
   Player m_player;
   qreal m_scale;
   qreal m_opacity;
};

#endif // KCUBEWIDGET_H
