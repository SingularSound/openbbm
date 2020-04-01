/*
  	This software and the content provided for use with it is Copyright © 2014-2020 Singular Sound 
 	BeatBuddy Manager is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as published by
    the Free Software Foundation.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "DrumsetNameDialog.h"
#include "../../Model/drmmakermodel.h"
#include "math.h"

class QDefaultableSlider : public QSlider
{
    const int m_default;

protected:
    void mouseReleaseEvent(QMouseEvent *ev) {
        if (ev->button() == Qt::MouseButton::RightButton) {
            setValue(m_default);
        }
    }

public:
    explicit QDefaultableSlider(int value_min, int value_default, int value_max, Qt::Orientation orientation, QWidget* parent = nullptr) : QSlider(orientation, parent), m_default(value_default)
    {
        setRange(value_min, value_max);
        setValue(value_default);
    }

};

DrumsetNameDialog::DrumsetNameDialog(QString name, int volume, QWidget *parent)
    : QDialog(parent)
{
    this->setObjectName(QStringLiteral("Drumset_name_dialog"));
    this->setWindowTitle(tr("Drumset details"));

    gridLayout = new QGridLayout(this);
    gridLayout->setObjectName(QStringLiteral("gridLayout"));
    gridLayout->setContentsMargins(3, 3, 3, 3);

    auto label = new QLabel(tr("Drumset Name"), this);
    label->setObjectName(QStringLiteral("label"));
    gridLayout->addWidget(label, 0, 0);

    lineEditDrumsetName = new QLineEdit(name, this);
    lineEditDrumsetName->setObjectName(QStringLiteral("lineEditDrumsetName"));
    gridLayout->addWidget(lineEditDrumsetName, 0, 1);

    auto label_volume = new QLabel(tr("Volume (%1 dB)").arg(0), this);
    label_volume->setObjectName(QStringLiteral("label_volume"));
    gridLayout->addWidget(label_volume, 1, 0);

    sliderVolume = new QDefaultableSlider(0, 100, 159, Qt::Horizontal, this);
    sliderVolume->setToolTip(tr("Set the overall volume of the drumset\n\nRight click to reset to +0 db"));
    sliderVolume->setObjectName((QStringLiteral("horizontalVolumeSlider")));
    gridLayout->addWidget(sliderVolume, 1, 1);


    buttonBox = new QDialogButtonBox(this);
    buttonBox->setObjectName(QStringLiteral("buttonBox"));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->setContentsMargins(0, 0, 0, 0);
    gridLayout->addWidget(buttonBox, 2, 0, 1, 2);

    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(sliderVolume, &QSlider::valueChanged, [label_volume](int volume) {
        if (volume) {
            label_volume->setText(tr("Volume (%1 dB)").arg((volume >= 100 ? "+" : "") + QString::number(-20.0 * log10(100.0/volume),'g',2)));
        } else {
            label_volume->setText(tr("Volume (%1 dB)").arg("-inf"));
        }
    });
    sliderVolume->setValue(volume);

    QMetaObject::connectSlotsByName(this);
}
