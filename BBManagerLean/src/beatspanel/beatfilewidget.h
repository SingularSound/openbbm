#ifndef BEATFILEWIDGET_H
#define BEATFILEWIDGET_H

#include <QtWidgets>
#include <functional>
#include "songfolderviewitem.h"
#include "../copypastable.h"

class DragButton : public QPushButton
{
    QTimer t;
    Q_OBJECT

signals:
    void drag();

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);

public:
    explicit DragButton(QWidget *parent = nullptr);

};

class BeatFileWidget : public SongFolderViewItem, public CopyPaste::Copyable, public CopyPaste::Pastable
{
   Q_OBJECT
public:
   explicit BeatFileWidget(BeatsProjectModel* p_Model, QWidget* parent = nullptr);

   void setLabel(QString const& label);

   void populate(QModelIndex const& modelIndex);
   void updateLayout();
   void dataChanged(const QModelIndex &left, const QModelIndex &right);
   // Hack for header column width
   int headerColumnWidth(int columnIndex);

   void dragEnterEvent(QDragEnterEvent *event);
   void dragLeaveEvent(QDragLeaveEvent *event);
   void dropEvent(QDropEvent *event);

   void enterEvent(QEvent *event);
   void leaveEvent(QEvent *event);
   void updateAPText(bool hasTrans, bool hasMain,bool hasOutro, int sigNum, bool lastpart = false);
   bool finiteMain();
   void setAsNew(bool value);
   bool isNew();
   void AdjustAPText();

signals:
   void sigSelectTrack(const QByteArray &trackData, int trackIndex);
   void sigMainAPUpdated();
   void sigPartEmpty(bool value);

public slots:
   void endEditMidi(const QByteArray& data);
   void deleteButtonClicked();
   bool trackButtonClicked(const QString& dropFileName = nullptr);
   void playButtonClicked();
   void APBoxStatusChanged();
   void parentAPBoxStatusChanged(int sigNum, bool hasMain = false);
   void ApValueChanged(bool off = false);
   void edit();
   void slotSetPlayerEnabled(bool enabled);
   void drag();
   void setCopyPasteState(const QString& file = nullptr, const QString& paste = nullptr);
   void dragActionChanged(Qt::DropAction);
   bool copy();
   bool paste();
   void exportMIDI(const QString& destination = nullptr);
   void openAutopilotSettings();

private:
   void updateMinimumSize();
   DragButton* mp_FileButton;
   QAction* mp_acCopy;
   QAction* mp_acPaste;
   QPushButton* mp_DeleteButton;
   QPushButton* mp_PlayButton;
   QByteArray m_editingTrackData;
   QWidget *APWidget = new QWidget();
   QHBoxLayout *leftl = new QHBoxLayout();
   QHBoxLayout *rightl = new QHBoxLayout();
   QLabel *APText = new QLabel();
   QLabel *PostText = new QLabel();
   QLineEdit *APBar = new QLineEdit();
   QCheckBox *mp_APBox;
   bool m_dragging;
   bool m_dropped = false;
   bool isfiniteMain;
   bool TransFill = false;
   bool newFill = false;

   int m_PlayFor;
   int m_PlayAt;

   typedef enum {
       STYLE_NONE,
       STYLE_PLAYING,
       STYLE_INVALID,
       NUMBER_OF_KEY
   }selectedStyleSheet_t;

   selectedStyleSheet_t m_selectedStyleSheet;
};

class LambdaFilter : public QObject
{
    Q_OBJECT
    std::function<bool(QObject*, QEvent*)> m_f;

protected:
    bool eventFilter(QObject* o, QEvent* event) { return m_f ? m_f(o, event) : false; }

public:
    LambdaFilter(QObject* source, const std::function<bool(QObject*, QEvent*)>& func) : m_f(func) { source->installEventFilter(this); }
};

#endif // BEATFILEWIDGET_H
