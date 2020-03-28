#ifndef SONGTITLEWIDGET_H
#define SONGTITLEWIDGET_H

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QSpinBox>
#include <QString>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>


class MySpinBox;
class MyComboBox;
class BeatsProjectModel;

class SongTitleWidget : public QFrame
{
   Q_OBJECT
public:
   explicit SongTitleWidget(QAbstractItemModel *p_Model, QWidget *parent = nullptr);

   void setTitle(const QString &title);
   QString title() const;
   void setNumber(int number);
   void setUnsavedChanges(bool unsaved);
   void populateDrmCombo(QModelIndex defaultDrmIndex);
   int indexOfDrmFileName(const QString &refDrmFileName);

signals:
   void sigTempoChangeByModel(int tempo);
   void sigAPEnableChangeByModel(bool state);
   void sigTempoChangeByUI(int tempo);
   void sigTitleChangeByUI(const QString &title);
   void sigNumberChangeByUI(const QString &num);
   void sigSubWidgetClicked();
   void sigDefaultDrmChangeByUI(const QString &drmName, const QString &drmFileName);
   void sigAPEnableChangeByUI(bool state);
   void sigGetAPState();

public slots:
   void slotAutoPilotChangeByModel(bool state);
   void slotTempoChangeByModel(int tempo);
   void slotTempoChangeByUI(int tempo);
   void slotTitleChangeByUI();
   void slotNumberChangeByUI();
   void slotSubWidgetClicked();
   void slotRowsAboutToBeRemoved(const QModelIndex & parent, int start, int end);
   void slotRowsInserted(const QModelIndex & parent, int start, int end);
   void slotDataChanged(const QModelIndex & topLeft, const QModelIndex & bottomRight, const QVector<int> & roles = QVector<int> ());
   void slotDefaultDrmChangeByModel(const QModelIndex &newDrmIndex);
   void slotDefaultDrmChangeByUI(int index);
   void slotBlankSaved();
   void slotAPBoxClicked ( bool checked );

private:
   void insertNewDrm(int row);
private:
   int m_Number;
   bool m_UnsavedChanges;
   QLabel *mp_Saved;
   QLineEdit *mp_Number;
   QLineEdit *mp_Title;

   QLabel *mp_TempoLabel;
   MySpinBox *mp_TempoSpin;

   QLabel *mp_DrmLabel;
   MyComboBox *mp_DrmCombo;

   QLabel * mp_APLabel;
   QCheckBox *mp_APBox;

   BeatsProjectModel *mp_beatsModel;

   int m_memorizedIndex;
   QString m_memorizedFileName;
   QString m_memorizedDrmName;

   bool m_ModelDirectedChange;

};

class MySpinBox : public QSpinBox
{
   Q_OBJECT
public:
   explicit MySpinBox(QWidget *parent = nullptr);

protected:
   bool eventFilter(QObject *obj, QEvent *event);
   void focusInEvent(QFocusEvent* event);
   void focusOutEvent(QFocusEvent* event);
};

class MyComboBox : public QComboBox
{
   Q_OBJECT
public:
   explicit MyComboBox(QWidget *parent = nullptr);

protected:
   bool eventFilter(QObject *obj, QEvent *event);
   void focusInEvent(QFocusEvent* event);
   void focusOutEvent(QFocusEvent* event);
};


#endif // SONGTITLEWIDGET_H
