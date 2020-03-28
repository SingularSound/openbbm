#ifndef LIBCONTENT_H
#define LIBCONTENT_H

#include <QObject>
#include <QtCore>

class ContentLibrary;


class LibContent : public QObject
{
    Q_OBJECT
public:
   explicit LibContent(ContentLibrary *p_parent, const QString &name);
   bool create(QWidget *p_parentWidget);
   void invalidate();
   void resetCurrentPath();

   inline const QString &name() const {return m_name;}
   QString key() const;
   inline bool isValid() const{return m_valid;}
   QDir dir() const;
   QString defaultPath() const;
   QString currentPath() const;
   void setCurrentPath(const QString &path);

private:
   bool initialize();

private:
   ContentLibrary *mp_parent;
   QString m_name;
   bool m_valid;
   QDir m_dir;
};

#endif // LIBCONTENT_H
