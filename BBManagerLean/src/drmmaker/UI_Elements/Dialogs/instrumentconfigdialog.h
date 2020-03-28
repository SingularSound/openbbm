#ifndef INSTRUMENTCONFIGDIALOG_H
#define INSTRUMENTCONFIGDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QLineEdit>

class DrmMakerModel;

class InstrumentConfigDialog : public QDialog
{
   Q_OBJECT
public:
   explicit InstrumentConfigDialog(DrmMakerModel *model,
                                   QString name,
                                   int midiId,
                                   int chokeGroup,
                                   int polyPhony,
                                   int volume,
                                   int fillChokeGroup,
                                   int fillChokeDelay,
                                   int nonPercussion,
                                   QWidget *parent = nullptr);

   // Getters
   QString getName();
   int getMidiId();
   int getChokeGroup();
   uint getPolyPhony();
   uint getVolume();
   uint getFillChokeGroup();
   uint getFillChokeDelay();
   uint getNonPercussion();
signals:

private slots:
   void pre_accept();
   void polySliderValueChanged(int value);
   void volSliderValueChanged(int value);
   void onFillChokeGroupSpinBoxChanged(int value);
   void onComboBoxNonPercussionChanged(int value);
private:

   // Auto generated from Designer
   QDialogButtonBox *buttonBox;
   QWidget *gridLayoutWidget;
   QGridLayout *gridLayout;
   QLineEdit *lineEditInstrumentName;
   QLabel *label;
   QLabel *label_2;
   QLabel *label_fillChokeGroup;
   QLabel *label_3;
   QLabel *label_4;
   QLabel *label_5;
   QLabel *label_6;
   QSlider *horizontalSlider;
   QSpinBox *spinBoxChokeGroup;
   QSpinBox *spinBoxFillChokeGroup;
   QSlider *horizontalVolumeSlider;
   QComboBox *comboBoxMidiId;
   QComboBox *comboBoxFillChokeDelay;
   QComboBox *comboBoxNonPercussion;

   // Manually added
   QString mName;
   int mMidiId;
   int mChokeGroup;
   uint mPolyPhony;
   uint mFillChokeGroup;
   uint mFillChokeDelay;
   uint mVolume;
   uint mNonPercussion;

   DrmMakerModel *mp_Model;
};

#endif // INSTRUMENTCONFIGDIALOG_H
