#include "DrumsetLabel.h"

DrumsetLabel::DrumsetLabel(const QString& text, QWidget * parent)
    : BigTextClickableLabelWithErrorIndicator(text,parent)
{
    this->setContentsMargins(10,0,0,0);
    this->setMinimumWidth(125);
}
