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
#include <QLabel>
#include <QFrame>
#include <QVBoxLayout>
#include <QStyleOption>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QDebug>

#include "virtualmachinepanel.h"

VirtualMachinePanel::VirtualMachinePanel(QWidget *parent) :
   QWidget(parent)
{
   //this->setStyleSheet("{ background-color: red;}");
   mp_ParentContainer = parent;
   QVBoxLayout * p_VBoxLayout = new QVBoxLayout();
   setLayout(p_VBoxLayout);
   p_VBoxLayout->setContentsMargins(0,0,0,0);

   QLabel * p_Title = new QLabel(this);
   p_Title->setText(tr("Virtual Machine"));
   p_Title->setObjectName("titleBar");
   p_VBoxLayout->addWidget(p_Title, 0);

   QWidget *p_MainContainer = new QWidget(this);
   p_VBoxLayout->addWidget(p_MainContainer, Qt::AlignCenter);
   p_MainContainer->setLayout(new QGridLayout());
   p_MainContainer->setObjectName("VirtualMachine");
   QFrame *p_Dummy = new QFrame(p_MainContainer);

   QImage vm(":/images/images/VM.png");

   m_aspectRatio = double(vm.height())/double(vm.width());
   m_lastHeight = this->height();
   m_lastWidth = this->width();

   m_minHeight = vm.height()/ 8;
   m_maxHeight = vm.height();
   m_minWidth = vm.width()/8;
   m_maxWidth = vm.width();

   p_Dummy->setMinimumWidth(m_minWidth);
   p_Dummy->setMaximumWidth(m_maxWidth);
   p_Dummy->setMinimumHeight(m_minHeight);
   p_Dummy->setMaximumHeight(m_maxHeight);

   static_cast<QGridLayout*>(p_MainContainer->layout())->addWidget(p_Dummy,0,0);
   static_cast<QGridLayout*>(p_MainContainer->layout())->setRowStretch(0,false);
   static_cast<QGridLayout*>(p_MainContainer->layout())->setRowStretch(1,false);
   static_cast<QGridLayout*>(p_MainContainer->layout())->setColumnStretch(0,false);
   static_cast<QGridLayout*>(p_MainContainer->layout())->setColumnStretch(1,false);


   // Pedal control push button
   mp_Pedal = new QPushButton(this);
   mp_Pedal->setObjectName(QStringLiteral("VMPedal"));
   mp_Pedal->setFlat(true);
   mp_Pedal->setAutoFillBackground(false);
   mp_Pedal->setText(nullptr);
   mp_Pedal->setToolTip(tr("Press pedal"));
   mp_Pedal->move(VM_PEDAL_POS_X, VM_PEDAL_POS_Y);
   mp_Pedal->show();
   connect(mp_Pedal, SIGNAL(pressed()), this, SLOT(pedalPressed()));
   connect(mp_Pedal, SIGNAL(released()), this, SLOT(pedalReleased()));


   // Special effect push button creation
   mp_Effect = new QPushButton(this);
   mp_Effect->setObjectName(QStringLiteral("VMEffect"));
   mp_Effect->setFlat(true);
   mp_Effect->setAutoFillBackground(false);
   mp_Effect->setText(nullptr);
   mp_Effect->setToolTip(tr("Play Accent Hit"));
   mp_Effect->move(VM_EFFECT_POS_X, VM_EFFECT_POS_Y);
   mp_Effect->show();
   connect(mp_Effect, SIGNAL(pressed()), this, SLOT(effect()));

   mp_Screen = new VMScreen(this);
   mp_Screen->setObjectName(QStringLiteral("VMScreen"));
   mp_Screen->move(VM_SCREEN_POS_X, VM_SCREEN_POS_X);
   mp_Screen->show();


   this->resize(width() * 4,height());
}

