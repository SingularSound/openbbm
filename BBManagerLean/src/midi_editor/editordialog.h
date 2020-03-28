#ifndef EDITORDIALOG_H
#define EDITORDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QtWidgets>
#include <midi_editor/editor.h>
#include <midi_editor/editorvelocitycolorizer.h>

class EditorDialog : public QDialog
{
    class EditorSlider : public QSlider
    {
        friend EditorDialog;
    protected:
        void mouseReleaseEvent(QMouseEvent* e) { QSlider::mouseReleaseEvent(e); (static_cast<EditorDialog*>(parentWidget()))->done(value()); }
    public:
        EditorSlider(EditorDialog* parent) : QSlider(parent) { setMinimum(-1); setMaximum(127); setTickPosition(QSlider::TicksRight); setTickInterval(16); }
    };

    EditorSlider* m_slider;
    QLabel* m_label;
    QLineEdit* m_text;
    QLinearGradient m_fill;
    QPolygon m_area;
    int m_value;

protected:
    void resizeEvent(QResizeEvent*)
    {
        auto a = m_slider->mapToParent(m_slider->rect().bottomRight());
        auto b = m_slider->mapToParent(m_slider->rect().topRight());
        auto c = m_text->mapToParent(m_text->rect().bottomRight()); c.setY(b.y());
        m_area.clear();
        m_area << a << b << c;
        m_fill.setStart(a);
        m_fill.setFinalStop(b);
    }


    void paintEvent(QPaintEvent*)
    {
        QPainter p(this);
        p.setPen(Qt::NoPen);
        p.setBrush(m_fill);
        p.drawPolygon(m_area);
    }

    void reject() { done(m_value); }

public:
    EditorDialog(EditorTableWidget* parent)
        : QDialog(parent, Qt::Tool | Qt::CustomizeWindowHint)
        , m_slider(new EditorSlider(this))
        , m_label(new QLabel(tr("Velocity:")))
        , m_text(new QLineEdit("0", this))
        , m_value(0)
    {
        // The four following lines of code are intented to be kept here as an example on how to make window completely opaque
        //setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
        //setAttribute(Qt::WA_NoSystemBackground);
        //setAttribute(Qt::WA_TranslucentBackground);
        //setAttribute(Qt::WA_PaintOnScreen);
        auto lo = new QGridLayout();
        lo->setSpacing(0);
        setLayout(lo);
        m_text->setMaximumWidth(50);
        lo->addWidget(m_label, 0, 0, 1, 2);
        auto l = new QHBoxLayout();
        l->addStretch();
        l->addWidget(m_text);
        l->addStretch();
        lo->addLayout(l, 1, 0, 1, 2);
        lo->addWidget(m_slider, 2, 0, 6, 1);
        lo->addWidget(new QLabel(tr("MAX"), this), 2, 1);
        lo->addWidget(new QLabel(tr("100"), this), 3, 1);
        lo->addWidget(new QLabel(tr(" 75"), this), 4, 1);
        lo->addWidget(new QLabel(tr(" 50"), this), 5, 1);
        lo->addWidget(new QLabel(tr(" 25"), this), 6, 1);
        lo->addWidget(new QLabel(tr("OFF"), this), 7, 1);
        lo->addWidget(new QLabel(tr("DEL"), this), 8, 1);
        m_slider->setTracking(true);
        connect(m_slider, &QSlider::valueChanged, [this](int value) {
            m_text->setText(QVariant(value).toString());
            m_text->selectAll();
        });
        m_text->setValidator(new QIntValidator(-1, 127, m_text));
        m_fill.setStops(EditorVelocityColorizer().data);
    }

    int exec(int value, QMouseEvent* e, int cur = 0, int max = 0)
    {
        m_label->setText(max ? tr("Note %1/%2:").arg(cur+1).arg(max+1) : tr("Velocity:"));
        m_slider->setValue(m_value = value);
        m_text->selectAll();
        show();
        auto p = e->globalPos();
        QStyleOptionSlider opt;
        m_slider->initStyleOption(&opt);
        auto rc = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, m_slider);
        auto ofs = rc.center()+QPoint(0, (64-value)/128.*rc.height());
        move(p-ofs-m_slider->mapToParent(QPoint(0, 0)));
        m_slider->setFocus();
        QMouseEvent fake(QEvent::MouseButtonPress, ofs, mapToParent(pos()), e->globalPos(), Qt::LeftButton, Qt::LeftButton, e->modifiers());
        m_slider->mousePressEvent(&fake);
        m_text->grabKeyboard();
        m_slider->grabMouse();
        auto ret = QDialog::exec();
        m_slider->releaseMouse();
        m_text->releaseKeyboard();
        qDebug() << "dialog" << ret << m_text->text();
        return m_text->hasSelectedText() ? ret : QVariant(m_text->text()).toInt();
    }
};

#endif // EDITORDIALOG_H
