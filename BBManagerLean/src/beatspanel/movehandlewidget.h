#ifndef MOVEHANDLEWIDGET_H
#define MOVEHANDLEWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QString>

class MoveHandleWidget : public QWidget
{
   Q_OBJECT
public:
   explicit MoveHandleWidget(QWidget *parent = nullptr);
   void hideArrows();
   void setLabelText(const QString & text);
   static const int PREFERRED_WIDTH;

signals:
   void sigUpClicked();
   void sigDownClicked();
   void sigIsFirst(bool first);
   void sigIsLast(bool last);
   void sigSubWidgetClicked();

public slots:
   void upButtonClicked();
   void downButtonClicked();
   void slotIsFirst(bool first);
   void slotIsLast(bool last);
protected:
   virtual void paintEvent(QPaintEvent * event);

private:
   QPushButton *mp_UpButton;
   QPushButton *mp_DownButton;
   QLabel *mp_Label;


};

#endif // MOVEHANDLEWIDGET_H
