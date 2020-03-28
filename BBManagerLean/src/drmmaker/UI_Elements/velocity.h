#ifndef VELOCITY_H
#define VELOCITY_H

#include <QWidget>
#include <QHBoxLayout>
#include <QSpinBox>
#include <QPushButton>
#include <QFileDialog>
#include <QDir>
#include <QSound>

#include "DropLineEdit.h"
#include <QSoundEffect>
#include <QMediaPlayer>

class DrmMakerModel;
class FocusSpinBox;

class Velocity : public QWidget
{
    Q_OBJECT
public:
    explicit Velocity(DrmMakerModel *model, int start = 0, int end = 127, const QString& file = nullptr, QWidget *parent = nullptr);
    ~Velocity();

    // Operators
    bool operator<(const Velocity& obj) const;

    // Methods
    QString getText() const;
    bool isFileValid() const;
    bool isEmpty() const;
    QFileInfo getFileInfo() const;

    int getStart() const;
    int getEnd() const;
    void setStart(int);
    void setEnd(int);

    void setRemoveEnabled(bool value);
    void setFilePath(const QString filePath);

    quint32 size();
signals:
    void newWavSize(quint32 old_size, quint32 new_size);
    void textFieldValueChanged();
    void removeRequest(Velocity* self);
    void fileSelected(Velocity* self, const QString& file);
    void spinBoxValueChanged(Velocity* self, int id, int value, int old);

public slots:
    void on_SpinBox_valueChanged(int, int, int);
    void on_FileName_valueChanged();
    void on_Browse_clicked();
    void on_Play_clicked();
    void on_Remove_clicked();
    void on_Browse_play_file(QString filename);
    void on_Browse_select_file(QString filename, bool notify = true);
    void on_Volume_Changed(uint volume);

private:
    quint32 mWavsize;

    // Layout for all the stuff
    QHBoxLayout *mLayout;

    // UI items
    FocusSpinBox *mStartSpinBox;
    FocusSpinBox *mEndSpinBox;
    DropLineEdit *mFileName;
    QPushButton *mBrowseButton;
    QPushButton *mPlayButton;
    QPushButton *mRemoveButton;

    DrmMakerModel *mp_Model;

    quint32 wordToInt(QByteArray b);
    uint m_volume;

};


class FocusSpinBox : public QSpinBox
{
   Q_OBJECT
    int m_id, m_old;
public:
   FocusSpinBox(int id, int start, int end, int value, QWidget *parent = nullptr);
    void setValueAndNotify(int value);

signals:
   void valueChanged(int id, int value, int old);


private slots:
    void onValueChanged(int value);

protected:
   bool eventFilter(QObject *obj, QEvent *event);
   void focusInEvent(QFocusEvent* event);
   void focusOutEvent(QFocusEvent* event);
};


#endif // VELOCITY_H
