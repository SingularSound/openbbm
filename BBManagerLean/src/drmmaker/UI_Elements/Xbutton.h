#ifndef XBUTTON_H
#define XBUTTON_H

#include <QLabel>
#include <QMouseEvent>
#include <QDebug>

class DrmMakerModel;

class XButton : public QLabel
{
    Q_OBJECT
public:
    explicit XButton(DrmMakerModel *model, QWidget *parent = nullptr);
    
signals:
    void Clicked();
    
public slots:

protected:
    void mouseReleaseEvent(QMouseEvent *);
    bool eventFilter(QObject*, QEvent* event);

    DrmMakerModel *mp_Model;

};

#endif // XBUTTON_H
