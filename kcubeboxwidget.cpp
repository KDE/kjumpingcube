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
#include "kcubeboxwidget.h"

#include <KgTheme>
#include <KStandardDirs>
#include <QTimer>
#include <QPainter>

#include <assert.h>
#include <kcursor.h>

#include "prefs.h"

KCubeBoxWidget::KCubeBoxWidget(const int d,QWidget *parent)
        : QWidget(parent),
          CubeBoxBase<KCubeWidget>(d)
{
   init();
}



KCubeBoxWidget::KCubeBoxWidget(CubeBox& box,QWidget *parent)
      :QWidget(parent),
       CubeBoxBase<KCubeWidget>(box.dim())
{
   init();

   int i,j;
   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
         *cubes[i][j]=*box[i][j];
      }

   currentPlayer=(KCubeBoxWidget::Player)box.player();
}



KCubeBoxWidget::KCubeBoxWidget(const KCubeBoxWidget& box,QWidget *parent)
      :QWidget(parent),
       CubeBoxBase<KCubeWidget>(box.dim())
{
   init();

   int i,j;
   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
         *cubes[i][j]=*box.cubes[i][j];
      }


   currentPlayer=box.currentPlayer;
}



KCubeBoxWidget::~KCubeBoxWidget()
{
   if(isActive())
    stopActivities();
   if(cubes)
      deleteCubes();
   delete undoBox;
}

void KCubeBoxWidget::loadSettings(){
  bool reColorCubes = ((color1 != Prefs::color1()) ||
                       (color2 != Prefs::color2()) ||
                       (color0 != Prefs::color0()));
  bool reSizeCubes  = (dim() != Prefs::cubeDim());

  color1 = Prefs::color1();
  color2 = Prefs::color2();
  color0 = Prefs::color0();
  animationTime = Prefs::animationSpeed() * 150;
  setDim (Prefs::cubeDim());

  if (reColorCubes) {
     makeStatusPixmaps (sWidth);		// Make new status pixmaps.
     emit colorChanged(currentPlayer);		// Change color in status bar.
  }
  if (reSizeCubes) {
     reCalculateGraphics (width(), height());
  }
  else if (reColorCubes) {
     makeSVGCubes (cubeSize);
     setColors ();
  }

  brain.setSkill( Prefs::skill() );

  setComputerplayer(KCubeBoxWidget::One, Prefs::computerPlayer1());
  setComputerplayer(KCubeBoxWidget::Two, Prefs::computerPlayer2());

  if (reSizeCubes) {
     // Start a new game (in the KJumpingCube object).
     emit dimensionsChanged();
     return;
  }
  else if (m_cubesToWin [Cube::Nobody] > 0) {
     // Continue current game, maybe with change of computer player settings.
     checkComputerplayer(currentPlayer);
     return;
  }
  // There is no more to do: we are waiting for a click to start the game.
}

KCubeBoxWidget& KCubeBoxWidget::operator=(const KCubeBoxWidget& box)
{
   if(this!=&box)
   {
      if(dim()!=box.dim())
      {
         setDim(box.dim());
      }


      for(int i=0;i<dim();i++)
         for(int j=0;j<dim();j++)
         {
            *cubes[i][j]=*box.cubes[i][j];
         }

      currentPlayer=box.currentPlayer;
   }
   return *this;
}

KCubeBoxWidget& KCubeBoxWidget::operator=(CubeBox& box)
{
   if(dim()!=box.dim())
   {
      setDim(box.dim());
   }

   for(int i=0;i<dim();i++)
      for(int j=0;j<dim();j++)
      {
         *cubes[i][j]=*box[i][j];
      }

   currentPlayer=(KCubeBoxWidget::Player)box.player();

   return *this;
}

void KCubeBoxWidget::reset()
{
   stopActivities();

   int i,j;
   for(i=0;i<dim();i++)
      for(j=0;j<dim();j++)
      {
         cubes[i][j]->reset();
      }

   m_cubesToWin [Cube::Nobody] = 0;
   m_cubesToWin [Cube::One]    = dim() * dim();
   m_cubesToWin [Cube::Two]    = dim() * dim();

   KCubeWidget::enableClicks(true);

   currentPlayer=One;

   emit playerChanged(One);
   // When re-starting, WAIT FOR A CLICK.
   // checkComputerplayer(One); // Enable this line for IDW high-speed test.
}

