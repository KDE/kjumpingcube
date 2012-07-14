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
#include "cube.h"

class QMouseEvent;
class QPaintEvent;

/**
* 
*/
class KCubeWidget : public QFrame , public Cube 
{
   Q_OBJECT
         
public:
   /** constructs a new KCubeWidget*/
   explicit KCubeWidget(QWidget* parent=0,
                           Owner owner=Cube::Nobody, int value=1, int max=0);  
   virtual ~KCubeWidget();   
   
   void setPixmaps (QList<QPixmap> * ptr);
   virtual Owner setOwner(Owner newOwner); 
   virtual void setValue(int newValue);
   
   
   /** takes the information from a Cube */
   KCubeWidget& operator=(const Cube&);
   KCubeWidget& operator=(const KCubeWidget&);
   
   void migrateDot  (int moveDelay, int fromRow, int fromCol);

   /** 
   * sets the coordinates of the Cube in a Cubebox;
   * needed for identification when clicked.
   */ 
   void setCoordinates (int row, int col, int limit);
   /** returns the row */
   int row() const;
   /** returns the column */
   int column() const;
   
   /** enables or disables possibility to click a cube*/
   static void enableClicks(bool flag);

   void setLight()   { blinking = Light; update(); }
   void setDark()    { blinking = Dark; update(); }
   void setNeutral() { blinking = None; update(); }
   bool isNeutral()  { return (blinking == None); }

   void shrink (qreal scale);
   void expand (qreal scale);
   void migrateDot (int rowDiff, int colDiff, qreal scale);

public slots:
   /** resets the Cube to default values */
   virtual void reset();
   /** shows changed colors*/
   virtual void updateColors();
      
signals:
   void clicked(int row,int column,bool isClick);
   
protected:
   /** checks, if mouseclick was inside this cube*/
   virtual void mouseReleaseEvent(QMouseEvent*);
   
   /** refreshes the contents of the Cube */
   virtual void paintEvent(QPaintEvent*);

private:
   int m_row;
   int m_col;
   int m_limit;
   QList<QPixmap> * pixmaps;
   enum Blink {None, Light, Dark};
   Blink blinking;

   static bool _clicksAllowed;
   int    migrating;
   qreal  m_rowDiff;
   qreal  m_colDiff;
   double m_scale;
};

#endif // KCUBEWIDGET_H
