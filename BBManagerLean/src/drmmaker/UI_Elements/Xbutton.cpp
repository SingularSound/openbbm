#include "Xbutton.h"
#include "../Model/drmmakermodel.h"

XButton::XButton(DrmMakerModel *model, QWidget *parent) :
    QLabel(parent)
{
    mp_Model = model;

    this->setObjectName(QStringLiteral("Remove"));
    this->setGeometry(QRect(0, 0, 15, 15));
    this->setPixmap(QPixmap(QString::fromUtf8(":/drawable/x-mark-small.png")));
    this->setAlignment(Qt::AlignCenter);
    installEventFilter(this);
}

void XButton::mouseReleaseEvent(QMouseEvent *) {
    emit Clicked();
}

bool XButton::eventFilter(QObject*, QEvent* event) {
    if((event->type() == QEvent::Enter) && (mp_Model->getInstrumentCount() > 1)){
        this->setPixmap(QPixmap(QString::fromUtf8(":/drawable/x-mark-small-hover.png")));
        setCursor(Qt::PointingHandCursor);
    } else if(event->type() == QEvent::Leave){
        this->setPixmap(QPixmap(QString::fromUtf8(":/drawable/x-mark-small.png")));
    }
    return false;
}
