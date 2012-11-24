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
#include <KLocalizedString>
#include <KMessageBox>
#include <QTimer>
#include <QPainter>

#include <assert.h>
#include <kcursor.h>

#include "prefs.h"

KCubeBoxWidget::KCubeBoxWidget (const int d, QWidget *parent)
        : QWidget(parent),
	  m_side (d),
	  m_currentPlayer (One)
{
   m_box = new AI_Box (m_side);
   m_steps = new QList<int>;
   init();
   m_gameHasBeenWon   = false;
   m_pauseForComputer = false;	// IDW test. TODO - Put this in Settings.
   m_pauseForStep     = false;	// IDW test. TODO - Put this in Settings.
   m_playerWaiting    = computerPlOne ? One : Nobody;
   m_waitingForStep   = false;
}

KCubeBoxWidget::~KCubeBoxWidget()
{
   if(isActive())
    stopActivities();
   if(cubes)
      deleteCubes(); // IDW TODO - Needed?
   delete m_box; // IDW test.
}

void KCubeBoxWidget::loadSettings(){
  bool reColorCubes = ((color1 != Prefs::color1()) ||
                       (color2 != Prefs::color2()) ||
                       (color0 != Prefs::color0()));
  bool reSizeCubes  = (m_side != Prefs::cubeDim());

  color1 = Prefs::color1();
  color2 = Prefs::color2();
  color0 = Prefs::color0();

  animationTime = Prefs::animationSpeed() * 150;
  fullSpeed = false;
  if (Prefs::animationNone()) {
     fullSpeed = true;
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

  setDim (Prefs::cubeDim());

  if (reColorCubes) {
     makeStatusPixmaps (sWidth);		// Make new status pixmaps.
     emit colorChanged (m_currentPlayer);	// Change color in status bar.
  }
  if (reSizeCubes) {
     reCalculateGraphics (width(), height());
  }
  else if (reColorCubes) {
     makeSVGCubes (cubeSize);
     setColors ();
  }

  brain.setSkill (Prefs::skill1(), Prefs::kepler1(), Prefs::newton1(),
                  Prefs::skill2(), Prefs::kepler2(), Prefs::newton2());
  qDebug() << "PLAYER 1 settings: skill" << Prefs::skill1() << "Kepler" << Prefs::kepler1() << "Newton" << Prefs::newton1();
  qDebug() << "PLAYER 2 settings: skill" << Prefs::skill2() << "Kepler" << Prefs::kepler2() << "Newton" << Prefs::newton2();

  setComputerplayer (One, Prefs::computerPlayer1());
  setComputerplayer (Two, Prefs::computerPlayer2());

  if (reSizeCubes) {
     // Start a new game (in the KJumpingCube object).
     emit dimensionsChanged();
     return;
  }
  else { // IDW TODO - Fix this test for empty box. if (m_cubesToWin [Cube::Nobody] > 0) {
     // Continue current game, maybe with change of computer player settings.
     checkComputerplayer (m_currentPlayer);
     return;
  }
  // There is no more to do: we are waiting for a click to start the game.
}

void KCubeBoxWidget::reset()
{
   stopActivities();

   int i,j;
   for (i = 0; i < m_side; i++)
      for (j = 0; j < m_side; j++)
      {
         cubes[i][j]->reset();
      }

   m_box->clear();

   KCubeWidget::enableClicks(true);

   m_currentPlayer = One;

   m_playerWaiting    = computerPlOne ? One : Nobody;
   m_waitingForStep   = false;

   emit playerChanged(One);
   // When re-starting, WAIT FOR A CLICK.
   // checkComputerplayer(One); // Enable this line for IDW high-speed test.

   brain.startStats();
}

void KCubeBoxWidget::undo()
{
   // IDW TODO - Return true/false dep. on whether any moves left to undo.
   if(isActive())
      return;

   // IDW test. Player oldPlayer=currentPlayer;
   Player oldPlayer = m_currentPlayer;

   // IDW TODO - Skip over computer players. Avoid undo if two computer players?
   bool isAI = false;
   m_box->undoPosition (m_currentPlayer, isAI);
   // IDW test. m_box->undoPosition (m_currentPlayer, isAI);
   // IDW test. m_box->redoPosition (m_currentPlayer, isAI);

   // IDW TODO - Update the cube display after an undo or redo.
   for (int x = 0; x < m_side; x++) {
       for (int y = 0; y < m_side; y++) {
	   int index = x * m_side + y;
           cubes[x][y]->setOwner (m_box->owner (index));
           cubes[x][y]->setValue (m_box->value (index));
       }
   }
   if (oldPlayer != m_currentPlayer)
      emit playerChanged (m_currentPlayer);

   checkComputerplayer (m_currentPlayer);
}
void KCubeBoxWidget::getHint()
{
   if(isActive())
      return;

   int row=0,column=0;

   emit startedThinking();
   bool done = brain.getMove (row, column, m_currentPlayer, m_box);
   if (delayedShutdown) {
      delayedShutdown = false;
      emit shutdownNow();
      return;
   }
   emit stoppedThinking();

   qDebug() << "HINT FOR PLAYER" << m_currentPlayer << "X" << row << "Y" << column;
   if (done) {
      startAnimation (Hint, row, column);
   }
   // If (! done), we interrupted the brain, so we do not want the hint.
}

void KCubeBoxWidget::setColors ()
{
   for (int row = 0; row < m_side; row++) {
      for (int col = 0; col < m_side; col++) {
         cubes[row][col]->updateColors();
      }
   }
}

void KCubeBoxWidget::setDim(int d)
{
   if (d != m_side) {
      delete m_box;
      m_box   = new AI_Box (d);
      m_side  = d;
      reset();
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

void KCubeBoxWidget::saveProperties (KConfigGroup & config)
{
   // IDW TODO - Save settings for computer player(s), animation, etc.
   if (isMoving()) {
      stopActivities();
      undo();
   }
   else if (brain.isActive()) {
      stopActivities();
   }

   // save current player
   config.writeEntry ("onTurn", (int) m_currentPlayer);

   QStringList list;
   QString owner, value, key;

   for (int x = 0; x < m_side; x++) {
     for (int y = 0; y < m_side; y++) {
	key.sprintf ("%u,%u", x, y);
	// IDW test. owner.sprintf("%u",cubes[row][y]->owner());
	// IDW test. value.sprintf("%u",cubes[row][y]->value());
	int index = x * m_side + y;
	owner.sprintf ("%u", m_box->owner (index));
	value.sprintf ("%u", m_box->value (index));
	list.append (owner.toAscii());
	list.append (value.toAscii());
	config.writeEntry (key, list);

	list.clear();
     }
   }
   config.writeEntry ("CubeDim", m_side);
}

void KCubeBoxWidget::readProperties (const KConfigGroup& config)
{
  // IDW TODO - Restore settings for computer player(s), animation, etc.
  // IDW TODO - If a computer player is "on turn", wait for a click or use
  //            a KMessageBox ...
  QStringList list;
  QString     key;
  int         owner, value, maxValue;
  int         minDim = 3, maxDim = 10;

  // Dimension must be 3 to 10.
  int cubeDim = config.readEntry ("CubeDim", minDim);
  if ((cubeDim < minDim) || (cubeDim > maxDim)) {
     KMessageBox::sorry (this, i18n("The file's cube box size is outside "
                                    "the range %1 to %2. It will be set to %1.")
                                    .arg(minDim).arg(maxDim));
     cubeDim = 3;
  }
  m_side = 1;			// Force setDim() to create a new AI_Box.
  setDim (cubeDim);

  for (int x = 0; x < m_side; x++) {
    for (int y = 0; y < m_side; y++) {
	key.sprintf ("%u,%u", x, y);
	list = config.readEntry (key, QStringList());
	// List length must be 2, owner must be 0-2, value >= 1 and <= max().
	if (list.count() < 2) {
	    KMessageBox::sorry (this, i18n("Missing input line for cube %1.")
		    .arg(key));
	    owner = 0;
	    value = 1;
	}
	else {
	    owner = list.at(0).toInt();
	    value = list.at(1).toInt();
	}
	if ((owner < 0) || (owner > 2)) {
	    KMessageBox::sorry (this, i18n("Owner of cube %1 is outside the "
                                           "range 0 to 2.").arg(key));
	    owner = 0;
	}
	int index = x * m_side + y;
	maxValue = (owner == 0) ? 1 : m_box->maxValue (index);
	if ((value < 1) || (value > maxValue)) {
	    KMessageBox::sorry (this, i18n("Value of cube %1 is outside the "
                                           "range 1 to %2.")
                                           .arg(key).arg(maxValue));
	    value = maxValue;
	}
	cubes[x][y]->setOwner ((Player) owner);
	cubes[x][y]->setValue (value);
	m_box->setOwner (index, (Player) owner);
	m_box->setValue (index, value);

	list.clear();
    }
  }

   // Set current player - must be 1 or 2.
   int onTurn = config.readEntry ("onTurn", 1);
   if ((onTurn < 1) || (onTurn > 2)) {
       KMessageBox::sorry (this, i18n("Current player is neither 1 nor 2."));
       onTurn = 1;
   }
   m_currentPlayer = (Player) onTurn;
   emit playerChanged (m_currentPlayer);
   checkComputerplayer (m_currentPlayer);
}

/* ***************************************************************** **
**                               slots                               **
** ***************************************************************** */
void KCubeBoxWidget::setWaitCursor()
{
   // IDW test. setCursor(Qt::BusyCursor); TODO - Decide on wristwatch v. disk.
   setCursor(Qt::WaitCursor);
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
   // IDW test. if (m_pauseForStep && (currentAnimation != None) && (! isMoving())) {
   if (isClick && m_waitingForStep) {
      m_waitingForStep = false;
      animationTimer->start();
      return false;
   }

   if(isActive())
      return false;

   // make the game start when computer player is player one and user clicks
   // IDW test. if (isClick && m_currentPlayer == One && computerPlOne)
   // IDW test. if (isClick && m_pauseForComputer && isComputer (m_currentPlayer)) {
   if (isClick && (m_playerWaiting != Nobody)) {
      checkComputerplayer (m_playerWaiting);
      return false;
   }
   else if (m_currentPlayer == cubes[row][column]->owner() ||
		   cubes[row][column]->owner() == Nobody) {
      doMove(row,column);
      return true;
   }
   return false;
}

void KCubeBoxWidget::checkComputerplayer(Player player)
{
   // checking if a process is running or the Widget isn't shown yet
   if(isActive() || !isVisible())
      return;
   if ((player == One && computerPlOne && m_currentPlayer == One) ||
       (player == Two && computerPlTwo && m_currentPlayer == Two)) {
      if (m_pauseForComputer && (m_playerWaiting == Nobody)) {
         m_playerWaiting = player;
         return;
      }
      m_playerWaiting = Nobody;
      KCubeWidget::enableClicks(false);

      int row=0,column=0;
      emit startedThinking();
      qDebug() << "Calling brain.getMove() for player" << player;
      t.start();
      brain.getMove (row, column, player, m_box);
      qDebug() << "TIME of MOVE" << t.elapsed();
      qDebug() << "==============================================================";
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

/* ***************************************************************** **
**                   initializing functions                          **
** ***************************************************************** */
void KCubeBoxWidget::init()
{
   delayedShutdown = false;	// True if we need to quit, but brain is active.

   fullSpeed      = false;
   currentAnimation = None;
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

   m_currentPlayer = One;
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

   brain.startStats();
}

void KCubeBoxWidget::initCubes()
{
   const int s = m_side;
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
   cubeSize = ((boxSize - hairline) / m_side) - hairline;
   boxSize = ((cubeSize + hairline) * m_side) + hairline;
   topLeft.setX ((w - boxSize)/2);
   topLeft.setY ((h - boxSize)/2);

   qDebug() << "Dimension:" << m_side << "cubeSize:" << cubeSize << "topLeft:" << topLeft;
   makeSVGBackground (w, h);
   makeSVGCubes (cubeSize);
   for (int i = 0; i < m_side; i++) {
      for (int j = 0; j < m_side; j++) {
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
   // IDW test. CubeBoxBase<KCubeWidget>::deleteCubes();
   qDebug() << "ENTERED NO-OP PROCEDURE KCubeBoxWidget::deleteCubes()";
}


/* ***************************************************************** **
**                   other private functions                         **
** ***************************************************************** */

void KCubeBoxWidget::doMove(int row,int column)
{
   // if a move hasn't finished yet don't do another move
   if(isActive())
      return;

   // IDW TODO. Allow undo and redo of computer moves.

   bool computerMove = ((computerPlOne && m_currentPlayer == One) ||
                        (computerPlTwo && m_currentPlayer == Two));
   if (! computerMove) { // Make only human-players' moves undoable.
      // For the undo-function: make a copy of the playfield.
      m_box->copyPosition (m_currentPlayer, computerMove);
      brain.postMove (m_currentPlayer, row, column); // IDW test.
   }
   int index = row * m_side + column;
   m_steps->clear();
   m_gameHasBeenWon = m_box->doMove (m_currentPlayer, index, m_steps);
   m_box->printBox(); // IDW test.
   // IDW test. qDebug() << "GAME WON?" << m_gameHasBeenWon << "STEPS" << (* m_steps);
   doStep();
}

void KCubeBoxWidget::doStep()
{
   int index;
   bool startStep = true;
   do {
      if (! m_steps->isEmpty()) {
         index = m_steps->takeFirst();
         if (index == 0) {
            qDebug() << "Win for player" << m_currentPlayer << "at this step.";
            emit stoppedMoving();
            currentAnimation = None;
            emit playerWon ((int) m_currentPlayer);
            reset();
            return;
         }

         startStep = (index > 0);
         index = startStep ? (index - 1) : (-index - 1);
         int x = index / m_side;
         int y = index % m_side;
	 int maxValue = m_box->maxValue (index);
         if (startStep) {
            int value = cubes[x][y]->value() + 1;
            cubes[x][y]->setOwner (/* (Cube::Owner) */ m_currentPlayer);
            cubes[x][y]->setValue (value);
	    if (value > maxValue) {
               cubes[x][y]->setDark();
	    }
	    // IDW test. qDebug() << "INCREMENT" << index << x << y << "to" << value << "maxVal" << maxValue << "dark" << (value > maxValue);
         }
         else {
	    // IDW test. cubes[x][y]->setNeutral();
	    startAnimation (cascadeAnimation, x, y);
         }
      }
      else {
	 qDebug() << "End of move for player" << m_currentPlayer;
         emit stoppedMoving();
         currentAnimation = None;
         KCubeWidget::enableClicks(true);

	 // The queued call to changePlayer() lets the graphics-update finish
	 // before a possible computer move that might take time to calculate.
         QMetaObject::invokeMethod (this, "changePlayer", Qt::QueuedConnection);
         return;
      }
   } while (startStep || (cascadeAnimation == None));
}

void KCubeBoxWidget::startAnimation (AnimationType type, int row, int col)
{
   int interval = 0;
   currentAnimation = type;
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
   animationTimer->setInterval (interval);
   if (m_pauseForStep &&
       (currentAnimation != Hint) && (currentAnimation != ComputerMove)) {
      m_waitingForStep = true;		// Timer will start on next click.
      return;
   }
   animationTimer->start();
   // IDW test. qDebug() << "START ANIMATION" << m_row << m_col << "type" << currentAnimation << "count" << animationCount << "interval" << interval;
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
   // IDW test. qDebug() << "KCubeBoxWidget::scatterDots (" << step << ")";
   Player player = m_currentPlayer;
   int d = m_side - 1;
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
      int max = m_box->maxValue (m_row * m_side + m_col);
      cubes[m_row][m_col]->setValue (cubes[m_row][m_col]->value() - max);
      doStep();
      break;
   }
}

Player KCubeBoxWidget::changePlayer()
{
   m_currentPlayer = (m_currentPlayer==One) ? Two : One;

   emit playerChanged (m_currentPlayer);
   checkComputerplayer (m_currentPlayer);
   KCubeWidget::enableClicks (true);
   return m_currentPlayer;
}

const QPixmap & KCubeBoxWidget::playerPixmap (const int p)
{
   return ((p == 1) ? status1 : status2);
}

#include "kcubeboxwidget.moc"
