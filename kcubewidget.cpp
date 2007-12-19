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

#include <kdebug.h>

/* ****************************************************** **
**                 static elements                        **
** ****************************************************** */
bool KCubeWidget::_clicksAllowed=true;

void KCubeWidget::enableClicks(bool flag)
{
   _clicksAllowed=flag;
}

/* ****************************************************** **
**                 public functions                       **
** ****************************************************** */

KCubeWidget::KCubeWidget(QWidget* parent, Owner owner, int value, int max)
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

  pixmaps = 0;
  blinking = None;

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

void KCubeWidget::setPixmaps (QList<QPixmap> * ptr)
{
   pixmaps = ptr;
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
  update();
}

void KCubeWidget::stopHint()
{
   if(hintTimer->isActive())
   {
      hintTimer->stop();
      blinking = None;		// Turn off blinking.
      update();

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
       return;
   }
   if (hintCounter%2 == 0) {
       blinking = Light;	// Blink light color.
   }
   else {
       blinking = Dark;		// Blink dark color.
   }
   hintCounter--;
   update();
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



void KCubeWidget::paintEvent(QPaintEvent * /* ev unused */)
{
  if ((pixmaps == 0) || (pixmaps->isEmpty()))
      return;

  int h = height();
  int w = width();

  QPainter p(this);
  QPixmap pip = pixmaps->at(Pip);

  SVGElement el = Neutral;
  if (owner() == One)
    el = Player1;
  else if (owner() == Two)
    el = Player2;

  int dia = pip.width();
  int pmw = pixmaps->at(el).width();
  int pmh = pixmaps->at(el).height();
  p.drawPixmap ((w - pmw)/2, (h - pmh)/2, pixmaps->at(el));

  int points = value();

  switch (points) {
  case 1:
      p.drawPixmap ((w - dia)/2, (h - dia)/2, pip);
      break;

  case 3:
      p.drawPixmap ((w - dia)/2, (h - dia)/2, pip);
  case 2:
      p.drawPixmap ((w/2 - dia)/2, (h/2 - dia)/2, pip);
      p.drawPixmap ((3*w/2 - dia)/2, (3*h/2 - dia)/2, pip);
      break;

  case 5:
      p.drawPixmap ((w - dia)/2, (h - dia)/2, pip);
  case 4:
      p.drawPixmap ((w/2 - dia)/2,   (h/2 - dia)/2, pip);
      p.drawPixmap ((w/2 - dia)/2,   (3*h/2 - dia)/2, pip);
      p.drawPixmap ((3*w/2 - dia)/2, (h/2 - dia)/2, pip);
      p.drawPixmap ((3*w/2 - dia)/2, (3*h/2 - dia)/2, pip);
      break;

  case 8:
      p.drawPixmap ((w - dia)/2,     2*h/3 - dia/2, pip);
  case 7:
      p.drawPixmap ((w - dia)/2,     h/3 - dia/2, pip);
  case 6:
      p.drawPixmap ((w/2 - dia)/2,   (h/2 - dia)/2, pip);
      p.drawPixmap ((w/2 - dia)/2,   (h - dia)/2, pip);
      p.drawPixmap ((w/2 - dia)/2,   (3*h/2 - dia)/2, pip);
      p.drawPixmap ((3*w/2 - dia)/2, (h/2 - dia)/2, pip);
      p.drawPixmap ((3*w/2 - dia)/2, (h - dia)/2, pip);
      p.drawPixmap ((3*w/2 - dia)/2, (3*h/2 - dia)/2, pip);
      break;

  default:
      QString s;
      s.sprintf("%d",points);
      p.setPen(Qt::black);
      p.drawText(w/2,h/2,s);
      break;
  }

  switch (blinking) {
  case Light:
      p.drawPixmap ((w - pmw)/2, (h - pmh)/2, pixmaps->at(BlinkLight));
      break;
  case Dark:
      p.drawPixmap ((w - pmw)/2, (h - pmh)/2, pixmaps->at(BlinkDark));
      break;
  default:
      break;
  }

  p.end();
}

#include "kcubewidget.moc"
