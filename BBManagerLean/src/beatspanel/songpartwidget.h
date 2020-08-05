#ifndef SONGPARTWIDGET_H
#define SONGPARTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QModelIndex>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QFile>

#include "songfolderviewitem.h"
#include "partcolumnwidget.h"
#include "../dialogs/loopCountDialog.h"

class MoveHandleWidget;
class DeleteHandleWidget;
class LoopCountDialog;

class SongPartWidget : public SongFolderViewItem
{
   Q_OBJECT
public:
   explicit SongPartWidget(BeatsProjectModel *p_Model, QWidget *parent = nullptr, QString filepath = "");

   void populate(QModelIndex const& modelIndex);
   void updateLayout();
   void dataChanged(const QModelIndex &left, const QModelIndex &right);
   // Hack for header column width
   int headerColumnWidth(int columnIndex);
   void updateMinimumSize();
   void parentAPBoxStatusChanged();
   void updateTransMain(bool hasOutro = false);
   void updateOnDeletedChild();

   // Accessors
   void setIntro(bool intro);
   bool isIntro() const;
   void setOutro(bool outro);
   bool isOutro() const;
   void setLast(bool last);
   bool isLast() const;

   PartColumnWidget *getChildItemAt(int i);
   QString readUpdatePartName(char CRUD, int start = 0, int end = 0);
signals:
   void sigIsFirst(bool first);
   void sigIsLast(bool last);
   void sigIsAlone(bool alone);
   void sigSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex);
   void sigUpdateAP();
   void sigMoveUp(int start, int end);
   void sigMoveDown(int start, int end);

public slots:
   void deleteButtonClicked();
   void slotIsFirst(bool first);
   void slotIsLast(bool last);
   void slotIsAlone(bool alone);
   void slotOrderChanged(int number);
   void slotTitleChangeByUI();
   void slotMovePartUpClicked();
   void slotMovePartDownClicked();
   void slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId);


protected:
   virtual void paintEvent(QPaintEvent * event);

private:
   QWidget *mp_ContentPanel;
   MoveHandleWidget *mp_MoveHandleWidget;
   DeleteHandleWidget *mp_DeleteHandleWidget;
   LoopCountDialog *mp_LoopCount;
   QList<PartColumnWidget*> *mp_PartColumnItems;

   bool m_intro;
   bool m_outro;
   bool m_last;
   bool m_playingInternal;
   bool m_validInternal;
   QString partName;
   QStringList partsNames;
   QString nameFile;
   QLineEdit *mp_Title;
   QVBoxLayout *l = new QVBoxLayout();

   typedef enum {
       STYLE_NONE,
       STYLE_PLAYING,
       STYLE_INVALID,
       NUMBER_OF_KEY
   }selectedStyleSheet_t;

   selectedStyleSheet_t m_selectedStyleSheet;

};

#endif // SONGPARTWIDGET_H
