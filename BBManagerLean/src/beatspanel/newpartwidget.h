#ifndef NEWPARTWIDGET_H
#define NEWPARTWIDGET_H

#include <QFrame>
#include <QPushButton>

class NewPartWidget : public QFrame
{
   Q_OBJECT
public:
   explicit NewPartWidget(QWidget *parent = nullptr, int partRow=0);

signals:
   void sigAddPartToRow(int row);
   void sigSubWidgetClicked();

public slots:
   void slotButtonClicked();

private:
   int m_PartRow;

   QPushButton *mp_NewButton;
};

#endif // NEWPARTWIDGET_H
