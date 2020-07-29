#ifndef PARTCOLUMNWIDGET_H
#define PARTCOLUMNWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include "songfolderviewitem.h"
#include "copypastable.h"
#include "beatfilewidget.h"

class DropPanel : public QWidget, public CopyPaste::Pastable
{
    QStringList acceptedExtensions;
    QPushButton* butt;
    Q_OBJECT

signals:
    void onDrop(QDropEvent *event);

public:
    explicit DropPanel(QWidget* parent = nullptr, Qt::WindowFlags f = nullptr);
    void setExtension(const QString&);
    void setVisualizer(QPushButton*);
    void dragEnterEvent(QDragEnterEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    void enterEvent(QEvent *event);
    void leaveEvent(QEvent *event);
    bool paste();

};

class PartColumnWidget : public SongFolderViewItem
{
   Q_OBJECT
public:
   explicit PartColumnWidget(BeatsProjectModel *p_Model, QWidget *parent = nullptr);

   void setLabel(QString const& label);
   void changeToolTip(QString const& tooltip);

   void populate(QModelIndex const& modelIndex);
   void updateLayout();
   void dataChanged(const QModelIndex &left, const QModelIndex &right);
   // Hack for header column width
   int headerColumnWidth(int columnIndex);
   void updateMinimumSize();
   void parentAPBoxStatusChanged(int sigNum, bool hasMain);
   void updateAPText(bool hasTrans,bool hasMain,bool hasOutro, int idx,int sigNum, bool isLast=false);
   bool finitePart();

   // Accessor
   int maxFileCount();

   int getNumSignature();
   bool isPartEmpty();
   QList<BeatFileWidget*> *mp_BeatFileItems;

   void setPartEmpty(bool value);
signals:
   void sigIsMultiFileAddEnabled(bool first);
   void sigIsShuffleEnabled(bool first);
   void sigIsShuffleActivated(bool first);
   void sigSelectTrack(const QByteArray &trackData, int trackIndex, int typeId);
   void sigUpdateTran(bool hasTrans = false, bool hasOutro = false);
   void sigRowInserted();
   void sigRowDelete();

public slots:
   void endEditMidi(const QByteArray& data);
   void slotDrop(QDropEvent *event);
   bool slotAddButtonClicked(const QString &dropFileName = nullptr);
   void slotCreateNewFile();
   void slotShuffleButtonClicked(bool checked);
   void slotSelectTrack(const QByteArray &trackData, int trackIndex);
   void slotMainAPUpdated();

protected:
   virtual void paintEvent(QPaintEvent * event);
   virtual void rowsInserted(int start, int end);
   virtual void rowsRemoved(int start, int end);

private:

   static const int PADDING;

   void updatePanels();
   void updateShuffleData(const QModelIndex &index);

   int m_MaxFileCount;
   bool m_ShuffleEnabled;
   bool m_ShuffleActivated;
   bool m_intro;
   bool m_outro;
   bool justInserted=false;
   bool isEmpty = true;

   // No file panel
   DropPanel *mp_NoFilePanel;
   QLabel *mp_NFPLabel;
   QPushButton *mp_NFPAddButton;

   // Multi file panel
   DropPanel *mp_MultiFilePanel;
   QPushButton *mp_MFPShuffleButton;
   QPushButton *mp_MFPAddButton;

   QByteArray m_editingTrackData;


};

#endif // PARTCOLUMNWIDGET_H
