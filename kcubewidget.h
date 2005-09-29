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

#include <qframe.h>
#include "cube.h"

class QPalette;
class QTimer;


/**
* 
*/
class KCubeWidget : public QFrame , public Cube 
{
   Q_OBJECT
         
public:
   /** constructs a new KCubeWidget*/
   KCubeWidget(QWidget* parent=0,const char* name=0
                    ,Owner owner=Cube::Nobody,int value=1,int max=0);  
   virtual ~KCubeWidget();   
   
   virtual Owner setOwner(Owner newOwner); 
   virtual void setValue(int newValue);
   
   
   /** takes the information from a Cube */
   KCubeWidget& operator=(const Cube&);
   KCubeWidget& operator=(const KCubeWidget&);
   
   /** shows a tip e.g. blinks with the interval 500 and number times */
   void showHint(int interval=500,int number=5);
   /** stops showing a hint */
   void stopHint();
   
   /** 
   * animates the cube if possible (if feature is enabled)
   * In KCubeWidget this function does nothing, it's just for having
   * a public interface for all classes that inherits KCubeWidget 
   */
   virtual void animate(bool flag);
   
   /** 
   * sets the coordinates of the Cube in a Cubebox;
   * needed for identification when clicked.
   */ 
   void setCoordinates(int row,int column);
   /** returns the row */
   int row() const;
   /** returns the column */
   int column() const;
   
   /** enables or disables possibility to click a cube*/
   static void enableClicks(bool flag);
   static void setColor(Owner forWhom, QPalette newPalette); 
   static QPalette color(Owner forWhom);
   
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
   
   /** refreshs the contents of the Cube */
   virtual void drawContents(QPainter*);
 
   
  
   int hintCounter;
   
protected slots:
   /** 
   * To this function the hintTimer is connected. 
   * It manage a periodical way of showing a hint.
   */
   virtual void hint(); 

private:
   int _row;
   int _column;
   
   QTimer *hintTimer;
   
   static bool _clicksAllowed;
   static QPalette color1;
   static QPalette color2;
};


#endif // KCUBEWIDGET_H
