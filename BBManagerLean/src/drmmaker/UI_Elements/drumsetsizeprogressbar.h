#ifndef DRUMSETSIZEPROGRESSBAR_H
#define DRUMSETSIZEPROGRESSBAR_H

#include <QWidget>
#include <QProgressBar>
#include <QLabel>

class DrumsetSizeProgressBar : public QWidget
{
    Q_OBJECT
public:
    explicit DrumsetSizeProgressBar(QWidget *parent = nullptr);
    void setDrumsetSize(quint32 size);


private:

    QProgressBar *mp_progressbar;
    QLabel * mp_RessourceText;
    quint32 m_minimum;
    quint32 m_maximum;
    quint32 m_warning;
signals:

public slots:

};

#endif // DRUMSETSIZEPROGRESSBAR_H
