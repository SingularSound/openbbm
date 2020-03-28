#ifndef VIRTUALMACHINEPANEL_H
#define VIRTUALMACHINEPANEL_H

#include <QWidget>
#include <QFrame>
#include <QPushButton>
#include <QTimer>

#include "vmscreen.h"

#define VM_PEDAL_POS_X          72
#define VM_PEDAL_POS_Y          665
#define VM_PEDAL_WIDTH          634
#define VM_PEDAL_HEIGHT         324

#define VM_EFFECT_POS_X         70
#define VM_EFFECT_POS_Y         1080
#define VM_EFFECT_WIDTH         342
#define VM_EFFECT_HEIGHT        462


// 11-9 ratio
#define VM_SCREEN_POS_X         105
#define VM_SCREEN_POS_Y         93
#define VM_SCREEN_WIDTH         275
#define VM_SCREEN_HEIGHT        225

#define VM_Y_BORDER_SIZE        0
#define VM_X_BORDER_SIZE        0
#define VM_Y_HEADER_OFFSET      22

class VirtualMachinePanel : public QWidget
{
   Q_OBJECT
public:
   explicit VirtualMachinePanel(QWidget *parent = nullptr);

   inline VMScreen * getVMScreen(){return mp_Screen;}

signals:
    void sigPedalPress(void);
    void sigPedalRelease(void);
    void sigPedalDoubleTap(void);
    void sigPedalLongPress(void);
    void sigEffect(void);

public slots:
    void pedalPressed(void);
    void pedalReleased(void);
    void pedalLongPress(void);
    void effect(void);

protected:
    virtual void paintEvent(QPaintEvent * event);

    QTimer m_pedalTimer;
    int m_pedalCounter;

private:
    double m_aspectRatio;

    QPushButton *mp_Pedal;
    QPushButton *mp_Effect;
    VMScreen *mp_Screen;


    int m_lastHeight;
    int m_lastWidth;

    int m_maxHeight;
    int m_minHeight;

    int m_maxWidth;
    int m_minWidth;

    int m_imgWidth;
    int m_imgHeight;
    QWidget *mp_ParentContainer;

};

#endif // VIRTUALMACHINEPANEL_H