void KCubeBoxWidget::undo()
{
   if(isActive())
      return;

   Player oldPlayer=currentPlayer;

   *this=*undoBox;

   if(oldPlayer!=currentPlayer)
      emit playerChanged(currentPlayer);

   checkComputerplayer(currentPlayer);

}

void KCubeBoxWidget::getHint()
{
   if(isActive())
      return;

   int row=0,column=0;
   CubeBox field=CubeBox(*this);

   emit startedThinking();
   bool done = brain.getHint(row,column,(CubeBox::Player)currentPlayer,field);
   if (delayedShutdown) {
      delayedShutdown = false;
      emit shutdownNow();
      return;
   }
   emit stoppedThinking();

   if (done) {
      startAnimation (Hint, row, column);
   }
   // If (! done), we interrupted the brain, so we do not want the hint.
}

void KCubeBoxWidget::setColors ()
{
   for (int row=0; row<dim(); row++) {
      for (int col=0; col<dim(); col++) {
         cubes[row][col]->updateColors();
      }
   }
}

void KCubeBoxWidget::setDim(int d)
{
   if(d != dim())
   {
      undoBox->setDim(d);
      CubeBoxBase<KCubeWidget>::setDim(d);
   }
}

void KCubeBoxWidget::setComputerplayer(Player player,bool flag)
{
   if(player==One)
      computerPlOne=flag;
   else if(player==Two)
      computerPlTwo=flag;
}

bool KCubeBoxWidget::shutdown()
{
   if (animationTimer->isActive()) {
      animationTimer->stop();	// Stop move or hint animation immediately.
   }
   if (brain.isActive()) {
      brain.stop();		// Brain stops only after next "thinking" cycle.
      delayedShutdown = true;
   }
   return (! delayedShutdown);
}

void KCubeBoxWidget::stopActivities()
{
   if (animationTimer->isActive()) {
      fullSpeed = true;		// If moving, complete the move immediately.
      stopAnimation();
   }
   if (brain.isActive()) {
      brain.stop();
      emit stoppedThinking();
   }
}

void KCubeBoxWidget::saveProperties(KConfigGroup& config)
{
   if(isMoving())
   {
      stopActivities();
      undo();
   }
   else if(brain.isActive())
      stopActivities();

   // save current player
   config.writeEntry("onTurn",(int)currentPlayer);

   QStringList list;
   //list.setAutoDelete(true);
   QString owner, value, key;
   int cubeDim=dim();

   for(int row=0; row < cubeDim ; row++)
     for(int column=0; column < cubeDim ; column++)
     {
	key.sprintf("%u,%u",row,column);
	owner.sprintf("%u",cubes[row][column]->owner());
	value.sprintf("%u",cubes[row][column]->value());
	list.append(owner.toAscii());
	list.append(value.toAscii());
	config.writeEntry(key,list);

	list.clear();
      }
  config.writeEntry("CubeDim",dim());
}

void KCubeBoxWidget::readProperties(const KConfigGroup& config)
{
  QStringList list;
  //list.setAutoDelete(true);
  QString owner, value, key;
  setDim(config.readEntry("CubeDim",5));
  int cubeDim=dim();

  for(int row=0; row < cubeDim ; row++)
    for(int column=0; column < cubeDim ; column++)
      {
	key.sprintf("%u,%u",row,column);
	list = config.readEntry(key,QStringList());
	owner=list.at(0);
	value=list.at(1);
	cubes[row][column]->setOwner((KCubeWidget::Owner)owner.toInt());
	cubes[row][column]->setValue(value.toInt());

	list.clear();
      }


   // set current player
   int onTurn=config.readEntry("onTurn",1);
   currentPlayer=(Player)onTurn;
   emit playerChanged(onTurn);
   checkComputerplayer((Player)onTurn);
}

/* ***************************************************************** **
**                               slots                               **
** ***************************************************************** */
void KCubeBoxWidget::setWaitCursor()
{
   setCursor(Qt::BusyCursor);
}



