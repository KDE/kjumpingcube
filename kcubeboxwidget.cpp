/* ****************************************************************************
  This file is part of the game 'KJumpingCube'

  Copyright (C) 1998-2000 by Matthias Kiefer <matthias.kiefer@gmx.de>
  Copyright (C) 2012-2013 by Ian Wadhan      <iandw.au@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************** */

#include "kcubeboxwidget.h"

#include <KgTheme>
#include <KStandardDirs>
#include <KLocalizedString>
#include <KMessageBox>
#include <QTimer>
#include <QLabel>
#include <QPainter>

#include <assert.h>
#include <kcursor.h>

#include "prefs.h"

KCubeBoxWidget::KCubeBoxWidget (const int d, QWidget *parent)
        : QWidget (parent),
	  m_side          (d),
	  m_popup         (new QLabel (this))
{
   qDebug() << "CONSTRUCT KCubeBoxWidget: side" << m_side;
   cubes.clear();
   init();
}

KCubeBoxWidget::~KCubeBoxWidget()
{
}

bool KCubeBoxWidget::loadSettings()
{
  qDebug() << "LOAD VIEW SETTINGS";
  bool reColorCubes = ((color1 != Prefs::color1()) ||
                       (color2 != Prefs::color2()) ||
                       (color0 != Prefs::color0()));

  color1 = Prefs::color1();
  color2 = Prefs::color2();
  color0 = Prefs::color0();

  if (Prefs::animationNone()) {
     cascadeAnimation = None;
  }
  else if (Prefs::animationDelay() || (Prefs::animationSpeed() <= 1)) {
     cascadeAnimation = Darken;
  }
  else if (Prefs::animationBlink()) {
     cascadeAnimation = RapidBlink;
  }
  else if (Prefs::animationSpread()) {
     cascadeAnimation = Scatter;
  }

  animationTime = Prefs::animationSpeed() * 150;

  // NOTE: When the box-size (Prefs::cubeDim()) changes, Game::newGame() calls
  //       KCubeBoxWidget::loadSettings() first, then KCubeBoxWidget::setDim(). 

  if (reColorCubes) {
     makeStatusPixmaps (sWidth);		// Make new status pixmaps.
     makeSVGCubes (cubeSize);
     setColors ();
  }
  return reColorCubes;
}

void KCubeBoxWidget::reset()
{
   foreach (KCubeWidget * cube, cubes) {
      cube->reset();
   }

   KCubeWidget::enableClicks(true);
   currentAnimation = None;
}

void KCubeBoxWidget::displayCube (int index, Player owner, int value)
{
   cubes.at(index)->setOwner (owner);
   cubes.at(index)->setValue (value);
}

void KCubeBoxWidget::highlightCube (int index, bool highlight)
{
   if (highlight) {
      cubes.at(index)->setDark();
   }
   else {
      cubes.at(index)->setNeutral();
   }
}

void KCubeBoxWidget::setColors ()
{
   foreach (KCubeWidget * cube, cubes) {
      cube->updateColors();
   }
}

void KCubeBoxWidget::setDim(int d)
{
   if (d != m_side) {
      m_side  = d;
      initCubes();
      reCalculateGraphics (width(), height());
      reset();
   }
}

/* ***************************************************************** **
**                               slots                               **
** ***************************************************************** */

void KCubeBoxWidget::setWaitCursor()
{
   setCursor (Qt::BusyCursor);
}

void KCubeBoxWidget::setNormalCursor()
{
   setCursor (Qt::PointingHandCursor);
}

bool KCubeBoxWidget::checkClick (int x, int y)
{
   /* IDW TODO - Remove this from the view OR rewrite it as a MouseEvent().
    *
   // IDW TODO - Write a new mouse-click event for KCubeBoxWidget? Remove the
   //            one that KCubeWidget has?
   */
   qDebug() << "Emit mouseClick (" << x << y << ")";
   emit mouseClick (x, y);
   return false;
}

/* ***************************************************************** **
**                   initializing functions                          **
** ***************************************************************** */
void KCubeBoxWidget::init()
{
   currentAnimation = None;
   animationSteps = 12;
   animationCount = 0;

   setMinimumSize (200, 200);
   color1 = Prefs::color1();			// Set preferred colors.
   color2 = Prefs::color2();
   color0 = Prefs::color0();

   KgTheme theme((QByteArray()));
   theme.readFromDesktopFile(KStandardDirs::locate("appdata",
                                                   "pics/default.desktop"));
   svg.load (theme.graphicsPath());
   drawHairlines = (theme.customData("DrawHairlines") == "0") ? false : true;

   initCubes();

   animationTime = Prefs::animationSpeed() * 150;
   animationTimer = new QTimer(this);

   connect (animationTimer, SIGNAL(timeout()), SLOT(nextAnimationStep()));
   setNormalCursor();
   setPopup();
}

