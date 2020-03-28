#ifndef DRUMSETLABEL_H
#define DRUMSETLABEL_H

#include "ClickableLabel.h"

class DrumsetLabel : public BigTextClickableLabelWithErrorIndicator
{
public:
     explicit DrumsetLabel(const QString& text = nullptr, QWidget * parent = nullptr);
};

#endif // DRUMSETLABEL_H