void KCubeBoxWidget::setNormalCursor()
{
   setCursor(Qt::PointingHandCursor);
}

void KCubeBoxWidget::stopHint (bool shutdown)
{
    // IDW TODO - Rewrite this?
/*
   int d=dim();
   for(int i=0;i<d;i++)
      for(int j=0;j<d;j++)
      {
         cubes[i][j]->stopHint (shutdown);
      }
*/
}

bool KCubeBoxWidget::checkClick(int row,int column, bool isClick)
{
   if(isActive())
      return false;

   // make the game start when computer player is player one and user clicks
   if(isClick && currentPlayer == One && computerPlOne)
   {
      checkComputerplayer(currentPlayer);
      return false;
   }
   else if((Cube::Owner)currentPlayer==cubes[row][column]->owner() ||
		   cubes[row][column]->owner()==Cube::Nobody)
   {
      doMove(row,column);
      return true;
   }
   else
      return false;
}

void KCubeBoxWidget::checkComputerplayer(Player player)
{
   // checking if a process is running or the Widget isn't shown yet
   if(isActive() || !isVisible())
      return;
   if((player==One && computerPlOne && currentPlayer==One)
         || (player==Two && computerPlTwo && currentPlayer==Two))
   {
      KCubeWidget::enableClicks(false);

      CubeBox field(*this);
      int row=0,column=0;
      emit startedThinking();
      brain.getHint (row, column, (CubeBoxBase<Cube>::Player) player, field);
      if (delayedShutdown) {
         delayedShutdown = false;
         emit shutdownNow();
         return;
      }
      emit stoppedThinking();

      // We do not care if we interrupted the computer.  It was probably taking
      // too long, so we will just take the best move it had so far.

      // Blink the cube to be moved (twice).  The realMove = true flag tells
      // the cube to simulate a mouse click and trigger the move animation,
      // but not until after the blinking is finished.  The cube's "clicked"
      // signal is connected to "checkClick (row, column, false)".`

      startAnimation (ComputerMove, row, column);
      // checkClick (row, column, false); // Use this for IDW high-speed test.
   }
}

/* ***************************************************************** **
**                         status functions                          **
** ***************************************************************** */

bool KCubeBoxWidget::isActive() const
{
   bool flag=false;
   if(animationTimer->isActive())
      flag=true;
   else if(brain.isActive())
      flag=true;

   return flag;
}

bool KCubeBoxWidget::isMoving() const
{
   if ((currentAnimation == Hint) || (currentAnimation == ComputerMove)) {
      return false;
   }
   else {
      return animationTimer->isActive();
   }
}

bool KCubeBoxWidget::isComputer(Player player) const
{
   if(player==One)
      return computerPlOne;
   else
      return computerPlTwo;
}

int KCubeBoxWidget::skill() const
{
   return brain.skill();
}

/* ***************************************************************** **
**                   initializing functions                          **
** ***************************************************************** */
void KCubeBoxWidget::init()
{
   delayedShutdown = false;	// True if we need to quit, but brain is active.

   fullSpeed      = false;
   animationSteps = 12;
   animationCount = 0;

   setMinimumSize (200, 200);
   color1 = Prefs::color1();			// Set preferred colors.
   color2 = Prefs::color2();
   color0 = Prefs::color0();

   KgTheme theme((QByteArray()));
   theme.readFromDesktopFile(KStandardDirs::locate("appdata", "pics/default.desktop"));

   t.start();
   qDebug() << t.restart() << "msec";
   svg.load (theme.graphicsPath());
   qDebug() << t.restart() << "msec" << "SVG loaded ...";
   if (svg.isValid())
	qDebug() << "SVG is valid ...";
   else
	qDebug() << "SVG is NOT valid ...";
   drawHairlines = (theme.customData("DrawHairlines") == "0") ? false : true;

   initCubes();

   undoBox = new CubeBox(dim());

   m_cubesToWin [Cube::Nobody] = 0;
   m_cubesToWin [Cube::One]    = dim() * dim();
   m_cubesToWin [Cube::Two]    = dim() * dim();

   currentPlayer=One;
   animationTime=Prefs::animationSpeed() * 150;
   animationTimer=new QTimer(this);
   computerPlOne=false;
   computerPlTwo=false;
   KCubeWidget::enableClicks(true);

   // At this point the user's currently preferred number of cubes and colors
   // are already loaded, so there should be no change and no SVG rendering yet.
   loadSettings();

   connect(animationTimer,SIGNAL(timeout()),SLOT(nextAnimationStep()));
   connect(this,SIGNAL(startedThinking()),SLOT(setWaitCursor()));
   connect(this,SIGNAL(stoppedThinking()),SLOT(setNormalCursor()));
   connect(this,SIGNAL(startedMoving()),SLOT(setWaitCursor()));
   connect(this,SIGNAL(stoppedMoving()),SLOT(setNormalCursor()));
   connect(this,SIGNAL(playerWon(int)),SLOT(stopActivities()));

   setNormalCursor();

   emit playerChanged(One);
}