void KCubeBoxWidget::initCubes()
{
   qDeleteAll (cubes);
   cubes.clear();

   int nCubes = m_side * m_side;
   for (int n = 0; n < nCubes; n++) {
      KCubeWidget * cube = new KCubeWidget (this);
      cubes.append (cube);
      cube->setCoordinates (n / m_side, n % m_side, m_side - 1);
      cube->setPixmaps (&elements);
      connect (cube, SIGNAL (clicked(int,int)),
                     SLOT   (checkClick(int,int)));
      cube->show();
   }
}

void KCubeBoxWidget::makeStatusPixmaps (const int width)
{
   qreal d, p;
   QImage status (width, width, QImage::Format_ARGB32_Premultiplied);
   QPainter s (&status);
   sWidth = width;

   d = width/4.0;
   p = width/2.0;
   status.fill (0);
   svg.render (&s, "player_1");
   colorImage (status, color1, width);
   svg.render (&s, "lighting");
   svg.render (&s, "pip", QRectF (p - d/2.0, p - d/2.0, d, d));
   status1 = QPixmap::fromImage (status);

   d = width/5.0;
   p = width/3.0;
   status.fill (0);
   svg.render (&s, "player_2");
   colorImage (status, color2, width);
   svg.render (&s, "lighting");
   svg.render (&s, "pip", QRectF (p - d/2.0, p - d/2.0, d, d));
   svg.render (&s, "pip", QRectF (p + p - d/2.0, p + p - d/2.0, d, d));
   s.end();
   status2 = QPixmap::fromImage (status);
}

void KCubeBoxWidget::makeSVGBackground (const int w, const int h)
{
   qDebug() << t.restart() << "msec";
   QImage img (w, h, QImage::Format_ARGB32_Premultiplied);
   QPainter p (&img);
   img.fill (0);
   svg.render (&p, "background");
   p.end();
   background = QPixmap::fromImage (img);
   qDebug() << t.restart() << "msec" << "SVG background rendered";
}

void KCubeBoxWidget::makeSVGCubes (const int width)
{
   qDebug() << t.restart() << "msec";
   QImage img (width, width, QImage::Format_ARGB32_Premultiplied);
   QPainter q;                 // Paints whole faces of the dice.

   QImage pip (width/7, width/7, QImage::Format_ARGB32_Premultiplied);
   QPainter r;                 // Paints the pips on the faces of the dice.

   QRectF rect (0, 0, width, width);
   qreal  pc = 20.0;		// % radius on corners.
   elements.clear();
   for (int i = FirstElement; i <= LastElement; i++) {
     q.begin(&img);
     q.setPen (Qt::NoPen);
     if (i == Pip) {
       pip.fill (0);
     }
     else {
       img.fill (0);
     }

     // NOTE: "neutral", "player_1" and "player_2" from file "default.svg" cause
     // odd effects at the corners. You get a cleaner look if they are omitted.

     switch (i) {
     case Neutral:
       // svg.render (&q, "neutral");
       q.setBrush (color0);
       q.drawRoundedRect (rect, pc, pc, Qt::RelativeSize);
       svg.render (&q, "lighting");
       break;
     case Player1:
       // svg.render (&q, "player_1");
       q.setBrush (color1);
       q.drawRoundedRect (rect, pc, pc, Qt::RelativeSize);
       svg.render (&q, "lighting");
       break;
     case Player2:
       // svg.render (&q, "player_2");
       q.setBrush (color2);
       q.drawRoundedRect (rect, pc, pc, Qt::RelativeSize);
       svg.render (&q, "lighting");
       break;
     case Pip:
       r.begin(&pip);
       svg.render (&r, "pip");
       r.end();
       break;
     case BlinkLight:
       svg.render (&q, "blink_light");
       break;
     case BlinkDark:
       svg.render (&q, "blink_dark");
       break;
     default:
       break;
     }
     q.end();
     elements.append
       ((i == Pip) ? QPixmap::fromImage (pip) : QPixmap::fromImage (img));
   }
   qDebug() << t.restart() << "msec" << "SVG rendered";
}

void KCubeBoxWidget::colorImage (QImage & img, const QColor & c, const int w)
{
   QRgb rgba = c.rgba();
   for (int i = 0; i < w; i++) {
      for (int j = 0; j < w; j++) {
         if (img.pixel (i, j) != 0) {
	    img.setPixel (i, j, rgba);
         }
      }
   }
}

void KCubeBoxWidget::paintEvent (QPaintEvent * /* event unused */)
{
   QPainter p (this);
   p.drawPixmap (0, 0, background);
}

void KCubeBoxWidget::resizeEvent (QResizeEvent * event)
{
   reCalculateGraphics (event->size().width(), event->size().height());
}

