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
#include "kcubewidget.h"

#include <QPainter>
#include <QTimer>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPixmap>
#include <QApplication>

#include <kdebug.h>

/* ****************************************************** **
**                 static elements                        **
** ****************************************************** */
bool KCubeWidget::_clicksAllowed=true;
QPalette KCubeWidget::color1;
QPalette KCubeWidget::color2;


void KCubeWidget::enableClicks(bool flag)
{
   _clicksAllowed=flag;
}


void KCubeWidget::setColor(Owner forWhom, QPalette newPalette)
{
   if(forWhom==One)
   {
      color1=newPalette;
   }
   else if(forWhom==Two)
   {
      color2=newPalette;
   }
}

QPalette KCubeWidget::color(Owner forWhom)
{
   QPalette color;
   if(forWhom==One)
   {
      color=color1;
   }
   else if(forWhom==Two)
   {
      color=color2;
   }

   return color;
}


/* ****************************************************** **
**                 public functions                       **
** ****************************************************** */

KCubeWidget::KCubeWidget(QWidget* parent,Owner owner,int value,int max)
              : QFrame(parent),
                Cube(owner,value,max)
{
  setMinimumSize (20,20);
  setFrameStyle(QFrame::Panel | QFrame::Raised);
  int h = height();
  int w = width();
  setLineWidth ((h<w?h:w) / 14); // Make QFrame::Raised width proportional.

  setCoordinates(0,0);

  //initialize hintTimer
  // will be automatically destroyed by the parent
  hintTimer = new QTimer(this);
  hintCounter=0;
  mRealMove = false;
  connect(hintTimer,SIGNAL(timeout()),SLOT(hint()));

  setPalette(QColor ("#b3925d")); // IDW Oxygen woodbrown2
  // setPalette(QColor ("#babdb6")); // IDW Oxygen aluminum gray3
  setAutoFillBackground (true);		// Allow ::hint() to work (Qt 4.1).

  // show values
  update();
}

KCubeWidget::~KCubeWidget()
{
}


KCubeWidget& KCubeWidget::operator=(const Cube& cube)
{
   if(this!=&cube)
   {
      setOwner(cube.owner());
      setValue(cube.value());
      setMax(cube.max());
   }

   return *this;
}

KCubeWidget& KCubeWidget::operator=(const KCubeWidget& cube)
{
   if(this!=&cube)
   {
      setOwner(cube.owner());
      setValue(cube.value());
      setMax(cube.max());
   }

   return *this;
}
KCubeWidget::Owner KCubeWidget::setOwner(Owner newOwner)
{
   Owner old=Cube::setOwner(newOwner);

   updateColors();

   return old;
}

void KCubeWidget::setValue(int newValue)
{
   Cube::setValue(newValue);
   update();
}


void KCubeWidget::showHint (int interval, int number, bool realMove)
{
   if(hintTimer->isActive())
      return;

   mRealMove = realMove;	// If not just a hint, must finish with a move.
   hintCounter=2*number;
   hint();			// Start the first blink.
   hintTimer->start(interval);	// Start the repeating timer.
}


void KCubeWidget::animate(bool )
{
}


void KCubeWidget::setCoordinates(int row,int column)
{
   _row=row;
   _column=column;
}

int KCubeWidget::row() const
{
   return _row;
}

int KCubeWidget::column() const
{
   return _column;
}




/* ****************************************************** **
**                   public slots                         **
** ****************************************************** */

void KCubeWidget::reset()
{
  setValue(1);
  setOwner(Nobody);
}


void KCubeWidget::updateColors()
{
  if(owner()==One)
    setPalette(color1);
  else if(owner()==Two)
    setPalette(color2);
  else if(owner()==Nobody)
     setPalette(QColor ("#b3925d")); // IDW Oxygen woodbrown2
     // setPalette(QColor ("#babdb6")); // IDW Oxygen aluminum gray3
}

void KCubeWidget::stopHint()
{
   if(hintTimer->isActive())
   {
      setBackgroundRole(QPalette::Window);
      hintTimer->stop();
      if (mRealMove) {
        // If it is a real move, not a hint, start animating the move for this
        // cube and updating the cube(s), using a simulated mouse click to
        // connect to KCubeBoxWidget::checkClicked (row(), column(), false).
        emit clicked (row(), column(), false);	// False --> not a mouse click.
        mRealMove = false;
      }
   }
}



