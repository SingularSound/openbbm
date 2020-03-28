#ifndef SONGPARTWIDGET_H
#define SONGPARTWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QModelIndex>
#include <QLineEdit>

#include "songfolderviewitem.h"
#include "../dialogs/loopCountDialog.h"

class MoveHandleWidget;
class DeleteHandleWidget;
class LoopCountDialog;

class SongPartWidget : public SongFolderViewItem
{
   Q_OBJECT
public:
   explicit SongPartWidget(BeatsProjectModel *p_Model, QWidget *parent = nullptr);

   void populate(QModelIndex const& modelIndex);
   void updateLayout();
   void dataChanged(const QModelIndex &left, const QModelIndex &right);
   // Hack for header column width
   int headerColumnWidth(int columnIndex);
   void updateMinimumSize();

   // Accessors
   void setIntro(bool intro);
   bool isIntro() const;
   void setOutro(bool outro);
   bool isOutro() const;

signals:
   void sigIsFirst(bool first);
   void sigIsLast(bool last);
   void sigIsAlone(bool alone);
   void sigSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex);

public slots:
   void deleteButtonClicked();
   void slotIsFirst(bool first);
   void slotIsLast(bool last);
   void slotIsAlone(bool alone);
   void slotOrderChanged(int number);
   void slotMovePartUpClicked();
   void slotMovePartDownClicked();
   void slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId);
   void slotLoopCountEntered();


protected:
   virtual void paintEvent(QPaintEvent * event);

private:
   QWidget *mp_ContentPanel;
   MoveHandleWidget *mp_MoveHandleWidget;
   DeleteHandleWidget *mp_DeleteHandleWidget;
   LoopCountDialog *mp_LoopCount;

   bool m_intro;
   bool m_outro;
   bool m_playingInternal;
   bool m_validInternal;

   typedef enum {
       STYLE_NONE,
       STYLE_PLAYING,
       STYLE_INVALID,
       NUMBER_OF_KEY
   }selectedStyleSheet_t;

   selectedStyleSheet_t m_selectedStyleSheet;

};

#endif // SONGPARTWIDGET_H
