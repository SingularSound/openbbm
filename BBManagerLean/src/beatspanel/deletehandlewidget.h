#ifndef DELETEHANDLEWIDGET_H
#define DELETEHANDLEWIDGET_H

#include <QWidget>
#include <QPushButton>

class DeleteHandleWidget : public QWidget
{
   Q_OBJECT
public:
   explicit DeleteHandleWidget(QWidget *parent = nullptr, bool song = 0);
   void hideButton();
   static const int PREFERRED_WIDTH;

signals:
   void sigDeleteClicked();
   void sigIsDisabled(bool);
   void sigSubWidgetClicked();

public slots:
   void slotDeleteClicked();
   void slotIsDisabled(bool disabled);

protected:
   virtual void paintEvent(QPaintEvent * event);

private:
    bool       m_type;
   QPushButton *mp_DeleteButton;

};

#endif // DELETEHANDLEWIDGET_H
