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

#include "ai_main.h"
#include "ai_box.h"
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
          m_state         (NotStarted),
          m_busyState     (NA),
          m_waitingState  (Nil),
	  m_side          (d),
	  m_currentPlayer (One),
	  m_popup         (new QLabel (this)),
	  m_ignoreComputerMove    (false)
{
   qDebug() << "CONSTRUCT KCubeBoxWidget: side" << m_side;
   m_box = new AI_Box  (this, m_side);
   m_ai  = new AI_Main (this, m_side);
   m_steps = new QList<int>;
   cubes.clear();
   init();
   m_gameHasBeenWon   = false;
   // IDW DELETE. m_computerMoveType = Hint;
   m_waitingForStep   = false;
   m_playerWaiting    = computerPlOne ? One : Nobody;
   m_state        = computerPlOne ? Waiting : HumanToMove;
   m_waitingState = computerPlOne ? ComputerToMove : Nil;
}

KCubeBoxWidget::~KCubeBoxWidget()
{
   if (isActive()) {
      stopActivities();
   }
   delete m_steps;
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

  m_ai->setSkill (Prefs::skill1(), Prefs::kepler1(), Prefs::newton1(),
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
   // IDW TODO - Delete? stopActivities(); shutdown() here only (except Quit)?

   foreach (KCubeWidget * cube, cubes) {
      cube->reset();
   }

   m_box->clear();

   KCubeWidget::enableClicks(true);
   currentAnimation = None;

   m_currentPlayer = One;

   m_waitingForStep = false;
   m_playerWaiting  = computerPlOne ? One : Nobody;
   m_state          = computerPlOne ? Waiting : HumanToMove;
   m_waitingState   = computerPlOne ? ComputerToMove : Nil;
   if (computerPlOne) {
      emit buttonChange (true, false, i18n("Start computer move"));
   }
   else {
      emit buttonChange (false);
   }

   emit playerChanged(One);
   // When re-starting, WAIT FOR A CLICK.
   // checkComputerplayer(One); // Enable this line for IDW high-speed test.

   m_ai->startStats();
}

int KCubeBoxWidget::undoRedo (int actionType)
{
   // IDW TODO - A user-friendly way to defer undo/redo if computer is moving.
   // IDW test. if (isActive())
      // IDW test. return -1;
   if (isActive()) {
      stopActivities();
      m_ignoreComputerMove = true;
   }

   Player oldPlayer = m_currentPlayer;

   bool isAI = false;
   bool moreToDo = (actionType < 0) ?
      m_box->undoPosition (m_currentPlayer, isAI) :
      m_box->redoPosition (m_currentPlayer, isAI);

   // Update the cube display after an undo or redo.
   int index = 0;
   foreach (KCubeWidget * cube, cubes) {
       cube->setOwner (m_box->owner (index));
       cube->setValue (m_box->value (index));
       index++;
   }
   if (oldPlayer != m_currentPlayer)
      emit playerChanged (m_currentPlayer);

   if ((actionType > 0) && (! moreToDo)) {	// If end of Redo's: restart AI.
      checkComputerplayer (m_currentPlayer);
   }
   return (moreToDo ? 1 : 0);
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
	    // IDW DELETE. showPopup (i18n ("Click anywhere to continue"));
            m_state = Waiting;
            m_waitingState = ComputerToMove;
            if (computerPlOne && computerPlTwo) {
                emit buttonChange (true, false, i18n("Continue game"));
            }
            else {
                emit buttonChange (true, false, i18n("Start computer move"));
            }
            return;
         }
	 else {
            // IDW DELETE. hidePopup();
         }
      }
      m_playerWaiting = Nobody;
      KCubeWidget::enableClicks (false);

      // IDW DELETE. m_computerMoveType = ComputerMove;
      emit startedThinking();
      emit buttonChange (true, true, i18n("Stop computing"));	// Red look.
      qDebug() << "Calling m_ai->getMove() for player" << player;
      t.start(); // IDW test.
      m_state = ComputingMove;	// IDW TODO - Set null sub-states ???
      m_ignoreComputerMove = false;
      m_ai->getMove (player, m_box);
   }
}