void KCubeBoxWidget::initCubes()
{
   const int s=dim();
   int i,j;

   // create new cubes
   cubes = new KCubeWidget**[s];
   for(i=0;i<s;i++)
   {
      cubes[i]=new KCubeWidget*[s];
   }
   for(i=0;i<s;i++)
      for(j=0;j<s;j++)
      {
         cubes[i][j] = new KCubeWidget (this);
         cubes[i][j]->setCoordinates (i, j, s - 1);
         cubes[i][j]->setPixmaps (&elements);
         // connect(cubes[i][j],SIGNAL(clicked(int,int,bool)),SLOT(stopHint()));
         connect(cubes[i][j],SIGNAL(clicked(int,int,bool)),
                             SLOT(checkClick(int,int,bool)));
         cubes[i][j]->show();
      }

   // initialize cubes
   int max=dim()-1;

   cubes[0][0]->setMax(2);
   cubes[0][max]->setMax(2);
   cubes[max][0]->setMax(2);
   cubes[max][max]->setMax(2);

   for(i=1;i<max;i++)
   {
      cubes[i][0]->setMax(3);
      cubes[i][max]->setMax(3);
      cubes[0][i]->setMax(3);
      cubes[max][i]->setMax(3);
   }

   for(i=1;i<max;i++)
     for(j=1;j<max;j++)
      {
         cubes[i][j]->setMax(4);
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
   qDebug() << endl << "KCubeBoxWidget::resizeEvent:" << event->size() << this->size();
   reCalculateGraphics (event->size().width(), event->size().height());
}

void KCubeBoxWidget::reCalculateGraphics (const int w, const int h)
{
   int boxSize = qMin(w, h);
   int frameWidth = boxSize / 30;
   int hairline = drawHairlines ? frameWidth / 10 : 0;
   qDebug() << "boxSize" << boxSize << "frameWidth" << frameWidth << "hairline" << hairline;
   boxSize = boxSize - (2 * frameWidth);
   cubeSize = ((boxSize - hairline) / dim()) - hairline;
   boxSize = ((cubeSize + hairline) * dim()) + hairline;
   topLeft.setX ((w - boxSize)/2);
   topLeft.setY ((h - boxSize)/2);

   qDebug() << "Dimension:" << dim() << "cubeSize:" << cubeSize << "topLeft:" << topLeft;
   makeSVGBackground (w, h);
   makeSVGCubes (cubeSize);
   for (int i = 0; i < dim(); i++) {
      for (int j = 0; j < dim(); j++) {
         cubes[i][j]->move (topLeft.x() + hairline + i * (cubeSize + hairline),
                            topLeft.y() + hairline + j * (cubeSize + hairline));
         cubes[i][j]->resize (cubeSize, cubeSize);
      }
   }
}

QSize  KCubeBoxWidget::sizeHint() const
{
   return QSize(400,400);
}

void  KCubeBoxWidget::deleteCubes()
{
   CubeBoxBase<KCubeWidget>::deleteCubes();
}


/* ***************************************************************** **
**                   other private functions                         **
** ***************************************************************** */

void KCubeBoxWidget::doMove(int row,int column)
{
   // if a move hasn't finished yet don't do another move
   if(isActive())
      return;

   bool computerMove = ((computerPlOne && currentPlayer==One) ||
                        (computerPlTwo && currentPlayer==Two));
   if (! computerMove) { // Make only human-players' moves undoable.
      // For the undo-function: make a copy of the playfield.
      *undoBox = *this;
   }

   // Increase this cube's count and previous owner's target: decrease current
   // player's target (the increase() method returns the previous owner).
   m_cubesToWin [cubes[row][column]->increase((Cube::Owner)currentPlayer)] ++;
   m_cubesToWin [currentPlayer] --;

   if(cubes[row][column]->overMax()) {
      startCascade (row, column);
   }
   else {
      changePlayer();
   }
}

void KCubeBoxWidget::startCascade (int row, int col)
{
   fullSpeed = false;
   if (Prefs::animationNone()) {
      fullSpeed = true;
      currentAnimation = None;
      stepTime = 0;
   }
   else if (Prefs::animationDelay() || (Prefs::animationSpeed() <= 1)) {
      currentAnimation = Darken;
      stepTime = animationTime;
   }
   else if (Prefs::animationBlink()) {
      currentAnimation = RapidBlink;
      stepTime = animationTime;
   }
   else if (Prefs::animationSpread()) {
      currentAnimation = Scatter;
      stepTime = animationTime;
   }

   cubes[row][col]->setDark();
   if (currentAnimation != None) {
      startAnimation (row, col);
   }

   emit startedMoving();
   KCubeWidget::enableClicks(false);

   saturated.clear();		// Start a list of cascaded moves.
   saturated.append (row * dim() + col);

   m_row = row;
   m_col = col;
   if (fullSpeed) {
      continueCascade();
   }
}

void KCubeBoxWidget::continueCascade()
{
   do {
      if (nextMoveStep()) {
         continue;
      }

      if (m_cubesToWin [currentPlayer] <= 0) {
         emit stoppedMoving();
         reset();
         return;
      }

      emit stoppedMoving();
      KCubeWidget::enableClicks(true);
      changePlayer();
      return;
   } while (fullSpeed);
}

void KCubeBoxWidget::startAnimation (AnimationType type, int row, int col)
{
   currentAnimation = type;
   startAnimation (row, col);
}

void KCubeBoxWidget::startAnimation (int row, int col)
{
   int interval = 0;
   m_row = row;
   m_col = col;
   switch (currentAnimation) {
   case None:
      animationCount = 0;
      return;
      break;
   case Hint:
   case ComputerMove:
      interval = 150 + (Prefs::animationSpeed() - 1) * 50;	// 150-600 msec.
      animationCount = 4;
      cubes[m_row][m_col]->setLight();
      break;
   case Darken:
      interval = animationTime;
      animationCount = 1;
      cubes[m_row][m_col]->setDark();
      break;
   case RapidBlink:
      interval = 60 + Prefs::animationSpeed() * 30;		// 120-360 msec.
      animationCount = 4;
      cubes[m_row][m_col]->setLight();
      break;
   case Scatter:
      interval = (animationTime + animationSteps/2) / animationSteps;
      animationCount = animationSteps;
      break;
   }
   animationTimer->start(interval);
}

void KCubeBoxWidget::nextAnimationStep()
{
   animationCount--;
   if (animationCount < 1) {
      stopAnimation();
      return;
   }
   switch (currentAnimation) {
   case None:
      return;
      break;
   case Hint:
   case ComputerMove:
   case RapidBlink:
      if (animationCount%2 == 1) {
         cubes[m_row][m_col]->setDark();
      }
      else {
         cubes[m_row][m_col]->setLight();
      }
      break;
   case Darken:
      break;
   case Scatter:
      int step = animationSteps - animationCount;
      if (step <= 2) {
         cubes[m_row][m_col]->shrink(1.0 - step * 0.3);
      }
      else if (step < 7) {
         cubes[m_row][m_col]->expand((step - 2) * 0.2);
      }
      else if (step == 7) {
         cubes[m_row][m_col]->expand(1.2);
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
   Cube::Owner player = (Cube::Owner)(currentPlayer);
   int d = dim() - 1;
   if (m_row > 0) cubes[m_row-1][m_col]->migrateDot (+1,  0, step, player);
   if (m_row < d) cubes[m_row+1][m_col]->migrateDot (-1,  0, step, player);
   if (m_col > 0) cubes[m_row][m_col-1]->migrateDot ( 0, +1, step, player);
   if (m_col < d) cubes[m_row][m_col+1]->migrateDot ( 0, -1, step, player);
}

void KCubeBoxWidget::stopAnimation()
{
   animationTimer->stop();
   switch (currentAnimation) {
   case None:
      return;
      break;
   case Hint:
   case ComputerMove:
      cubes[m_row][m_col]->setNeutral();
      if (currentAnimation == ComputerMove) { // IDW TODO - Check shutdown.
         checkClick (m_row, m_col, false);
      }
      break;
   case RapidBlink:
   case Darken:
   case Scatter:
      continueCascade();
      break;
   }
}

bool KCubeBoxWidget::nextMoveStep()
{
   if (saturated.isEmpty()) {
       return false;		// No more moves in this cascade.
   }

   int d = dim();
   int index = saturated.takeLast();
   int row = index / d;
   int col = index % d;

   if (! cubes[row][col]->overMax()) {
       qDebug() << "CUBE IS NOT overMax() !!!!!!!!!!!!!!!!!";
   }
   cubes[row][col]->setNeutral();
   // IDW test. NOT needed? cubes[row][col]->stopHint();
   increaseNeighbours(currentPlayer, row, col);
   cubes[row][col]->decrease();

   int limit = d - 1;
   if ((row > 0) && (cubes[row-1][col]->overMax()) &&
       (cubes[row-1][col]->isNeutral())) {
       cubes[row-1][col]->setDark();
       saturated.append (index - d);	// West.
   }
   if ((col < limit) && (cubes[row][col+1]->overMax()) &&
       (cubes[row][col+1]->isNeutral())) {
       cubes[row][col+1]->setDark();
       saturated.append (index + 1);	// South.
   }
   if ((row < limit) && (cubes[row+1][col]->overMax()) &&
       (cubes[row+1][col]->isNeutral())) {
       cubes[row+1][col]->setDark();
       saturated.append (index + d);	// East.
   }
   if ((col > 0) && (cubes[row][col-1]->overMax()) &&
       (cubes[row][col-1]->isNeutral())) {
       cubes[row][col-1]->setDark();
       saturated.append (index - 1);	// North.
   }
   if (cubes[row][col]->overMax()) {
       cubes[row][col]->setDark();
       saturated.append (index);	// Cube is still saturated.
   }

   if (m_cubesToWin [currentPlayer] <= 0) {
      foreach (index, saturated) {
	  cubes[index/d][index%d]->setNeutral();
      }
      saturated.clear();
      emit playerWon((int)currentPlayer);
      return false;
   }

   bool stillMoving = (! saturated.isEmpty());

   if (stillMoving) {
      index = saturated.last();	// Show next cube to fire.
      startAnimation (index/d, index%d);
   }
   return stillMoving;
}

KCubeBoxWidget::Player KCubeBoxWidget::changePlayer()
{
   currentPlayer=(currentPlayer==One)? Two : One;

   emit playerChanged(currentPlayer);
   checkComputerplayer(currentPlayer);
   KCubeWidget::enableClicks(true);
   return currentPlayer;
}

const QPixmap & KCubeBoxWidget::playerPixmap (const int p)
{
   return ((p == 1) ? status1 : status2);
}

void KCubeBoxWidget::increaseNeighbours(KCubeBoxWidget::Player forWhom,int row,int column)
{
   KCubeWidget::Owner player = (KCubeWidget::Owner)(forWhom);

   // For each neighbour, increase count and previous owner's target: decrease
   // current player's target (the increase() method returns previous owner).
   if (row != 0) {
      m_cubesToWin [cubes[row-1][column]->increase(player)] ++;
      m_cubesToWin [player] --;
   }
   if (row != dim()-1) {
      m_cubesToWin [cubes[row+1][column]->increase(player)] ++;
      m_cubesToWin [player] --;
   }
   if (column != 0) {
      m_cubesToWin [cubes[row][column-1]->increase(player)] ++;
      m_cubesToWin [player] --;
   }
   if (column != dim()-1) {
      m_cubesToWin [cubes[row][column+1]->increase(player)] ++;
      m_cubesToWin [player] --;
   }
}

#include "kcubeboxwidget.moc"
