#ifndef DROPLINEEDIT_H
#define DROPLINEEDIT_H

#include <QLineEdit>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QFileInfo>

class DropLineEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit DropLineEdit(QWidget *parent = nullptr);

    QFileInfo getFileInfo() const;
    void setFileInfo(const QFileInfo fileInfo);
    void setFilePath(const QString filePath);

    bool getValid();
    void setValid(bool value);

protected:
    virtual void dragEnterEvent(QDragEnterEvent *event);
    virtual void dropEvent(QDropEvent *event);
    virtual void dragLeaveEvent(QDragLeaveEvent *event);

private:
    void checkAndUpdate();

    QFileInfo mFileInfo;
    bool mValid;
};

#endif // DROPLINEEDIT_H