void KCubeBoxWidget::getHint()
{
   if (isActive())
      return;

   // IDW DELETE. m_computerMoveType = Hint;
   m_state = Busy;
   m_busyState = ComputingHint;	// IDW TODO - m_waitingState = Nil; ???
   emit startedThinking();
   emit buttonChange (true, true, i18n("Stop computing"));	// Red look.
   m_ignoreComputerMove = false;
   m_ai->getMove (m_currentPlayer, m_box);
}

void KCubeBoxWidget::computerMoveDone (int index)
{
   // We do not care if we interrupted the computer.  It was probably
   // taking too long, so we will just take the best move it had so far.

   if (m_ignoreComputerMove) {
      // Whatever set this will start a new game or close down KJumpingCube.
      m_ignoreComputerMove = false;
      return;
   }
   if ((index < 0) || (index >= (m_side * m_side))) {
      emit stoppedThinking();
      emit buttonChange (false);				// Inactive.
      KMessageBox::sorry (this,
                          i18n ("The computer could not find a valid move."));
      // IDW TODO - What to do about state values ???
      return;
   }

   // IDW DELETE. if (m_computerMoveType == ComputerMove) {
   // }
   // else if (m_computerMoveType == Hint) {
   // }

   // IDW TODO - Check states.  ComputingMove, Busy, Waiting.
   // IDW TODO - Set appropriate button states and captions.
   switch (m_state) {
   case ComputingMove:
      qDebug() << "TIME of MOVE" << t.elapsed();
      qDebug() << "===========================================================";
      m_state = Busy;
      m_busyState = ShowingMove;
      startAnimation (ComputerMove, index);
      break;
   case Busy:
      qDebug() << "HINT FOR PLAYER" << m_currentPlayer
               << "X" << index / m_side << "Y" << index % m_side;
      m_busyState = ShowingHint;
      startAnimation (Hint, index);
      break;
   case Waiting:
      qDebug() << "TIME of MOVE" << t.elapsed();
      qDebug() << "===========================================================";
      startAnimation (ComputerMove, index);
      break;
   default:
      return;
      break;
   }

   // IDW TODO - Re-word this comment.
   // Blink the cube to be moved (twice).  When done, emit stoppedThinking()
   // and simulate a click if it is a ComputerMove, not a Hint.
   emit buttonChange (true, true, i18n("Stop moving"));
   // IDW test DELETE. startAnimation (m_computerMoveType, index);

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
      m_box   = new AI_Box (this, d);
      qDebug() << "AI_Box CONSTRUCTED by KCubeBoxWidget::setDim()";
      m_side  = d;
      initCubes();
      // IDW TODO - Crashed in slot computerMoveDone(int). Starts animation with
      //            too large an index? Going from larger to smaller box.
      // Should we do setDim and return and do the rest of loadSettings later?
      reset();
   }
}

void KCubeBoxWidget::setComputerplayer(Player player,bool flag)
{
   if(player==One)
      computerPlOne=flag;
   else if(player==Two)
      computerPlTwo=flag;
   // IDW TODO - If current player is not computer, m_playerWaiting = Nobody.
   if ((player == m_currentPlayer) && (! flag)) {
      m_playerWaiting = Nobody;	// Current player is human.
   }
}

void KCubeBoxWidget::shutdown()
{
   // Shut down gracefully, avoiding a possible crash when the user hits Quit.
   if (animationTimer->isActive()) {
      animationTimer->stop();	// Stop move or hint animation immediately.
   }
   if (m_ai->isActive()) {
      m_ai->stop();		// Stop the AI thread ASAP.
      m_ignoreComputerMove = true;
   }
}

void KCubeBoxWidget::stopActivities()
{
   qDebug() << "STOP ACTIVITIES";
   if (animationTimer->isActive() || m_waitingForStep) {
      m_waitingForStep = false;
      stopAnimation (true);	// If moving, do the rest of the steps ASAP.
   }
   if (m_ai->isActive()) {
      qDebug() << "BRAIN IS ACTIVE";
      m_ai->stop();		// Keep Stop enabled, for the blink animation.
   }
}

