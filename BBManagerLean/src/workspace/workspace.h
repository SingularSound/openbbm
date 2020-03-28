#ifndef WORKSPACE_H
#define WORKSPACE_H

#include <QObject>
#include <QDir>
#include <QRgb>

class ContentLibrary;

class Workspace : public QObject
{
   Q_OBJECT
public:
    explicit Workspace(QObject *p_parent = nullptr);
    bool create(const QFileInfo &location, QWidget *p_parentWidget);
    void resetCurrentPath();

    inline QString name() const {return "BBWorkspace";}
    inline bool isValid() const {return m_valid;}

    QDir dir() const;
    QString defaultPath() const;

    inline ContentLibrary *userLibrary(){return mp_userLibrary;}

signals:
    void sigAboutToChange();
    void sigChanged(bool valid);

private:
    bool initialize();
    void invalidate();

private:
    bool m_valid;
    QDir m_workspaceDirectory;
    ContentLibrary *mp_userLibrary;
};

#endif // WORKSPACE_H