// Required to apply stylesheet
void VirtualMachinePanel::paintEvent(QPaintEvent *event)
{
   QPainter painter(this);

   painter.fillRect(0,0,this->size().width(),this->size().height(),Qt::white);


   int h = height() - (2 * VM_Y_BORDER_SIZE + VM_Y_HEADER_OFFSET);
   int w = width() -  (2 * VM_X_BORDER_SIZE);

   int y_offset;
   int x_offset;

   double sizeRatio;
   double newAspectRatio = double(h)/ double(w);

   if (newAspectRatio >= m_aspectRatio){

      x_offset = 0;
      y_offset = (newAspectRatio - m_aspectRatio) * width() / 2;
      // Calculation based on the current width
      sizeRatio = double(w) / double(m_maxWidth);
      painter.drawPixmap(VM_X_BORDER_SIZE,
                         y_offset + VM_Y_BORDER_SIZE + VM_Y_HEADER_OFFSET,
                         QPixmap(":/images/images/VM.png").scaledToWidth(
                            w,
                            Qt::SmoothTransformation));

   } else {
      // Calculation based on the current height
      sizeRatio = double(h)/double(m_maxHeight);
      x_offset = ((m_aspectRatio - newAspectRatio) * double(width())) / 4;
      y_offset = 0;


      painter.drawPixmap(x_offset + VM_X_BORDER_SIZE,
                         VM_Y_BORDER_SIZE + VM_Y_HEADER_OFFSET,
                         QPixmap(":/images/images/VM.png").scaledToHeight(
                            h,
                            Qt::SmoothTransformation));
   }

   /* Resize and move the pedal press button */
   mp_Pedal->resize(sizeRatio * double(VM_PEDAL_WIDTH),sizeRatio * double(VM_PEDAL_HEIGHT));
   mp_Pedal->move(
            x_offset + VM_X_BORDER_SIZE + sizeRatio * VM_PEDAL_POS_X,
            y_offset + VM_Y_BORDER_SIZE + VM_Y_HEADER_OFFSET + sizeRatio * VM_PEDAL_POS_Y);

   /* Resize and remove the effect switch button */
   mp_Effect->resize(sizeRatio * double(VM_EFFECT_WIDTH),sizeRatio * double(VM_EFFECT_HEIGHT));
   mp_Effect->move(
            x_offset + VM_X_BORDER_SIZE + sizeRatio * VM_EFFECT_POS_X,
            y_offset + VM_Y_BORDER_SIZE + VM_Y_HEADER_OFFSET + sizeRatio * VM_EFFECT_POS_Y);


   mp_Screen->resize(sizeRatio * double(VM_SCREEN_WIDTH),sizeRatio * double(VM_SCREEN_HEIGHT));
   mp_Screen->move(
            x_offset + VM_X_BORDER_SIZE + sizeRatio * VM_SCREEN_POS_X,
            y_offset + VM_Y_BORDER_SIZE + VM_Y_HEADER_OFFSET + sizeRatio * VM_SCREEN_POS_Y);


   QWidget::paintEvent(event);
}

void VirtualMachinePanel::pedalPressed(void)
{
   // If less than 250ms since release of button, it's a double tap
   // Need to stop timer before emitting signal because emit is blocking
   if (m_pedalTimer.isActive() && m_pedalCounter == 1) {
      m_pedalTimer.stop();
      emit sigPedalDoubleTap();
   } else {
      m_pedalTimer.stop();
      emit sigPedalPress();
   }
   m_pedalCounter = 0;

   connect(&m_pedalTimer, SIGNAL(timeout()), this, SLOT(pedalLongPress()));
   m_pedalTimer.setSingleShot(true);


   m_pedalTimer.start(250);
}

void VirtualMachinePanel::pedalReleased(void)
{
   if (m_pedalTimer.isActive()) {
      m_pedalCounter++;
   }
   m_pedalTimer.stop();
   disconnect(&m_pedalTimer, SIGNAL(timeout()), this, SLOT(pedalLongPress()));

   emit sigPedalRelease();

   m_pedalTimer.setSingleShot(true);
   m_pedalTimer.start(250);
}

void VirtualMachinePanel::pedalLongPress(void)
{
   emit sigPedalLongPress();
}

void VirtualMachinePanel::effect(void)
{
   emit sigEffect();
}