void KCubeBoxWidget::saveProperties (KConfigGroup & config)
{
   // IDW TODO - Save settings for computer player(s), animation, etc.
   //            Is Undo right for interrupted animation????
   //            Do we need Undo for interrupted computer move?
   //            What happens to the signal when the computer move ends?
   if (isMoving()) {
      stopActivities();
      undoRedo (-1);		// Undo incomplete move.
   }
   else if (m_ai->isActive()) {
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
  qDebug() << "Entering KCubeBoxWidget::readProperties ...";
  QStringList list;
  QString     key;
  int         owner, value, maxValue;

  // Dimension must be 3 to 15 (see definition in ai_box.h).
  int cubeDim = config.readEntry ("CubeDim", minSide);
  if ((cubeDim < minSide) || (cubeDim > maxSide)) {
     KMessageBox::sorry (this, i18n("The file's cube box size is outside "
                                    "the range %1 to %2. It will be set to %1.")
                                    .arg(minSide).arg(maxSide));
     cubeDim = 3;
  }
  m_side = 1;					// Create a new cube box.
  setDim (cubeDim);
  reCalculateGraphics (width(), height());	// Re-draw the cube box.

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
   qDebug() << "Leaving KCubeBoxWidget::readProperties ...";
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
   // IDW TODO - This needs a re-write. Do we need more than just REAL click?
   //            Button will handle m_waitingForStep, start game, continue game:
   //            leaving just doMove() for human player or computer player.
   // IDW TODO - Write a new mouse-click event for KCubeBoxWidget? Remove the
   //            one that KCubeWidget has?
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

void KCubeBoxWidget::buttonClick()
{
   qDebug() << "BUTTON CLICK seen";
   switch (m_state) {
   case Waiting:
      switch (m_waitingState) {
      case WaitingToStep:
         animationTimer->start();
         m_waitingForStep = false;
         break;
      case ComputerToMove:
         checkComputerplayer (m_playerWaiting);
         break;
      default:
         break;
      }
      break;
   case Busy:
      break;
   case ComputingMove:
      qDebug() << "BRAIN IS ACTIVE";
      m_ai->stop();		// Keep Stop enabled, for the blink animation.
      break;
   default:
      emit buttonChange (false);
      break;
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
   else if(m_ai->isActive())
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
   connect(m_ai,SIGNAL(done(int)),SLOT(computerMoveDone(int)));

   setNormalCursor();

   emit playerChanged(One);

   m_ai->startStats();
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

/* ***************************************************************** **
**                   other private functions                         **
** ***************************************************************** */

void KCubeBoxWidget::doMove (int index)
{
   // If a move has not finished yet do not do another move.
   if (isActive())
      return;

   // IDW TODO. Allow undo and redo of computer moves.
   //           Done, but needs further testing, esp. for race conditions
   //           and the use of m_ignoreComputerMove.

   bool computerMove = ((computerPlOne && m_currentPlayer == One) ||
                        (computerPlTwo && m_currentPlayer == Two));

   // Make a copy of the position and player to move for the Undo function.
   m_box->copyPosition (m_currentPlayer, computerMove);

   if (! computerMove) { // IDW test. Record a human move in the statistics.
      m_ai->postMove (m_currentPlayer, index, m_side); // IDW test.
   }
   emit newMove();		// GUI must update Undo and Redo actions.
   m_steps->clear();
   m_gameHasBeenWon = m_box->doMove (m_currentPlayer, index, m_steps);
   m_box->printBox(); // IDW test.
   qDebug() << "GAME WON?" << m_gameHasBeenWon << "STEPS" << (* m_steps);
   if (m_steps->count() > 1) {
      emit startedMoving();	// This will be a stoppable animation.
      currentAnimation = cascadeAnimation;
   }
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
            emit buttonChange (false);			// Inactive.
            currentAnimation = None;
	    m_ai->dumpStats();	// IDW test.
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
	    if (! m_pauseForStep) {
	       emit buttonChange (true, true, i18n("Stop animation"));
	    }
         }
      }
      else {
         // Views of the cubes at all steps of the move have been updated.
         qDebug() << "End of move for player" << m_currentPlayer;
         emit stoppedMoving();
         emit buttonChange (false);			// Inactive.
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
      m_waitingForStep = true;
      m_state = Waiting;		// Timer will start on next buttonClick.
      m_waitingState = WaitingToStep;
      emit buttonChange (true, false, i18n("Show next step"));
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
      emit buttonChange (false);	// Inactive.
      if (currentAnimation == ComputerMove) {
         // IDW TODO - Handle Busy states.
         // IDW TODO - Handle Waiting states.
         // IDW TODO - Only go to next move if BusyState:ShowingMove.
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
