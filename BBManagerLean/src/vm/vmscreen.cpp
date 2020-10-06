/*
  	This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound 
 	BeatBuddy Manager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "vmscreen.h"

#include <QGridLayout>
#include <QRect>
#include <QTimer>
#include <QPainter>
#include <QDebug>
#include <QStaticText>
#include <QtCore/qmath.h>
#ifdef Q_OS_MACX
#include <QPainterPath>
#endif
VMScreen::VMScreen(QWidget *parent) :
   QWidget(parent)
{

   m_sigNum = 8.0;
   m_beatInBar = 0.0;
   m_started = false;
   m_part = Player::stopped;

}

VMScreen::~VMScreen(){
}

void VMScreen::slotTimeSigNumChanged(int num)
{
   m_sigNum = (qreal) num;
   update();
}

void VMScreen::slotBeatInBarChanged(int beatInBar)
{
   m_beatInBar = (qreal) beatInBar;
   update();
}


void VMScreen::slotSetStarted(bool started)
{
   m_started = started;
   update();
}

void VMScreen::slotSetPart(int part)
{
   m_part = (Player::partEnum)part;
   update();
}

void VMScreen::slotTestPart()
{
   switch(m_part){
      case Player::stopped:
         m_part = Player::intro;
         break;
      case Player::intro:
      case Player::outro:
         m_part = Player::mainLoop;
         break;
      case Player::mainLoop:
         m_part = Player::drumFill;
         break;
      case Player::drumFill:
         m_part = Player::transFill;
         break;
      case Player::transFill:
         m_part = Player::pause;
         break;
      case Player::pause:
         m_part = Player::stopped;
         break;
      default:
         m_part = Player::stopped;
         break;
   }
   update();
}

void VMScreen::slotIncrementTick()
{
   m_beatInBar += 1.0;
   if(m_beatInBar >= m_sigNum){
      m_beatInBar = 0;
   }
   update();
}

void VMScreen::paintEvent(QPaintEvent * /* event */)
{
   // Calculate parameters
   qreal radius = ((qreal) height()) * 0.04;

   // divide width in time signature num
   qreal dwidth = (qreal) width();
   qreal tickWidth = dwidth / m_sigNum;


   QPainter painter(this);
   painter.setRenderHint(QPainter::Antialiasing,true);

   // draw background
   QRect bgndRect(0, 0, width(),height());
   
   QLinearGradient linearGradient(0, 0, height()-1, width()-1);
   switch(m_part){
      case Player::stopped:
         // static const Color BACKGROUND_BLUE = {165, 190, 230};
         linearGradient.setColorAt(0.0, QColor(215, 240, 255, 255));
         linearGradient.setColorAt(0.2, QColor(165, 190, 230, 255));
         linearGradient.setColorAt(1.0, QColor(100, 125, 165, 255));

         break;
      case Player::intro:
      case Player::outro:
         //static const Color BOX_RED = {255, 0, 0};
         linearGradient.setColorAt(0.0, QColor(255, 127, 127, 255));
         linearGradient.setColorAt(0.2, QColor(255,   0,   0, 255));
         linearGradient.setColorAt(1.0, QColor(127,   0,   0, 255));
         break;

      case Player::mainLoop:
         //static const Color BOX_GREEN = {0, 255, 0};
         linearGradient.setColorAt(0.0, QColor(127, 255, 127, 255));
         linearGradient.setColorAt(0.2, QColor(  0, 255,   0, 255));
         linearGradient.setColorAt(1.0, QColor(  0, 127,   0, 255));
         break;
      case Player::drumFill:
         //static const Color BOX_YELLOW = {255,255,0};
         linearGradient.setColorAt(0.0, QColor(255, 255, 127, 255));
         linearGradient.setColorAt(0.2, QColor(255, 255,   0, 255));
         linearGradient.setColorAt(1.0, QColor(127, 127,   0, 255));
         break;
      case Player::transFill:
         //static const Color BOX_WHITE  = {255,255,255};
         linearGradient.setColorAt(0.0, QColor(255, 255, 255, 255));
         linearGradient.setColorAt(0.2, QColor(240, 240, 240, 255));
         linearGradient.setColorAt(1.0, QColor(200, 200, 200, 255));
         break;
      case Player::pause:
         //static const Color BOX_BLACK = {0,0,0};
         linearGradient.setColorAt(0.0, QColor(127, 127, 127, 255));
         linearGradient.setColorAt(0.2, QColor( 50,  50,  50, 255));
         linearGradient.setColorAt(1.0, QColor(  0,   0,   0, 255));
         break;

      default:
         // static const Color BACKGROUND_BLUE = {165, 190, 230};
         linearGradient.setColorAt(0.0, QColor(215, 240, 255, 255));
         linearGradient.setColorAt(0.2, QColor(165, 190, 230, 255));
         linearGradient.setColorAt(1.0, QColor(100, 125, 165, 255));
   }

   painter.setBrush(linearGradient);
   painter.setPen(Qt::NoPen);
   painter.drawRoundedRect(bgndRect, radius, radius);

   // Write VM text, and make sure it is scaled properly with transform
#ifdef Q_OS_OSX
   QRect originalTextRect(0, 0, 55, 45);
#else
   QRect originalTextRect(0, 0, 44, 36);
#endif
   painter.setBrush(QBrush(Qt::black, Qt::SolidPattern));
   painter.setPen(QPen());
   painter.save();
   qreal scale= ((qreal) width()) / ((qreal) originalTextRect.width());
   painter.scale(scale, scale);
   painter.drawText(originalTextRect, Qt::AlignCenter, tr("Virtual\nMachine"));
   painter.restore();

   // draw tick (start is floored, end width is ceiled)
   if(m_started){

      painter.setBrush(QBrush(QColor(0,0,0,127), Qt::SolidPattern));
      painter.setPen(Qt::NoPen);

      QRect tickRect(QPoint((int) (tickWidth * m_beatInBar), 0), QPoint(qCeil(tickWidth * (m_beatInBar + 1.0))-1, height()-1));

      if(m_beatInBar <= 0.0){


         QRect tickHalfRect(QPoint((int) (tickWidth * (m_beatInBar + 0.5)), 0), QPoint(qCeil(tickWidth * (m_beatInBar + 1.0))-1, height()-1));
         QPainterPath path;
         path.setFillRule( Qt::WindingFill );
         path.addRoundedRect( tickRect, radius, radius );
         path.addRect( tickHalfRect ); // Right corners are not rounded
         painter.drawPath( path ); // Only left corners rounded


      } else if (m_beatInBar >= m_sigNum - 1.0){
         QRect tickHalfRect(QPoint((int) (tickWidth * m_beatInBar), 0), QPoint(qCeil(tickWidth * (m_beatInBar + 0.5))-1, height()-1));
         QPainterPath path;
         path.setFillRule( Qt::WindingFill );
         path.addRoundedRect( tickRect, radius, radius );
         path.addRect( tickHalfRect ); // Left corners are not rounded
         painter.drawPath( path ); // Only right corners rounded
      } else {
         painter.drawRect(tickRect);
      }
   }

}
