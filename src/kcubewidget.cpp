/*
    This file is part of the game 'KJumpingCube'

    SPDX-FileCopyrightText: 1998-2000 Matthias Kiefer <matthias.kiefer@gmx.de>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "kcubewidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPixmap>

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

KCubeWidget::KCubeWidget (QWidget* parent)
              : QFrame(parent)
{
  setMinimumSize (20,20);
  setFrameStyle(QFrame::Panel | QFrame::Raised);
  int h = height();
  int w = width();
  setLineWidth ((h<w?h:w) / 14); // Make QFrame::Raised width proportional.

  setCoordinates (0, 0, 2);

  migrating = 0;
  m_scale = 1.0;
  m_row = 0;
  m_col = 0;
  m_owner = Nobody;
  m_value = 1;

  pixmaps = nullptr;
  blinking = None;

  // show values
  update();
}

KCubeWidget::~KCubeWidget()
{
}

void KCubeWidget::setPixmaps (QList<QPixmap> * ptr)
{
   pixmaps = ptr;
}

void KCubeWidget::setOwner (Player newOwner)
{
   if (newOwner != m_owner) {
      m_owner = newOwner;
      updateColors();
   }
}

void KCubeWidget::setValue(int newValue)
{
   if (newValue != m_value) {
      m_value = newValue;
      update();
   }
}

void KCubeWidget::shrink (qreal scale)
{
   migrating = 0;
   m_scale = scale;
   update();
}

void KCubeWidget::expand (qreal scale)
{
   migrating = 1;
   m_scale = scale;
   blinking = None;			// Remove overloaded cube's dark color.
   update();
}

void KCubeWidget::migrateDot (int rowDiff, int colDiff, int step, Player player)
{
   migrating = 2;
   qreal scale = (step < 4) ? 1.0 - 0.3 * step : 0.0;
   m_rowDiff = rowDiff * scale;		// Calculate relative position of dot.
   m_colDiff = colDiff * scale;
   // If owner changes, fade in new color as dot approaches centre of cube.
   m_player  = player;
   m_opacity = (step < 4) ? 0.2 * (step + 1) : 1.0;
   update();
}

void KCubeWidget::setCoordinates (int row, int col, int limit)
{
   m_row = row;
   m_col = col;
   m_limit = limit;
}

/* ****************************************************** **
**                   public slots                         **
** ****************************************************** */

void KCubeWidget::reset()
{
  blinking = None;
  setValue (1);
  setOwner (Nobody);
  update();
}


void KCubeWidget::updateColors()
{
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

  if(e->button() == Qt::LeftButton && _clicksAllowed) {
    e->accept();
    Q_EMIT clicked (m_row, m_col);
  }
}

