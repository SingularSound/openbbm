#ifndef BEATSPANEL_H
#define BEATSPANEL_H

#include <QWidget>
#include <QAbstractItemModel>
#include <QItemSelectionModel>
#include <QLabel>


class SongFolderView;

class BeatsPanel : public QWidget
{
   Q_OBJECT

public:
   explicit BeatsPanel(QWidget *parent = nullptr);
   void setModel(QAbstractItemModel * model);
   void setSelectionModel(QItemSelectionModel *selectionModel);
   QItemSelectionModel * selectionModel();
   QModelIndex rootIndex();
   void setRootIndex(const QModelIndex &root);


   QString getTitle() const;

   void setSongsEnabled(bool enabled);

   SongFolderView* songFolderView() { return mp_SongFolderView; }


signals:
   void sigSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex);

public slots:
   void setTitle(const QString &text);
   void changeSelection(const QModelIndex& index);

private slots:
   void slotOnRootIndexChanged(const QModelIndex &root);
   void slotSelectTrack(const QByteArray &trackData, int trackIndex, int typeId, int partIndex);
   void slotSetPlayerEnabled(bool enabled);

protected:
   virtual void paintEvent(QPaintEvent * event);

private:
   SongFolderView *mp_SongFolderView;
   QLabel *mp_Title;
};

#endif // BEATSPANEL_H
