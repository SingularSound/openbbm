#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QEvent>

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(const QString& text = nullptr, QWidget* parent = nullptr);

signals:
    void clicked(QMouseEvent* event);

protected:
    void mousePressEvent(QMouseEvent* event);
    bool eventFilter(QObject*, QEvent* event);
};

class BigTextClickableLabelWithErrorIndicator : public ClickableLabel
{
    Q_OBJECT

public:
    explicit BigTextClickableLabelWithErrorIndicator(const QString& text = nullptr, QWidget* parent = nullptr);

    const QString& getName() { return mName; }
    void setName(const QString& name) { setText(mName = name); }

    void setErrorState(bool state);

signals:
    void clicked(QMouseEvent* event);

protected:
    bool eventFilter(QObject*, QEvent* event);
    QString mName;
    bool mErrorState;
};

#endif // CLICKABLELABEL_H