/* ****************************************************** **
**                   protected slots                      **
** ****************************************************** */

void KCubeWidget::hint()
{
   if (hintCounter <= 0) {
       stopHint();
   }
   if (hintCounter%2 == 1) {
       setBackgroundRole (QPalette::Light);	// Blink light color.
   }
   else {
       setBackgroundRole (QPalette::Window);	// Blink normal color.
   }
   hintCounter--;
}



/* ****************************************************** **
**                   Event handler                        **
** ****************************************************** */

void KCubeWidget::mouseReleaseEvent(QMouseEvent *e)
{
  // only accept click if it was inside this cube
  if(e->x()< 0 || e->x() > width() || e->y() < 0 || e->y() > height())
    return;

  if(e->button() == Qt::LeftButton && _clicksAllowed)
  {
    stopHint();
    emit clicked(row(),column(),true);
  }
}



void KCubeWidget::paintEvent(QPaintEvent *ev)
{
  // Background is already painted (see "setAutoFillBackground (true)").
  // Resizing has been done by stretch options in KCubeBoxWidget.

  int h = height();
  int w = width();
  setLineWidth ((h<w?h:w) / 14); // Make QFrame::Raised width proportional.
  QFrame::paintEvent(ev);

  QPainter p(this);

  int circleSize=(h<w?h:w)/7;
  int points=value();

  p.setBrush(Qt::black);
  p.setPen(Qt::black);
  switch(points)
    {
    case 1:
      p.drawEllipse(w/2-circleSize/2,h/2-circleSize/2,circleSize,circleSize);
      break;

    case 3:
      p.drawEllipse(w/2-circleSize/2,h/2-circleSize/2,circleSize,circleSize);
    case 2:
      p.drawEllipse(w/4-circleSize/2,h/4-circleSize/2,circleSize,circleSize);
      p.drawEllipse(3*w/4-circleSize/2,3*h/4-circleSize/2,
		     circleSize,circleSize);
      break;

    case 5:
      p.drawEllipse(w/2-circleSize/2,h/2-circleSize/2,circleSize,circleSize);
    case 4:
      p.drawEllipse(w/4-circleSize/2,h/4-circleSize/2,circleSize,circleSize);
      p.drawEllipse(3*w/4-circleSize/2,h/4-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(w/4-circleSize/2,3*h/4-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(3*w/4-circleSize/2,3*h/4-circleSize/2,
		     circleSize,circleSize);
      break;

    case 8:
      p.drawEllipse(w/2-circleSize/2,2*h/3-circleSize/2,
		     circleSize,circleSize);
    case 7:
      p.drawEllipse(w/2-circleSize/2,h/3-circleSize/2,
		     circleSize,circleSize);
    case 6:
      p.drawEllipse(w/4-circleSize/2,h/6-circleSize/2,circleSize,circleSize);
      p.drawEllipse(3*w/4-circleSize/2,h/6-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(w/4-circleSize/2,3*h/6-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(3*w/4-circleSize/2,3*h/6-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(w/4-circleSize/2,5*h/6-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(3*w/4-circleSize/2,5*h/6-circleSize/2,
		     circleSize,circleSize);
      break;


    case 9:
      p.drawEllipse(w/4-circleSize/2,h/6-circleSize/2,circleSize,circleSize);
      p.drawEllipse(3*w/4-circleSize/2,h/6-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(w/4-circleSize/2,3*h/6-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(3*w/4-circleSize/2,3*h/6-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(w/4-circleSize/2,5*h/6-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(3*w/4-circleSize/2,5*h/6-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(w/2-circleSize/2,2*h/7-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(w/2-circleSize/2,5*h/7-circleSize/2,
		     circleSize,circleSize);
      p.drawEllipse(w/2-circleSize/2,h/2-circleSize/2,
		     circleSize,circleSize);
      break;

    default:
      kDebug() << "cube had value " << points << endl;
      QString s;
      s.sprintf("%d",points);
      p.drawText(w/2,h/2,s);
      break;
    }
   p.end();
}

#include "kcubewidget.moc"