void KCubeWidget::paintEvent(QPaintEvent * /* ev unused */)
{
  if ((pixmaps == nullptr) || (pixmaps->isEmpty()))
      return;

  int width  = this->width();
  int height = this->height();

  QPainter p(this);

  SVGElement el = Neutral;
  if (owner() == One)
    el = Player1;
  else if (owner() == Two)
    el = Player2;

  // if ((migrating == 2) && (m_player != owner()))	// && (m_scale < 0.5))
    // el = m_element;

  int pmw = pixmaps->at(el).width();
  int pmh = pixmaps->at(el).height();
  p.drawPixmap ((width - pmw)/2, (height - pmh)/2, pixmaps->at(el));
  if ((migrating == 2) && (m_player != owner())) {
      el = (m_player == One) ? Player1 : Player2;
      p.setOpacity (m_opacity);	// Cube is being captured: fade in new color.
      p.drawPixmap ((width - pmw)/2, (height - pmh)/2, pixmaps->at(el));
      p.setOpacity (1.0);
  }

  QPixmap pip = pixmaps->at(Pip);
  int dia = pip.width();

  // Normally scale = 1.0, but it will be less during the first part of an
  // animation that shows a cube taking over its neighboring cubes.

  int w   = m_scale * width;	// The size of the pattern of pips.
  int h   = m_scale * height;
  int cx  = width/2;		// The center point of the cube face.
  int cy  = height/2;
  int tlx = (width - w) / 2;	// The top left corner of the pattern of pips.
  int tly = (height - h) / 2;

  int points = (migrating == 1) ? 0 : value();
  if (migrating == 2) {
      int dRow = m_rowDiff * width / 2;
      int dCol = m_colDiff * height / 2;
      p.drawPixmap (cx + dRow - dia/2, cy + dCol - dia/2, pip);
  }

  switch (points) {
  case 0:
      // Show the pattern of pips migrating to neighboring cubes: 
      // one pip in the center and one migrating to each neighbor.
      p.drawPixmap    (cx - dia/2,          cy - dia/2,           pip);
      if (m_scale > 1.0) {	// The migrating dots have all left this cube.
         break;
      }
      if (m_row > 0)		// Neighbor above, if any.
         p.drawPixmap (tlx - dia/2,         cy - dia/2,           pip);
      if (m_row < m_limit)	// Neighbor below, if any.
         p.drawPixmap (width - tlx - dia/2, cy - dia/2,           pip);
      if (m_col > 0)		// Neighbor to left, if any.
         p.drawPixmap (cx - dia/2,          tly - dia/2,          pip);
      if (m_col < m_limit)	// Neighbor to right, if any.
         p.drawPixmap (cx - dia/2,          height - tly - dia/2, pip);
      break;

      // Otherwise show a pattern for the current number of pips. It may be
      // scaled down during the first part of an animation that shows a cube
      // taking over its neighboring cubes.
  case 1:
      p.drawPixmap (tlx + (w - dia)/2, tly + (h - dia)/2, pip);
      break;

  case 3:
      p.drawPixmap (tlx + (w - dia)/2, tly + (h - dia)/2, pip);
  case 2:
      p.drawPixmap (tlx + (w/2 - dia)/2, tly + (h/2 - dia)/2, pip);
      p.drawPixmap (tlx + (3*w/2 - dia)/2, tly + (3*h/2 - dia)/2, pip);
      break;

  case 5:
      p.drawPixmap (tlx + (w - dia)/2, tly + (h - dia)/2, pip);
  case 4:
      p.drawPixmap (tlx + (w/2 - dia)/2,   tly + (h/2 - dia)/2, pip);
      p.drawPixmap (tlx + (w/2 - dia)/2,   tly + (3*h/2 - dia)/2, pip);
      p.drawPixmap (tlx + (3*w/2 - dia)/2, tly + (h/2 - dia)/2, pip);
      p.drawPixmap (tlx + (3*w/2 - dia)/2, tly + (3*h/2 - dia)/2, pip);
      break;

  case 8:
      p.drawPixmap (tlx + (w - dia)/2,     tly + 2*h/3 - dia/2, pip);
  case 7:
      p.drawPixmap (tlx + (w - dia)/2,     tly + h/3 - dia/2, pip);
  case 6:
      p.drawPixmap (tlx + (w/2 - dia)/2,   tly + (h/2 - dia)/2, pip);
      p.drawPixmap (tlx + (w/2 - dia)/2,   tly + (h - dia)/2, pip);
      p.drawPixmap (tlx + (w/2 - dia)/2,   tly + (3*h/2 - dia)/2, pip);
      p.drawPixmap (tlx + (3*w/2 - dia)/2, tly + (h/2 - dia)/2, pip);
      p.drawPixmap (tlx + (3*w/2 - dia)/2, tly + (h - dia)/2, pip);
      p.drawPixmap (tlx + (3*w/2 - dia)/2, tly + (3*h/2 - dia)/2, pip);
      break;

  default:
      QString s = QString::asprintf("%d",points);
      p.setPen(Qt::black);
      p.drawText(tlx + w/2,tly + h/2,s);
      break;
  }

  // This is used to highlight a cube and also to perform the hint animation.
  switch (blinking) {
  case Light:
      p.drawPixmap ((width - pmw)/2, (height - pmh)/2, pixmaps->at(BlinkLight));
      break;
  case Dark:
      p.drawPixmap ((width - pmw)/2, (height - pmh)/2, pixmaps->at(BlinkDark));
      break;
  default:
      break;
  }
  migrating = 0;
  m_scale = 1.0;

  p.end();
}


