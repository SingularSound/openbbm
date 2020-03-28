#ifndef NEWSONGWIDGET_H
#define NEWSONGWIDGET_H

#include <QFrame>
#include <QPushButton>

/**
 * @brief The NewSongWidget THe widget situated under the songs.
 *        This is the widget that adds a new song when clicked
 */
class NewSongWidget : public QFrame
{
   Q_OBJECT
public:
   explicit NewSongWidget(QWidget *parent = nullptr);

signals:
   void sigAddSongToRow(NewSongWidget* self, bool import);

public slots:
   void slotButtonClicked();
   void slotContextMenuImport();

private:
   QPushButton* mp_NewButton;
};

#include <QMenu>
#include <QMouseEvent>
class TemporaryMenu : public QMenu
{
    bool rmb;
    void setVisible(bool visible)
    {
        QMenu::setVisible(rmb = visible);
    }

    bool isPointInRegion(QPoint pos, QVector<QRect> regionRects){
        foreach (auto x, regionRects) {
            if (x.contains(pos)){
                return true;
            }
        }
        return false;
    }

    inline QMouseEvent* checkHide(QMouseEvent* event)
    {
        if (!rmb) {
            QWidget *parent = parentWidget();
            bool remainVisible = (isPointInRegion(event->pos(), visibleRegion().rects()) ||
                                  (parent && isPointInRegion(parent->mapFromGlobal(mapToGlobal(event->pos())),parent->visibleRegion().rects())));

            if(!remainVisible){
                hide();
            }
        }
        return event;
    }

    void mouseMoveEvent(QMouseEvent* event)
    {
        QMenu::mouseMoveEvent(checkHide(event));
    }

    void mousePressEvent(QMouseEvent* event)
    {
        QMenu::mousePressEvent(checkHide(event));
    }

    void mouseReleaseEvent(QMouseEvent* event)
    {
        if (event->button() == Qt::RightButton) {
            rmb = false;
        }
        QMenu::mouseReleaseEvent(checkHide(event));
    }
public:
    TemporaryMenu(QWidget* p = nullptr)
        : QMenu(p)
    {}
};

class MenuPushButton : public QPushButton
{
    Q_OBJECT
    QMenu* menu;

protected:
    void enterEvent(QEvent*)
    {
    }
    void leaveEvent(QEvent*)
    {
    }

    void mousePressEvent(QMouseEvent* event)
    {
        if (menu && event->button() == Qt::RightButton) {
            menu->move(mapToGlobal(QPoint(0, height())));
            menu->show();
            return;
        }
        return QPushButton::mousePressEvent(event);
    }
    void mouseReleaseEvent(QMouseEvent* event)
    {
        if (menu && event->button() == Qt::RightButton) {
            menu->hide();
            return;
        }
        return QPushButton::mouseReleaseEvent(event);
    }

public:
    explicit MenuPushButton(QWidget* parent)
        : QPushButton(parent)
        , menu(nullptr)
    {}

    QMenu* addMenu()
    {
        if (!menu) {
            menu = new TemporaryMenu(this);
        } else {
            menu->addSeparator();
        }
        return menu;
    }

    void disableMenu() {
        menu = new QMenu();
    }
};

#endif // NEWSONGWIDGET_H
