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
#include <QLabel>
#include <QPainter>

#include <assert.h>
#include <kcursor.h>

#include "prefs.h"

KCubeBoxWidget::KCubeBoxWidget (const int d, QWidget *parent)
        : QWidget(parent),
	  m_side (d),
	  m_currentPlayer (One),
	  m_popup (new QLabel (this))
{
   m_box = new AI_Box (m_side);
   m_steps = new QList<int>;
   cubes.clear();
   init();
   m_gameHasBeenWon   = false;
   m_computerMoveType = Hint;
   m_playerWaiting    = computerPlOne ? One : Nobody;
   m_waitingForStep   = false;
}

KCubeBoxWidget::~KCubeBoxWidget()
{
   if(isActive())
    stopActivities();
   // if(cubes)
      // deleteCubes(); // IDW TODO - Needed?
   delete m_steps;
   delete m_box;
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

  // IDW TODO - Delete? bool oldPauseForStep = m_pauseForStep;
  m_pauseForStep       = Prefs::pauseForStep();
  m_pauseForComputer   = Prefs::pauseForComputer();
  qDebug() << "m_pauseForComputer" << m_pauseForComputer << "m_pauseForStep" << m_pauseForStep; // IDW test.

  if (! m_pauseForComputer) {
      hidePopup();
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

  // IDW TODO - There are just too many actions flowing from settings changes.
  //            Simplify them or work out proper inter-dependencies.
  // IDW TODO - This code is not needed if we leave things so that the popup is
  //            still showing and the first click triggers all remaining steps.
  // if ((! m_pauseForStep) && oldPauseForStep) {
     // IDW TODO - This is the same as part 1 of checkClick().
     // if (animationTimer->isActive() || m_waitingForStep) {
        // m_waitingForStep = false;
        // animationTimer->start();
     // }
  // }
  if (reSizeCubes) {
     // Start a new game (in the KJumpingCube object).
     emit dimensionsChanged();
     // IDW test. Does stop(), disable Undo, reset() AGAIN and status message.
     return;
  }
  else if (! m_box->isClear()) {		// If not first move...
  // IDW TODO - Fix test for empty box. if (m_cubesToWin [Cube::Nobody] > 0) {
     // Continue current game, maybe with change of computer player settings.
     checkComputerplayer (m_currentPlayer);
     return;
  }
  // There is no more to do: we are waiting for a click to start the game.
}

void KCubeBoxWidget::reset()
{
   // IDW TODO - Delete? stopActivities();

   foreach (KCubeWidget * cube, cubes) {
      cube->reset();
   }

   m_box->clear();

   KCubeWidget::enableClicks(true);
   currentAnimation = None;

   m_currentPlayer = One;

   m_playerWaiting    = computerPlOne ? One : Nobody;
   m_waitingForStep   = false;

   emit playerChanged(One);
   // When re-starting, WAIT FOR A CLICK.
   // checkComputerplayer(One); // Enable this line for IDW high-speed test.

   brain.startStats();
}

void KCubeBoxWidget::undo() {
   // IDW TODO - Return true/false dep. on whether any moves left to undo.
   if (isActive())
      return;

   // IDW test. Player oldPlayer=currentPlayer;
   Player oldPlayer = m_currentPlayer;

   // IDW TODO - Skip over computer players. Avoid undo if two computer players?
   bool isAI = false;
   m_box->undoPosition (m_currentPlayer, isAI);
   // IDW test. m_box->undoPosition (m_currentPlayer, isAI);
   // IDW test. m_box->redoPosition (m_currentPlayer, isAI);

   // IDW TODO - Update the cube display after an undo or redo.
   int index = 0;
   foreach (KCubeWidget * cube, cubes) {
       cube->setOwner (m_box->owner (index));
       cube->setValue (m_box->value (index));
       index++;
   }
   if (oldPlayer != m_currentPlayer)
      emit playerChanged (m_currentPlayer);

   checkComputerplayer (m_currentPlayer);
}

void KCubeBoxWidget::checkComputerplayer(Player player)
{
   // Check if a process is running or the widget is not shown yet
   if (isActive() || (! isVisible()))
      return;
   if ((player == One && computerPlOne && m_currentPlayer == One) ||
       (player == Two && computerPlTwo && m_currentPlayer == Two)) {
      if (m_pauseForComputer) {
         if (m_playerWaiting == Nobody) {
            m_playerWaiting = player;
	    showPopup (i18n ("Click anywhere to continue"));
            return;
         }
	 else {
            hidePopup();
         }
      }
      m_playerWaiting = Nobody;
      KCubeWidget::enableClicks (false);

      m_computerMoveType = ComputerMove;
      emit startedThinking();
      qDebug() << "Calling brain.getMove() for player" << player;
      t.start(); // IDW test.
      brain.getMove (player, m_box);
   }
}

void KCubeBoxWidget::getHint()
{
   if (isActive())
      return;

   m_computerMoveType = Hint;
   emit startedThinking();
   brain.getMove (m_currentPlayer, m_box);
}

void KCubeBoxWidget::computerMoveDone (int index)
{
   // We do not care if we interrupted the computer.  It was probably
   // taking too long, so we will just take the best move it had so far.

   if (m_computerMoveType == ComputerMove) {
      qDebug() << "TIME of MOVE" << t.elapsed();
      qDebug() << "==============================================================";
   }
   else if (m_computerMoveType == Hint) {
      qDebug() << "HINT FOR PLAYER" << m_currentPlayer
               << "X" << index / m_side << "Y" << index % m_side;
   }

   // Blink the cube to be moved (twice).  When done, emit stoppedThinking()
   // and simulate a click if it is a ComputerMove, not a Hint.
   startAnimation (m_computerMoveType, index);

   // checkClick (index / m_side, index % m_side, false);
   // Use this for IDW high-speed test.
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
      shutdown();
      delete m_box;
      m_box   = new AI_Box (d);
      m_side  = d;
      initCubes();
      // IDW TODO - Crashed in slot computerMoveDone(int). Starts animation with
      //            too large an index? Going from larger to smaller box.
      // Should we do setDim and return and do the rest of loadSettings later?
      connect(&brain, SIGNAL(done(int)), SLOT(computerMoveDone(int)));
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

void KCubeBoxWidget::shutdown()
{
   // Shut down gracefully, avoiding a possible crash when the user hits Quit.
   if (animationTimer->isActive()) {
      animationTimer->stop();	// Stop move or hint animation immediately.
   }
   if (brain.isActive()) {
      brain.disconnect();	// Ignore the AI thread's signal of its move.
      brain.stop();		// Stop the AI thread ASAP.
   }
}

void KCubeBoxWidget::stopActivities()
{
   qDebug() << "STOP ACTIVITIES";
   if (animationTimer->isActive() || m_waitingForStep) {
      m_waitingForStep = false;
      stopAnimation (true);	// If moving, do the rest of the steps ASAP.
   }
   if (brain.isActive()) {
      qDebug() << "BRAIN IS ACTIVE";
      brain.stop();		// Keep Stop enabled, for the blink animation.
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
	cubes.at (index)->setOwner ((Player) owner);
	cubes.at (index)->setValue (value);
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
   setCursor (Qt::BusyCursor);
}

void KCubeBoxWidget::setNormalCursor()
{
   setCursor (Qt::PointingHandCursor);
}

bool KCubeBoxWidget::checkClick (int x, int y, bool isClick)
{
   if (isClick && m_waitingForStep) {
      m_waitingForStep = false;
      animationTimer->start();
      return false;
   }

   if(isActive())
      return false;

   // make the game start when computer player is player one and user clicks
   int index = x * m_side + y;
   qDebug() << "CLICK" << x << y << "index" << index;
   // IDW test. if (isClick && m_currentPlayer == One && computerPlOne)
   if (isClick && (m_playerWaiting != Nobody)) {
      checkComputerplayer (m_playerWaiting);
      return false;
   }
   else if (m_currentPlayer == cubes.at (index)->owner() ||
            cubes.at (index)->owner() == Nobody) {
      doMove (index);
      return true;
   }
   return false;
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
   connect(&brain,SIGNAL(done(int)),SLOT(computerMoveDone(int)));

   setNormalCursor();

   emit playerChanged(One);

   brain.startStats();
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
      connect (cube, SIGNAL (clicked(int,int,bool)),
                     SLOT   (checkClick(int,int,bool)));
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
   for (int x = 0; x < m_side; x++) {
      for (int y = 0; y < m_side; y++) {
	 int index = x * m_side + y;
         cubes.at (index)->move (
                            topLeft.x() + hairline + x * (cubeSize + hairline),
                            topLeft.y() + hairline + y * (cubeSize + hairline));
         cubes.at (index)->resize (cubeSize, cubeSize);
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

void KCubeBoxWidget::doMove (int index)
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
      brain.postMove (m_currentPlayer, index, m_side); // IDW test.
   }
   m_steps->clear();
   m_gameHasBeenWon = m_box->doMove (m_currentPlayer, index, m_steps);
   m_box->printBox(); // IDW test.
   qDebug() << "GAME WON?" << m_gameHasBeenWon << "STEPS" << (* m_steps);
   if (m_steps->count() > 1) {
      emit startedMoving();	// This will be a stoppable animation.
   }
   currentAnimation = cascadeAnimation;
   doStep();
}

void KCubeBoxWidget::doStep()
{
   // Re-draw all cubes affected by a move, proceeding one step at a time.
   int index;
   bool startStep = true;
   do {
      if (! m_steps->isEmpty()) {
         index = m_steps->takeFirst();		// Get a cube to be re-drawn.

         // Check if the player wins at this step (no more cubes are re-drawn).
         if (index == 0) {
            qDebug() << "Win for player" << m_currentPlayer << "at this step.";
            emit stoppedMoving();
            currentAnimation = None;
	    brain.dumpStats();	// IDW test.
            emit playerWon ((int) m_currentPlayer);	// Trigger a popup.
	    stopActivities();				// End the move display.
            reset();					// Clear the cube box.
            return;
         }

         // Update the view of a cube, either immediately or via animation.
         startStep = (index > 0);		// + -> increment, - -> expand.
         index = startStep ? (index - 1) : (-index - 1);
         int value = cubes.at (index)->value();
         int max   = m_box->maxValue (index);
         if (startStep) {				// Add 1 and take.
            cubes.at (index)->setOwner (m_currentPlayer);
            cubes.at (index)->setValue (value + 1);
            if ((value >= max) && (currentAnimation != None)) {
               cubes.at (index)->setDark();
            }
         }
         else if (currentAnimation == None) {		// Decrease immediately.
            cubes.at (index)->setValue (value - max);
            cubes.at (index)->setNeutral();		// Maybe user hit Stop.
         }
         else {
            startAnimation (currentAnimation, index);	// Animate and decrease.
         }
      }
      else {
         // Views of the cubes at all steps of the move have been updated.
         qDebug() << "End of move for player" << m_currentPlayer;
         emit stoppedMoving();
         currentAnimation = None;
         KCubeWidget::enableClicks(true);
         changePlayer();
         return;
      }
   } while (startStep || (currentAnimation == None));
}

void KCubeBoxWidget::startAnimation (AnimationType type, int index)
{
   int interval = 0;
   currentAnimation = type;
   m_index = index;
   switch (currentAnimation) {
   case None:
      animationCount = 0;
      return;				// Should never happen.
      break;
   case Hint:
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
   if (m_pauseForStep &&
       (currentAnimation != Hint) && (currentAnimation != ComputerMove)) {
      m_waitingForStep = true;		// Timer will start on next click.
      return;
   }
   animationTimer->start();
   // IDW test. qDebug() << "START ANIMATION" << m_index / m_side << m_index % m_side << "type" << currentAnimation << "count" << animationCount << "interval" << interval;
}

void KCubeBoxWidget::nextAnimationStep()
{
   animationCount--;
   if (animationCount < 1) {
      stopAnimation (false);		// Finish normally.
      return;
   }
   switch (currentAnimation) {
   case None:
      return;				// Should not happen (see doStep()).
      break;
   case Hint:
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
   // IDW test. qDebug() << "KCubeBoxWidget::scatterDots (" << step << ")";
   Player player = m_currentPlayer;
   int d = m_side - 1;
   int x = m_index / m_side;
   int y = m_index % m_side;
   if (x > 0) cubes.at (m_index - m_side)->migrateDot (+1,  0, step, player);
   if (x < d) cubes.at (m_index + m_side)->migrateDot (-1,  0, step, player);
   if (y > 0) cubes.at (m_index - 1)     ->migrateDot ( 0, +1, step, player);
   if (y < d) cubes.at (m_index + 1)     ->migrateDot ( 0, -1, step, player);
}

void KCubeBoxWidget::stopAnimation (bool completeAllSteps)
{
   // Animation ended normally or the user hit Stop (completeAllSteps = true).
   animationTimer->stop();
   cubes.at (m_index)->setNeutral();
   switch (currentAnimation) {
   case Hint:
   case ComputerMove:
      emit stoppedThinking();
      if (currentAnimation == ComputerMove) {
         checkClick (m_index / m_side, m_index % m_side, false);
      }
      break;
   case RapidBlink:
   case Darken:
   case Scatter:
      cubes.at (m_index)->setValue (cubes.at (m_index)->value() -
                                              m_box->maxValue (m_index));

      if (completeAllSteps) {		// If called by stopActivities().
         qDebug() << "Stop at index" << m_index << "anim" << currentAnimation;
         currentAnimation = None;	// Do the rest of the steps immediately.
      }
      doStep();				// Go and update the next cube.
      break;
   case None:
      break;				// Should never happen.
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

void KCubeBoxWidget::showPopup (const QString & message)
{
   QFont f;
   // f.setPixelSize ((int) (cubes.at (0)->height() * 0.2 + 0.5));
   f.setPixelSize ((int) (height() * 0.02 + 0.5));
   f.setWeight (QFont::Bold);
   f.setStretch (QFont::Expanded);
   // m_popup->setBrush (QColor (0.5, 1.0, 1.0, 1.0));
   // m_popup->setStyleSheet("QLabel { background-color : none; color : rgba(255, 255, 255, 50%); }");
   m_popup->setStyleSheet("QLabel { color : rgba(255, 255, 255, 75%); }");
   m_popup->setFont (f);

   m_popup->setText (message);
   m_popup->move ((this->width()  - m_popup->width()) / 2,
                  (this->height() - m_popup->height()) / 2 +
		  (cubes.at (0)->height() / 5)); 
   m_popup->raise();
   m_popup->show();
}

void KCubeBoxWidget::hidePopup()
{
   m_popup->hide();
}

#include "kcubeboxwidget.moc"
