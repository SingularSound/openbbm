#ifndef VMSCREEN_H
#define VMSCREEN_H

#include <QWidget>
#include "../player/player.h"

class VMScreen : public QWidget
{
   Q_OBJECT
public:

   explicit VMScreen(QWidget *parent = nullptr);
   ~VMScreen();

signals:

public slots:
   void slotTimeSigNumChanged(int num);
   void slotBeatInBarChanged(int beatInBar);
   void slotSetStarted(bool started);
   void slotSetPart(int part);
   void slotTestPart();
   void slotIncrementTick();


protected:
    void paintEvent(QPaintEvent *event);

private:

   qreal m_sigNum;
   qreal m_beatInBar;
   qreal m_started;
   Player::partEnum m_part;


   QTimer *mp_testTimer;

};

#endif // VMSCREEN_H