void KCubeBoxWidget::reCalculateGraphics (const int w, const int h)
{
   int boxSize = qMin(w, h);
   int frameWidth = boxSize / 30;
   int hairline = drawHairlines ? frameWidth / 10 : 0;
   qDebug() << "boxSize" << boxSize << "frameWidth" << frameWidth << "hairline" << hairline;
   boxSize = boxSize - (2 * frameWidth);
   cubeSize = ((boxSize - hairline) / m_side) - hairline;
   boxSize = ((cubeSize + hairline) * m_side) + hairline;
   topLeft.setX ((w - boxSize)/2);
   topLeft.setY ((h - boxSize)/2);

   qDebug() << "Dimension:" << m_side << "cubeSize:" << cubeSize << "topLeft:" << topLeft;
   makeSVGBackground (w, h);
   makeSVGCubes (cubeSize);
   for (int x = 0; x < m_side; x++) {
      for (int y = 0; y < m_side; y++) {
	 int index = x * m_side + y;
         cubes.at (index)->move (
                            topLeft.x() + hairline + x * (cubeSize + hairline),
                            topLeft.y() + hairline + y * (cubeSize + hairline));
         cubes.at (index)->resize (cubeSize, cubeSize);
      }
   }
   setPopup();
}

QSize  KCubeBoxWidget::sizeHint() const
{
   return QSize(400,400);
}

/* ***************************************************************** **
**                   other private functions                         **
** ***************************************************************** */

void KCubeBoxWidget::startAnimation (bool cascading, int index)
{
   int interval = 0;
   m_index = index;
   currentAnimation = cascading ? cascadeAnimation : ComputerMove;
   switch (currentAnimation) {
   case None:
      animationCount = 0;
      return;				// Should never happen.
      break;
   case ComputerMove:
      interval = 150 + (Prefs::animationSpeed() - 1) * 50;	// 150-600 msec.
      animationCount = 4;
      cubes.at (index)->setLight();
      break;
   case Darken:
      interval = animationTime;
      animationCount = 1;
      cubes.at (index)->setDark();
      break;
   case RapidBlink:
      interval = 60 + Prefs::animationSpeed() * 30;		// 120-360 msec.
      animationCount = 4;
      cubes.at (index)->setLight();
      break;
   case Scatter:
      interval = (animationTime + animationSteps/2) / animationSteps;
      animationCount = animationSteps;
      break;
   }
   animationTimer->setInterval (interval);
   animationTimer->start();
}

void KCubeBoxWidget::nextAnimationStep()
{
   animationCount--;
   if (animationCount < 1) {
      animationTimer->stop();		// Finish normally.
      cubes.at (m_index)->setNeutral();
      currentAnimation = None;
      emit animationDone (m_index);
      return;
   }
   switch (currentAnimation) {
   case None:
      return;				// Should not happen.
      break;
   case ComputerMove:
   case RapidBlink:
      if (animationCount%2 == 1) {	// Set light or dark phase.
         cubes.at (m_index)->setDark();
      }
      else {
         cubes.at (m_index)->setLight();
      }
      break;
   case Darken:
      break;				// Should never happen (1 tick).
   case Scatter:
      int step = animationSteps - animationCount;
      if (step <= 2) {			// Set the animation phase.
         cubes.at (m_index)->shrink(1.0 - step * 0.3);
      }
      else if (step < 7) {
         cubes.at (m_index)->expand((step - 2) * 0.2);
      }
      else if (step == 7) {
         cubes.at (m_index)->expand(1.2);
         scatterDots (0);
      }
      else {
         scatterDots (step - 7);
      }
      break;
   }
}

void KCubeBoxWidget::scatterDots (int step)
{
   Player player = cubes.at(m_index)->owner();
   int d = m_side - 1;
   int x = m_index / m_side;
   int y = m_index % m_side;
   if (x > 0) cubes.at (m_index - m_side)->migrateDot (+1,  0, step, player);
   if (x < d) cubes.at (m_index + m_side)->migrateDot (-1,  0, step, player);
   if (y > 0) cubes.at (m_index - 1)     ->migrateDot ( 0, +1, step, player);
   if (y < d) cubes.at (m_index + 1)     ->migrateDot ( 0, -1, step, player);
}

int KCubeBoxWidget::killAnimation()
{
   if (animationTimer->isActive()) {
      animationTimer->stop();   	// Stop current animation immediately.
   }
   return m_index;
}

const QPixmap & KCubeBoxWidget::playerPixmap (const int p)
{
   return ((p == 1) ? status1 : status2);
}

void KCubeBoxWidget::setPopup()
{
   QFont f;
   f.setPixelSize ((int) (height() * 0.04 + 0.5));
   f.setWeight (QFont::Bold);
   f.setStretch (QFont::Expanded);
   m_popup->setStyleSheet("QLabel { color : rgba(255, 255, 255, 75%); }");
   m_popup->setFont (f);
   m_popup->resize (width(), (int) (height() * 0.08 + 0.5));
   m_popup->setAlignment (Qt::AlignCenter);
}

void KCubeBoxWidget::showPopup (const QString & message)
{
   m_popup->setText (message);
   m_popup->move ((this->width()  - m_popup->width()) / 2,
                  (this->height() - m_popup->height()) / 2 +
                  (cubes.at (0)->height() / 5));
   m_popup->raise();
   m_popup->show();
   update();
}

void KCubeBoxWidget::hidePopup()
{
   m_popup->hide();
   update();
}

#include "kcubeboxwidget.moc"
